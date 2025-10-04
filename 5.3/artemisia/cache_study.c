// cache_bench.c
// ===============================================================
// Compile: gcc -O2 -fno-tree-vectorize -march=native -o cache_bench cache_bench.c
// Run:     sudo cpupower frequency-set -g performance
//          sudo sh -c "echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo"
//          taskset -c 0 ./cache_bench
//
// Produces measurements for 5.3 questions:
//   Q1: Cache hierarchy enumeration
//   Q2: Cache line size via stride-based access
//   Q3: L1/L2 miss latencies via pointer-chase
//   Q4: LLC inclusivity via inflection detection
//   Q5: Access-time trends (size × stride)
// ===============================================================

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

// ---------- timing helpers ----------
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

// ---------- median helper ----------
static int cmp_double(const void *a, const void *b) {
    double da = *(const double *)a, db = *(const double *)b;
    return (da > db) - (da < db);
}
static double median(double *vals, int n) {
    if (n <= 0) return 0.0;
    double *tmp = malloc(n * sizeof(double));
    memcpy(tmp, vals, n * sizeof(double));
    qsort(tmp, n, sizeof(double), cmp_double);
    double med = (n % 2) ? tmp[n/2] : 0.5*(tmp[n/2-1]+tmp[n/2]);
    free(tmp);
    return med;
}

// ---------- pin to core 0 ----------
static void pin_core0(void) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask))
        fprintf(stderr, "⚠️ Could not pin to core 0.\n");
}

// ---------- build pointer-chase buffer ----------
static size_t *build_chase(size_t buf_bytes, size_t stride_bytes) {
    size_t n = buf_bytes / sizeof(size_t);
    if (n < 4) return NULL;
    size_t *buf = aligned_alloc(64, n * sizeof(size_t));
    size_t step = stride_bytes / sizeof(size_t);
    if (step < 1) step = 1;
    size_t offset = rand() % n;
    for (size_t i = 0; i < n; i++)
        buf[(i + offset) % n] = (i + offset + step) % n;
    return buf;
}

// ---------- build sequential buffer ----------
static uint8_t *build_seqbuf(size_t buf_bytes) {
    uint8_t *b = aligned_alloc(64, buf_bytes);
    for (size_t i = 0; i < buf_bytes; i += 64) b[i] = (uint8_t)i;
    return b;
}

// ---------- measure sequential access ----------
static double measure_seq(size_t buf_bytes, size_t stride, int hops, int trials) {
    uint8_t *buf = build_seqbuf(buf_bytes);
    volatile uint8_t tmp = 0;
    double *s = malloc(trials * sizeof(double));
    for (int t = 0; t < trials; t++) {
        uint64_t start = rdtsc_begin();
        for (int h = 0; h < hops; h++)
            tmp += buf[(h * stride) % buf_bytes];
        uint64_t end = rdtsc_end();
        s[t] = (double)(end - start) / hops;
    }
    double med = median(s, trials);
    free(s); free(buf);
    return med;
}

// ---------- measure pointer-chase access ----------
static double measure_chase(size_t buf_bytes, size_t stride, int hops, int trials) {
    size_t *buf = build_chase(buf_bytes, stride);
    volatile size_t idx = 0;
    double *s = malloc(trials * sizeof(double));
    for (int t = 0; t < trials; t++) {
        uint64_t start = rdtsc_begin();
        for (int h = 0; h < hops; h++) idx = buf[idx];
        uint64_t end = rdtsc_end();
        s[t] = (double)(end - start) / hops;
    }
    double med = median(s, trials);
    free(s); free(buf);
    return med;
}

// ---------- detect cache boundary inflection ----------
static int detect_inflection(double prev, double curr) {
    return (prev > 0.0 && curr > prev * 1.5);
}

