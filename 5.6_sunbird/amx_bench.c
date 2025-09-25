#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>
#include <x86intrin.h>
#include <string.h>

// Timing
static inline uint64_t rdtscp_serialized(void) {
    unsigned lo, hi, aux;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}

#define TILE_M 16
#define TILE_N 16
#define TILE_K 16

int main() {
    // Tile configuration
    struct {
        uint16_t palette_id;
        uint16_t reserved;
        uint32_t colsb[8];
        uint16_t rows[8];
    } __attribute__((packed, aligned(64))) cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.palette_id = 1; // AMX BF16 palette
    cfg.colsb[0] = TILE_K * 2; // bytes per row (BF16 = 2 bytes)
    cfg.rows[0] = TILE_M;
    cfg.colsb[1] = TILE_N * 4; // result (FP32)
    cfg.rows[1] = TILE_M;

    __tile_loadconfig(&cfg);

    // Allocate matrices
    uint16_t A[TILE_M][TILE_K]; // BF16 (16-bit)
    uint16_t B[TILE_K][TILE_N];
    float C[TILE_M][TILE_N];

    // Example: vary sparsity
    float sparsity_levels[] = {0.0, 0.25, 0.5, 0.75, 1.0};
    for (int s = 0; s < 5; s++) {
        float sparsity = sparsity_levels[s];

        // Fill A and B with controlled sparsity
        for (int i = 0; i < TILE_M; i++) {
            for (int j = 0; j < TILE_K; j++) {
                A[i][j] = ((float)rand()/RAND_MAX < sparsity) ? 0 : 0x3f80; // BF16 for 1.0
            }
        }
        for (int i = 0; i < TILE_K; i++) {
            for (int j = 0; j < TILE_N; j++) {
                B[i][j] = ((float)rand()/RAND_MAX < sparsity) ? 0 : 0x3f80;
            }
        }

        memset(C, 0, sizeof(C));

        // Load tiles
        __tile_loadd(0, A, TILE_K * 2);   // A
        __tile_loadd(1, B, TILE_N * 2);   // B
        __tile_loadd(2, C, TILE_N * 4);   // C

        uint64_t start = rdtscp_serialized();

        // Run TMUL
        __tile_dpbf16ps(2, 0, 1);

        uint64_t end = rdtscp_serialized();

        printf("Sparsity %.2f → %llu cycles\n", sparsity, (unsigned long long)(end - start));

        // Store result back if needed
        __tile_stored(C, 2, TILE_N * 4);
    }

    __tile_release(); // free resources
    return 0;
}
