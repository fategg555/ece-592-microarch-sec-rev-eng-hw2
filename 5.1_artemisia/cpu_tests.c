#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <immintrin.h>
#include <cpuid.h>

typedef struct {
    int cache_levels;
    int branch_prediction;
    int pipelined;
    uint64_t predictable_cycles;
    uint64_t unpredictable_cycles;
    uint64_t dependent_cycles;
    uint64_t independent_cycles;
} Results;

Results results = {0, 0, 0, 0, 0, 0, 0};

FILE *log_fp;

// ------------------ CPU Feature Check (CPUID) ------------------
void check_cpu_features() {
    unsigned int eax, ebx, ecx, edx;
    int level_count = 0;

    fprintf(log_fp, "=== CPU Feature Check (CPUID) ===\n");

    int i = 0;
    while (1) {
        __cpuid_count(4, i, eax, ebx, ecx, edx);
        unsigned int cache_type = eax & 0x1F;
        if (cache_type == 0) break; // no more caches

        unsigned int level = (eax >> 5) & 0x7;
        fprintf(log_fp, "Cache level %u present\n", level);
        level_count++;
        i++;
    }
    results.cache_levels = level_count;

    results.branch_prediction = 1; // assume yes for modern CPUs
    results.pipelined = 1;         // assume yes for modern CPUs

    fprintf(log_fp, "Branch prediction: YES (all modern x86 CPUs)\n");
    fprintf(log_fp, "Pipelined: YES (all modern x86 CPUs)\n");
    fprintf(log_fp, "=================================\n\n");
}

// ------------------ Cache Levels Test ------------------
void cache_levels_test() {
    fprintf(log_fp, "=== Cache Latency Staircase Test ===\n");
    size_t sizes[] = {4*1024, 32*1024, 256*1024, 4*1024*1024, 64*1024*1024}; // 4KB–64MB
    int n = sizeof(sizes)/sizeof(sizes[0]);
    unsigned int temp;
    uint64_t start, end;
    volatile char *arr;

    for (int i = 0; i < n; i++) {
        arr = aligned_alloc(64, sizes[i]);
        for (size_t j = 0; j < sizes[i]; j += 64) arr[j] = 1; // warm up

        start = __rdtscp(&temp);
        for (size_t j = 0; j < sizes[i]; j += 64) {
            arr[j]++; // touch every cache line
        }
        end = __rdtscp(&temp);

        fprintf(log_fp, "Size %8zu bytes: %llu cycles\n", sizes[i],
                (unsigned long long)(end - start));
        free((void*)arr);
    }
    fprintf(log_fp, "Look for jumps in latency = cache boundaries\n\n");
}

// ------------------ Branch Prediction Test ------------------
void branch_prediction_test() {
    fprintf(log_fp, "=== Branch Prediction Test ===\n");
    unsigned int temp;
    uint64_t start, end;
    volatile int sum = 0;

    // Predictable branch
    start = __rdtscp(&temp);
    for (int i = 0; i < 10000000; i++) {
        if ((i & 1) == 0) sum++;
    }
    end = __rdtscp(&temp);
    results.predictable_cycles = end - start;
    fprintf(log_fp, "Predictable branch cycles: %llu\n",
            (unsigned long long)results.predictable_cycles);

    // Unpredictable branch
    srand(time(NULL));
    start = __rdtscp(&temp);
    for (int i = 0; i < 10000000; i++) {
        if (rand() & 1) sum++;
    }
    end = __rdtscp(&temp);
    results.unpredictable_cycles = end - start;
    fprintf(log_fp, "Unpredictable branch cycles: %llu\n",
            (unsigned long long)results.unpredictable_cycles);

    if (results.unpredictable_cycles > results.predictable_cycles * 1.2) {
        results.branch_prediction = 1;
    } else {
        results.branch_prediction = 0;
    }

    fprintf(log_fp,
            "If unpredictable >> predictable → Branch prediction confirmed\n\n");
}

// ------------------ Pipeline Test ------------------
void pipeline_test() {
    fprintf(log_fp, "=== Pipeline Test ===\n");
    unsigned int temp;
    uint64_t start, end;
    volatile int x = 1;

    // Dependent operations (serial)
    start = __rdtscp(&temp);
    for (int i = 0; i < 10000000; i++) {
        x = x + 1; // each depends on previous
    }
    end = __rdtscp(&temp);
    results.dependent_cycles = end - start;
    fprintf(log_fp, "Dependent ops: %llu cycles\n",
            (unsigned long long)results.dependent_cycles);

    // Independent operations (parallelizable)
    volatile int a=1,b=2,c=3,d=4;
    start = __rdtscp(&temp);
    for (int i = 0; i < 10000000; i++) {
        a++; b++; c++; d++; // independent, can pipeline
    }
    end = __rdtscp(&temp);
    results.independent_cycles = end - start;
    fprintf(log_fp, "Independent ops: %llu cycles\n",
            (unsigned long long)results.independent_cycles);

    if (results.independent_cycles < results.dependent_cycles) {
        results.pipelined = 1;
    } else {
        results.pipelined = 0;
    }

    fprintf(log_fp,
            "If independent ops are faster per op → Pipeline confirmed\n\n");
}

// ------------------ Write Summary ------------------
void write_summary() {
    fprintf(log_fp, "=== Final Results ===\n");
    fprintf(log_fp, "Cache: %s\n", results.cache_levels > 0 ? "Yes" : "No");
    fprintf(log_fp, "Cache Levels Detected: %d\n", results.cache_levels);
    fprintf(log_fp, "Branch Prediction: %s\n",
            results.branch_prediction ? "Yes" : "No");
    fprintf(log_fp, "Predictable branch cycles: %llu\n",
            (unsigned long long)results.predictable_cycles);
    fprintf(log_fp, "Unpredictable branch cycles: %llu\n",
            (unsigned long long)results.unpredictable_cycles);
    fprintf(log_fp, "Pipelined: %s\n", results.pipelined ? "Yes" : "No");
    fprintf(log_fp, "Dependent ops cycles: %llu\n",
            (unsigned long long)results.dependent_cycles);
    fprintf(log_fp, "Independent ops cycles: %llu\n",
            (unsigned long long)results.independent_cycles);
    fprintf(log_fp, "=====================\n");
}

// ------------------ Main ------------------
int main() {
    log_fp = fopen("results.txt", "w");
    if (!log_fp) {
        fprintf(stderr, "Error: Could not open results.txt\n");
        return 1;
    }

    check_cpu_features();     // Q1/Q2
    cache_levels_test();      // Q2 experimental
    branch_prediction_test(); // Q3
    pipeline_test();          // Q4
    write_summary();          // summary

    fclose(log_fp);
    printf("All results written to results.txt\n");
    return 0;
}
