#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <immintrin.h>
#include <x86intrin.h>

static inline uint64_t rdtscp_serialized(void) {
    unsigned lo, hi, aux;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}

// --- Worker that stresses the ROB with dependent ops
void *rob_stress(void *arg) {
    __m256i x = _mm256_set1_epi32(1);
    while (1) {
        for (int i = 0; i < 1000000; i++) {
            x = _mm256_add_epi32(x, x); // dependency chain
        }
        __asm__ volatile("" :: "x"(x));
    }
    return NULL;
}

// --- Worker that stresses the BTB with many branch targets
void *btb_stress(void *arg) {
    volatile int sum = 0;
    while (1) {
        for (int i = 0; i < 1000000; i++) {
            if (i & 1) sum++;
            if (i & 2) sum++;
            if (i & 4) sum++;
            if (i & 8) sum++;
        }
    }
    return NULL;
}

// --- Measurement thread
typedef struct {
    const char *mode;       // "ROB" or "BTB"
    const char *condition;  // "Baseline" or "Stress"
    FILE *fp;
} meas_args;

void *measure(void *arg) {
    meas_args *margs = (meas_args*)arg;
    const char *mode = margs->mode;
    const char *cond = margs->condition;
    FILE *fp = margs->fp;
    uint64_t start, end;

    for (int t = 0; t < 10; t++) {
        start = rdtscp_serialized();
        if (mode[0] == 'R') {
            __m256i x = _mm256_set1_epi32(1);
            for (int i = 0; i < 10000000; i++) {
                x = _mm256_add_epi32(x, x);
            }
            __asm__ volatile("" :: "x"(x));
        } else {
            volatile int sum = 0;
            for (int i = 0; i < 10000000; i++) {
                if (i & 1) sum++;
                if (i & 2) sum++;
                if (i & 4) sum++;
                if (i & 8) sum++;
            }
        }
        end = rdtscp_serialized();
        uint64_t cycles = end - start;

        // Print to terminal
        printf("%s [%s] trial %d: %lu cycles\n", mode, cond, t, cycles);

        // Log to CSV
        fprintf(fp, "%s,%s,%d,%lu\n", mode, cond, t, cycles);
        fflush(fp);
    }
    return NULL;
}

// --- Pin a thread to a specific logical CPU
void pin_thread(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        perror("pthread_setaffinity_np");
        exit(1);
    }
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <coreA> <coreB> <mode:R|B> <0=baseline|1=stress>\n", argv[0]);
        return 1;
    }
    int coreA = atoi(argv[1]);
    int coreB = atoi(argv[2]);
    char mode = argv[3][0];
    int stress_enabled = atoi(argv[4]);

    // Open results file
    FILE *fp = fopen("ht_results.csv", "a");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    // If file is new, write header
    static int header_written = 0;
    if (!header_written) {
        fprintf(fp, "Mode,Condition,Trial,Cycles\n");
        header_written = 1;
    }

    pthread_t stress, meas;

    // Launch stressor if requested
    if (stress_enabled) {
        if (mode == 'R')
            pthread_create(&stress, NULL, rob_stress, NULL);
        else
            pthread_create(&stress, NULL, btb_stress, NULL);
        pin_thread(coreA);
    }

    // Launch measurement thread
    meas_args args = { mode == 'R' ? "ROB" : "BTB",
                       stress_enabled ? "Stress" : "Baseline", fp };
    pthread_create(&meas, NULL, measure, &args);
    pin_thread(coreB);

    pthread_join(meas, NULL);
    fclose(fp);
    return 0;
}