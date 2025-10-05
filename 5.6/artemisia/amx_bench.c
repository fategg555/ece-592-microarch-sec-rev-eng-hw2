#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <time.h>
#include <immintrin.h>
#include <x86intrin.h>
#include <pthread.h>

#define TILE_M 16
#define TILE_N 16
#define TILE_K 16
#define TRIALS 10
#define NITER 200   //repeat to amplify cycles

// ---------------- timing helper ----------------
static inline uint64_t rdtscp_serialized(void) {
    unsigned lo, hi, aux;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}

// ---------------- pin thread ----------------
static void pin_thread(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}

// ---------------- median ----------------
static uint64_t median(uint64_t *v, int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (v[j] < v[i]) {
                uint64_t t = v[i];
                v[i] = v[j];
                v[j] = t;
            }
    return v[n / 2];
}

int main() {
    pin_thread(0);
    srand(time(NULL));

    FILE *fp = fopen("amx_sparsity_results.csv", "w");
    if (!fp) { perror("fopen"); return 1; }
    fprintf(fp, "sparsity,trial,cycles\n");

    // allocate matrices
    float (*A)[TILE_K] = aligned_alloc(64, sizeof(float) * TILE_M * TILE_K);
    float (*B)[TILE_N] = aligned_alloc(64, sizeof(float) * TILE_K * TILE_N);
    float (*C)[TILE_N] = aligned_alloc(64, sizeof(float) * TILE_M * TILE_N);

    printf("⚠️ AMX not available — using software emulation of TMUL.\n");
    printf("Running %d trials per sparsity level...\n\n", TRIALS);

    float sparsity_levels[] = {0.0, 0.25, 0.5, 0.75, 1.0};
    for (int s = 0; s < 5; s++) {
        float sp = sparsity_levels[s];
        uint64_t times[TRIALS];

        for (int t = 0; t < TRIALS; t++) {
            // fill A, B with zeros based on sparsity
            for (int i = 0; i < TILE_M; i++)
                for (int j = 0; j < TILE_K; j++)
                    A[i][j] = ((float)rand() / RAND_MAX < sp) ? 0.0f : 1.0f;
            for (int i = 0; i < TILE_K; i++)
                for (int j = 0; j < TILE_N; j++)
                    B[i][j] = ((float)rand() / RAND_MAX < sp) ? 0.0f : 1.0f;
            memset(C, 0, sizeof(float) * TILE_M * TILE_N);

            _mm_mfence();
            uint64_t start = rdtscp_serialized();

            // software “TMUL” loop (repeat NITER times)
            for (int rep = 0; rep < NITER; rep++) {
                for (int i = 0; i < TILE_M; i++) {
                    for (int j = 0; j < TILE_N; j++) {
                        float sum = 0.0f;
                        for (int k = 0; k < TILE_K; k++) {
                            sum += A[i][k] * B[k][j];
                        }
                        C[i][j] += sum;
                    }
                }
            }

            _mm_mfence();
            uint64_t end = rdtscp_serialized();

            uint64_t delta = end - start;
            times[t] = delta;
            fprintf(fp, "%.2f,%d,%llu\n", sp, t, (unsigned long long)delta);
        }

        uint64_t med = median(times, TRIALS);
        double avg = 0;
        for (int i = 0; i < TRIALS; i++) avg += times[i];
        avg /= TRIALS;
        printf("Sparsity %.2f → median %llu cycles (avg %.1f)\n",
               sp, (unsigned long long)med, avg);
    }

    fclose(fp);
    free(A); free(B); free(C);
    printf("\nResults written to amx_sparsity_results.csv\n");
    return 0;
}