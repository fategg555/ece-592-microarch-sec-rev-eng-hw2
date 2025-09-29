#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <immintrin.h>
#include <x86intrin.h>

#define N 10000000

int arr[N];
unsigned int temp;  // for rdtscp

static inline unsigned long long rdtscp_serialized() {
    unsigned lo, hi, aux;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux) :: "memory");
    return ((unsigned long long)hi << 32) | lo;
}

void streaming_access() {
    for (int i = 0; i < N; i++) {
        arr[i] = arr[i] * 2;  // Sequential access
    }
}

void randomized_access() {
    for (int i = 0; i < N; i++) {
        int idx = rand() % N;
        arr[idx] = arr[idx] * 2;  // Random access
    }
}

int main() {
    FILE *fp = fopen("results.txt", "w");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    unsigned long long begin = rdtscp_serialized();
    streaming_access();
    unsigned long long finish = rdtscp_serialized();

    unsigned long long begin_random = rdtscp_serialized();
    randomized_access();
    unsigned long long finish_random = rdtscp_serialized();

    fprintf(fp, "Streaming access time: %llu cycles\n", (finish - begin));
    fprintf(fp, "Randomized access time: %llu cycles\n", (finish_random - begin_random));

    fclose(fp);
    return 0;
}