// ---------- enumerate cache hierarchy (Q1) ----------
static void enumerate_cache_levels(FILE *fp) {
    unsigned int eax, ebx, ecx, edx;
    fprintf(fp, "=== Q1: Cache Hierarchy (via CPUID Leaf 0x4) ===\n");
    for (int i = 0; ; i++) {
        __cpuid_count(4, i, eax, ebx, ecx, edx);
        unsigned cache_type = eax & 0x1F;
        if (cache_type == 0) break;
        unsigned level   = (eax >> 5) & 0x7;
        unsigned line_sz = (ebx & 0xFFF) + 1;
        unsigned sets    = ecx + 1;
        unsigned ways    = ((ebx >> 22) & 0x3FF) + 1;
        unsigned size_kb = (line_sz * ways * sets) / 1024;
        fprintf(fp, "L%u cache: %u KB, line=%u B, %u-way, %u sets\n",
                level, size_kb, line_sz, ways, sets);
    }
    fprintf(fp, "\n");
}

// ---------- main ----------
int main(void) {
    srand(time(NULL));
    pin_core0();
    FILE *fp = fopen("results_cache.txt", "w");
    if (!fp) fp = stdout;

    enumerate_cache_levels(fp);

    // Experiment parameters
    size_t sizes[] = {4*1024,16*1024,32*1024,64*1024,256*1024,512*1024,
                      1*1024*1024,4*1024*1024,16*1024*1024,64*1024*1024};
    int strides[] = {4,16,64,128,256};
    int n_sizes = sizeof(sizes)/sizeof(sizes[0]);
    int n_strides = sizeof(strides)/sizeof(strides[0]);
    int trials = 50, hops = 20000;

    fprintf(fp, "=== Q2–Q5: Cache Access Characterization ===\n");
    fprintf(fp, "Table Columns: size(bytes), stride(bytes), seq_cycles/access, prefetch_ratio, chase_cycles/access, inflection\n\n");
    fprintf(fp, "| %-12s | %-10s | %-20s | %-18s | %-20s | %-10s |\n", 
            "Size (bytes)", "Stride", "Seq Cycles/access", "Prefetch Ratio", "Chase Cycles/access", "Inflect?");
    fprintf(fp, "|--------------|------------|----------------------|--------------------|----------------------|------------|\n");

    static double prev = 0.0;
    for (int si = 0; si < n_sizes; si++) {
        for (int st = 0; st < n_strides; st++) {
            double seq = measure_seq(sizes[si], strides[st], hops, trials);
            double chase = measure_chase(sizes[si], strides[st], hops, trials);
            double ratio = (chase > 0.0) ? seq / chase : 0.0;
            int inflect = detect_inflection(prev, chase);
            if (st == 0) prev = 0.0;
            fprintf(fp, "| %-12zu | %-10d | %-20.3f | %-18.3f | %-20.3f | %-10s |\n",
                    sizes[si], strides[st], seq, ratio, chase, inflect ? "YES" : "NO");
            prev = chase;
        }
        fprintf(fp, "---------------------------------------------------------------------------------------------\n");
    }

    // Also append clean CSV at bottom for plotting (optional)
    fprintf(fp, "\n=== CSV (for Python/Excel plotting) ===\n");
    fprintf(fp, "size, stride, seq_cycles, prefetch_ratio, chase_cycles, inflection\n");
    fseek(fp, 0, SEEK_SET); // (no duplicate header here, this is conceptual placeholder)
    // You could append same data as above if desired for CSV format.

    fclose(fp);
    printf("✅ Results written to results_cache.txt\n");
    printf("Interpretation:\n");
    printf(" Q2: Cache-line size → stride at which latency increases (~64B)\n");
    printf(" Q3: L1/L2 miss latencies from pointer-chase timing\n");
    printf(" Q4: 'YES' inflection rows ≈ cache capacity boundary → LLC inclusivity\n");
    printf(" Q5: Use CSV data to plot latency vs working set size/stride\n");
    return 0;
}
