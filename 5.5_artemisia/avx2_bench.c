#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>
#include <x86intrin.h>

static inline uint64_t rdtscp_serialized(void) {
    unsigned lo, hi, aux;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}

int main() {
    const int N = 10000000;   // loop iterations
    uint64_t start, end;

    FILE *fp = fopen("avx2_results.txt", "w");
    if (!fp) {
        fprintf(stderr, "Could not open results file\n");
        return 1;
    }

    // -----------------------------
    // 1. AVX2 Throughput (≥8 streams)
    // -----------------------------
    __m256i a0 = _mm256_set1_epi32(1);
    __m256i a1 = _mm256_set1_epi32(2);
    __m256i a2 = _mm256_set1_epi32(3);
    __m256i a3 = _mm256_set1_epi32(4);
    __m256i a4 = _mm256_set1_epi32(5);
    __m256i a5 = _mm256_set1_epi32(6);
    __m256i a6 = _mm256_set1_epi32(7);
    __m256i a7 = _mm256_set1_epi32(8);

    start = rdtscp_serialized();
    for (int i = 0; i < N; i++) {
        a0 = _mm256_add_epi32(a0, a1);
        a2 = _mm256_add_epi32(a2, a3);
        a4 = _mm256_add_epi32(a4, a5);
        a6 = _mm256_add_epi32(a6, a7);
        a1 = _mm256_add_epi32(a1, a0);
        a3 = _mm256_add_epi32(a3, a2);
        a5 = _mm256_add_epi32(a5, a4);
        a7 = _mm256_add_epi32(a7, a6);
    }
    end = rdtscp_serialized();

    uint64_t cycles_throughput = end - start;
    long long total_ops = (long long)N * 8;  // 8 ops per iter
    double cpi = (double)cycles_throughput / total_ops;
    double ipc = 1.0 / cpi;

    printf("=== AVX2 Throughput Test ===\n");
    printf("Total cycles: %llu\n", (unsigned long long)cycles_throughput);
    printf("Total ops:    %lld\n", total_ops);
    printf("CPI: %.4f\nIPC: %.4f\n\n", cpi, ipc);

    fprintf(fp, "=== AVX2 Throughput Test ===\n");
    fprintf(fp, "Total cycles: %llu\n", (unsigned long long)cycles_throughput);
    fprintf(fp, "Total ops:    %lld\n", total_ops);
    fprintf(fp, "CPI: %.4f\nIPC: %.4f\n\n", cpi, ipc);

    // -----------------------------
    // 2. AVX2 Latency (dependent chain)
    // -----------------------------
    __m256i b = _mm256_set1_epi32(1);

    start = rdtscp_serialized();
    for (int i = 0; i < N; i++) {
        b = _mm256_add_epi32(b, b); // dependent chain
    }
    end = rdtscp_serialized();
    uint64_t cycles_latency = end - start;

    // Empty loop overhead
    start = rdtscp_serialized();
    for (int i = 0; i < N; i++) {
        asm volatile("" ::: "memory");
    }
    end = rdtscp_serialized();
    uint64_t cycles_overhead = end - start;

    double latency = (double)(cycles_latency - cycles_overhead) / N;

    printf("=== AVX2 Latency Test ===\n");
    printf("Total cycles (with ops): %llu\n", (unsigned long long)cycles_latency);
    printf("Empty loop overhead:     %llu\n", (unsigned long long)cycles_overhead);
    printf("Latency per op:          %.2f cycles\n", latency);

    fprintf(fp, "=== AVX2 Latency Test ===\n");
    fprintf(fp, "Total cycles (with ops): %llu\n", (unsigned long long)cycles_latency);
    fprintf(fp, "Empty loop overhead:     %llu\n", (unsigned long long)cycles_overhead);
    fprintf(fp, "Latency per op:          %.2f cycles\n", latency);

    fclose(fp);
    return 0;
}
