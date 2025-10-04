// cache_bench.c
// Compile: gcc -O2 -fno-tree-vectorize -march=native -o cache_bench cache_bench.c
// Run (recommended): sudo cpupower frequency-set -g performance
//                   sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"  # (optional, root)
//                   taskset -c 0 ./cache_bench
//
// This program:
//  - enumerates caches via CPUID
//  - for many working-set sizes and strides measures:
//      * sequential access cycles/access (exercises prefetch / vectorization opt)
//      * pointer-chase cycles/access (defeats prefetch & out-of-order loads)
//  - computes medians over trials, does warmup, randomizes offsets
//  - reports candidate cache-size "inflection" when latency jumps
//
// NOTE: disabling turbo/frequency control requires root or external steps (documented above).

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <immintrin.h>
#include <x86intrin.h>
#include <cpuid.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>

// ---------- rdtsc helpers (serialized) ----------
static inline uint64_t rdtsc_begin(void) {
    unsigned a, d;
    __asm__ __volatile__("cpuid\n\t"
                         "rdtsc\n\t"
                         : "=a"(a), "=d"(d)
                         :
                         : "rbx", "rcx");
    return ((uint64_t)d << 32) | a;
}
static inline uint64_t rdtsc_end(void) {
    unsigned a, d;
    __asm__ __volatile__("rdtscp\n\t"
                         "mov %%eax, %0\n\t"
                         "mov %%edx, %1\n\t"
                         "cpuid\n\t"
                         : "=r"(a), "=r"(d)
                         :
                         : "rax","rbx","rcx","rdx");
    return ((uint64_t)d << 32) | a;
}

// Comparison helper for qsort
static int cmp_double(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

static double median_from_array(double *vals, int n) {
    if (n <= 0) return 0.0;
    double *c = malloc(sizeof(double) * n);
    if (!c) return 0.0;
    memcpy(c, vals, sizeof(double) * n);
    qsort(c, n, sizeof(double), cmp_double);
    double med;
    if (n % 2)
        med = c[n / 2];
    else
        med = 0.5 * (c[n / 2 - 1] + c[n / 2]);
    free(c);
    return med;
}


// ---------- pin to core 0 ----------
static void pin_to_core0(void) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) != 0) {
        perror("sched_setaffinity");
        // not fatal — continue but warn
        fprintf(stderr, "Warning: failed to pin to core 0; results less reproducible\n");
    }
}

// ---------- build pointer-chase permutation (defeats prefetch) ----------
static size_t *build_chase(size_t buf_bytes, size_t stride_bytes) {
    size_t n = buf_bytes / sizeof(size_t);
    if (n < 4) return NULL;
    size_t *buf = aligned_alloc(64, n * sizeof(size_t));
    if (!buf) return NULL;

    size_t step = stride_bytes / sizeof(size_t);
    if (step < 1) step = 1;

    // simple pseudo-random permutation using modular step and a random start
    // ensures every index links to exactly one next index (cyclic)
    size_t offset = (size_t)rand() % n;
    for (size_t i = 0; i < n; ++i) {
        buf[(i + offset) % n] = (i + offset + step) % n;
    }
    // touch to allocate pages
    for (size_t i = 0; i < n; i += (4096/sizeof(size_t))) buf[i] = buf[i];
    return buf;
}

// ---------- sequential buffer builder ----------
static uint8_t *build_seqbuf(size_t buf_bytes) {
    uint8_t *b = aligned_alloc(64, buf_bytes);
    if (!b) return NULL;
    // touch pages
    for (size_t i = 0; i < buf_bytes; i += 64) b[i] = (uint8_t)i;
    return b;
}

// ---------- measure pointer-chase median cycles/access ----------
static double measure_chase_median(size_t *buf, size_t buf_len, int hops_per_trial, int trials) {
    if (!buf) return -1.0;
    volatile size_t idx = 0;
    // warmup: dependent hops
    for (int w = 0; w < 1000; ++w) idx = buf[idx % buf_len];

    double *samples = malloc(sizeof(double)*trials);
    if (!samples) return -1.0;

    for (int t = 0; t < trials; ++t) {
        uint64_t start = rdtsc_begin();
        for (int h = 0; h < hops_per_trial; ++h) idx = buf[idx % buf_len];
        uint64_t end = rdtsc_end();
        samples[t] = (double)(end - start) / (double)hops_per_trial;
    }
    double med = median_from_array(samples, trials);
    free(samples);
    (void)idx;
    return med;
}

// ---------- measure sequential median cycles/access ----------
static double measure_seq_median(uint8_t *buf, size_t buf_bytes, int hops_per_trial, int trials, size_t stride_bytes) {
    if (!buf) return -1.0;
    size_t steps = buf_bytes / stride_bytes;
    if (steps < 1) return -1.0;
    volatile uint8_t tmp = 0;
    // warmup
    for (size_t w = 0; w < 1000 && w < steps; ++w) tmp += buf[(w*stride_bytes) % buf_bytes];

    double *samples = malloc(sizeof(double)*trials);
    if (!samples) return -1.0;

    for (int t = 0; t < trials; ++t) {
        uint64_t start = rdtsc_begin();
        // do hops_per_trial loads in a loop stepping through sequence
        size_t idx = 0;
        for (int h = 0; h < hops_per_trial; ++h) {
            tmp += buf[(idx * stride_bytes) % buf_bytes];
            idx++;
            if (idx >= steps) idx = 0;
        }
        uint64_t end = rdtsc_end();
        samples[t] = (double)(end - start) / (double)hops_per_trial;
    }
    double med = median_from_array(samples, trials);
    free(samples);
    (void)tmp;
    return med;
}

