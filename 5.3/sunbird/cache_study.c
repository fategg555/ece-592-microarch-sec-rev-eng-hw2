#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <immintrin.h>
#include <x86intrin.h>   // <-- ensures rdtscp_serialized is defined
#include <cpuid.h>


// then all your functions

static inline uint64_t rdtscp_serialized(unsigned *aux) {
    unsigned lo, hi;
    __asm__ __volatile__(
        "rdtscp"
        : "=a"(lo), "=d"(hi), "=c"(*aux)
        :
        : "memory"
    );
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

void enumerate_cache_levels(FILE *fp) {
    unsigned int eax, ebx, ecx, edx;
    int i = 0;
    fprintf(fp, "=== Cache Hierarchy ===\n");
    while (1) {
        __cpuid_count(4, i, eax, ebx, ecx, edx);
        unsigned int cache_type = eax & 0x1F;
        if (cache_type == 0) break; // no more caches

        unsigned int level   = (eax >> 5) & 0x7;
        unsigned int line_sz = (ebx & 0xFFF) + 1;
        unsigned int sets    = ecx + 1;
        unsigned int ways    = ((ebx >> 22) & 0x3FF) + 1;
        unsigned int size_kb = (line_sz * ways * sets) / 1024;

        fprintf(fp, "L%u cache: %u KB, line size=%u B, %u-way, %u sets\n",
                level, size_kb, line_sz, ways, sets);
        i++;
    }
    fprintf(fp, "\n");
}
void measure_cache_line_size(FILE *fp) {
    fprintf(fp, "=== Cache Line Size Test ===\n");
    const size_t array_size = 1024*1024; // 1 MB
    char *arr = aligned_alloc(64, array_size);
    unsigned int temp;
    uint64_t start, end;

    for (int stride = 1; stride <= 128; stride *= 2) {
        start = rdtscp_serialized(&temp);
        for (size_t i = 0; i < array_size; i += stride) {
            arr[i]++;
        }
        end = rdtscp_serialized(&temp);
        fprintf(fp, "Stride %3d: %llu cycles\n", stride,
                (unsigned long long)(end - start));
    }
    free(arr);
    fprintf(fp, "Expect a jump at the cache line size.\n\n");
}
void measure_cache_latencies(FILE *fp) {
    fprintf(fp, "=== L1 and L2 Miss Latency Test ===\n");
    volatile char data[64] __attribute__((aligned(64)));
    unsigned int temp;
    uint64_t start, end;

    // Hit latency
    data[0] = 1;
    start = rdtscp_serialized(&temp);
    volatile char c = data[0];
    end = rdtscp_serialized(&temp);
    fprintf(fp, "L1 Hit latency: %llu cycles\n", (unsigned long long)(end - start));

    // Miss (flush forces refill from next level)
    _mm_clflush((const void *)&data[0]);
    _mm_mfence();
    start = rdtscp_serialized(&temp);
    c = data[0];
    end = rdtscp_serialized(&temp);
    fprintf(fp, "L1 Miss (~L2 hit) latency: %llu cycles\n",
            (unsigned long long)(end - start));

    // Evict from L2 (use bigger array)
    size_t big_size = 8*1024*1024; // 8 MB
    char *arr = aligned_alloc(64, big_size);
    for (size_t i = 0; i < big_size; i += 64) arr[i]++;

    start = rdtscp_serialized(&temp);
    c = data[0];
    end = rdtscp_serialized(&temp);
    fprintf(fp, "L2 Miss (~LLC hit) latency: %llu cycles\n\n",
            (unsigned long long)(end - start));
    free(arr);
}
void test_llc_inclusivity(FILE *fp) {
    fprintf(fp, "=== LLC Inclusivity Test ===\n");
    const size_t big_size = 64*1024*1024; // large enough to evict LLC
    char *arr = aligned_alloc(64, big_size);
    volatile char line[64] __attribute__((aligned(64)));
    unsigned int temp;
    uint64_t start, end;

    line[0] = 42; // bring into L1
    _mm_mfence();

    // Access timing before eviction
    start = rdtscp_serialized(&temp);
    volatile char c = line[0];
    end = rdtscp_serialized(&temp);
    uint64_t before = end - start;

    // Thrash LLC
    for (size_t i = 0; i < big_size; i += 64) arr[i]++;

    // Access timing after eviction
    start = rdtscp_serialized(&temp);
    c = line[0];
    end = rdtscp_serialized(&temp);
    uint64_t after = end - start;

    fprintf(fp, "Before eviction: %llu cycles\n", (unsigned long long)before);
    fprintf(fp, "After eviction:  %llu cycles\n", (unsigned long long)after);

    if (after > before * 2) {
        fprintf(fp, "→ Likely Inclusive LLC (line evicted from L1)\n");
    } else {
        fprintf(fp, "→ Likely Non-inclusive/Exclusive LLC\n");
    }
    free(arr);
    fprintf(fp, "\n");
}
void plot_access_patterns(FILE *fp_csv) {
    size_t sizes[] = {4*1024, 32*1024, 256*1024, 1*1024*1024,
                      4*1024*1024, 16*1024*1024, 64*1024*1024};
    int strides[] = {1,2,4,8,16,32,64,128,256,512};
    unsigned int temp;
    uint64_t start, end;

    fprintf(fp_csv, "size,stride,cycles\n");

    for (int s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
        for (int st = 0; st < sizeof(strides)/sizeof(strides[0]); st++) {
            char *arr = aligned_alloc(64, sizes[s]);
            for (size_t i = 0; i < sizes[s]; i += 64) arr[i] = 1; // warmup

            start = rdtscp_serialized(&temp);
            for (size_t i = 0; i < sizes[s]; i += strides[st]) arr[i]++;
            end = rdtscp_serialized(&temp);

            fprintf(fp_csv, "%zu,%d,%llu\n", sizes[s], strides[st],
                    (unsigned long long)(end - start));
            free(arr);
        }
    }
}
int main() {
    FILE *fp = fopen("results_cache.txt", "w");
    FILE *csv = fopen("access_patterns.csv", "w");

    enumerate_cache_levels(fp);    // Q1
    measure_cache_line_size(fp);   // Q2
    measure_cache_latencies(fp);   // Q3
    test_llc_inclusivity(fp);      // Q4
    plot_access_patterns(csv);     // Q5

    fclose(fp);
    fclose(csv);
    printf("All results written to results_cache.txt and access_patterns.csv\n");
    return 0;
}