// ---------- detect inflection (simple heuristic) ----------
static int detect_inflection(double prev, double current) {
    // treat as inflection if latency increased by >= 1.5x
    if (prev <= 0.0) return 0;
    return (current > prev * 1.5) ? 1 : 0;
}

// ---------- enumerate caches (unchanged) ----------
void enumerate_cache_levels(FILE *fp) {
    unsigned int eax, ebx, ecx, edx;
    int i = 0;
    fprintf(fp, "=== Cache Hierarchy (CPUID) ===\n");
    while (1) {
        __cpuid_count(4, i, eax, ebx, ecx, edx);
        unsigned int cache_type = eax & 0x1F;
        if (cache_type == 0) break;
        unsigned int level   = (eax >> 5) & 0x7;
        unsigned int line_sz = (ebx & 0xFFF) + 1;
        unsigned int sets    = ecx + 1;
        unsigned int ways    = ((ebx >> 22) & 0x3FF) + 1;
        unsigned int size_kb = (line_sz * ways * sets) / 1024;
        fprintf(fp, "L%u cache: %u KB, line=%u B, %u-way, %u sets\n",
                level, size_kb, line_sz, ways, sets);
        i++;
    }
    fprintf(fp, "\n");
}

// ---------------- main experiment ----------------
int main(void) {
    srand((unsigned)time(NULL));
    pin_to_core0();

    FILE *out = fopen("results_cache.txt", "w");
    if (!out) out = stdout;

    enumerate_cache_levels(out);
    fprintf(out, "NOTE: For reproducible results: pin the process (done), set CPU governor to performance, disable Turbo if possible.\n");
    fprintf(out, "Compile with: -O2 -fno-tree-vectorize -march=native\n\n");

    // sizes sweep (bytes) from small L1-ish to >LLC
    size_t sizes[] = {
        4*1024,        // 4KB
        16*1024,       // 16KB
        32*1024,       // 32KB
        64*1024,       // 64KB
        256*1024,      // 256KB
        512*1024,      // 512KB
        1*1024*1024,   // 1MB
        4*1024*1024,   // 4MB
        16*1024*1024,  // 16MB
        64*1024*1024   // 64MB
    };
    int n_sizes = sizeof(sizes)/sizeof(sizes[0]);

    int strides[] = {4, 16, 64, 128, 256}; // bytes
    int n_strides = sizeof(strides)/sizeof(strides[0]);

    int trials = 100;          // repeat runs (statistics)
    int hops_per_trial = 20000; // number of dependent loads per trial (pointer-chase) / seq accesses

    fprintf(out, "size(bytes),stride(bytes),seq_cycles_per_access,prefetch_effect_ratio,chase_cycles_per_access,inflection\n");

    for (int si = 0; si < n_sizes; ++si) {
        for (int sti = 0; sti < n_strides; ++sti) {
            size_t buf_bytes = sizes[si];
            size_t stride = strides[sti];
            // build pointer-chase buffer
            size_t *chase = build_chase(buf_bytes, stride);
            uint8_t *seq = build_seqbuf(buf_bytes);

            if (!chase || !seq) {
                if (chase) free(chase);
                if (seq) free(seq);
                fprintf(out, "%zu,%zu,NA,NA,NA,NA\n", buf_bytes, stride);
                continue;
            }

            // measure pointer-chase (defeats prefetch)
            double chase_med = measure_chase_median(chase, buf_bytes/sizeof(size_t), hops_per_trial, trials);

            // measure sequential (prefetch-eligible)
            double seq_med = measure_seq_median(seq, buf_bytes, hops_per_trial, trials, stride);

            // prefetch effect: how much faster sequential is vs chase ( >1 means seq faster)
            double ratio = (chase_med > 0.0) ? seq_med / chase_med : 0.0;

            // naive inflection detection using previous size in same stride
            static double prev_chase = 0.0;
            int inflection = detect_inflection(prev_chase, chase_med);
            // reset prev when stride changes
            if (sti == 0) prev_chase = 0.0;
            // print
            fprintf(out, "%zu,%zu,%.3f,%.3f,%.3f,%s\n",
                    buf_bytes, stride,
                    (seq_med>0?seq_med:-1.0),
                    (ratio>0?ratio:-1.0),
                    (chase_med>0?chase_med:-1.0),
                    inflection ? "YES" : "NO");

            // update prev
            prev_chase = chase_med;

            free(chase);
            free(seq);
            // small sleep to reduce OS jitter between experiments
            struct timespec ts = {0, 20000000}; // 20 ms
            nanosleep(&ts, NULL);
        }
    }

    if (out != stdout) fclose(out);

    printf("Done. Results in results_cache.txt\n");
    printf("Interpreting results:\n"
           " - If seq_cycles << chase_cycles (ratio << 1), prefetch/vectorization is helping (optimization present).\n"
           " - Look for 'inflection' YES entries: these are sizes where pointer-chase latency jumped -> candidate capacity boundary.\n"
           "Recommendations: pin to a core (done), set CPU governor to performance, disable TurboBoost for tightest reproducibility.\n");
    return 0;
}
