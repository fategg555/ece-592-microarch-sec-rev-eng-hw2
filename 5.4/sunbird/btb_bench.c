#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <x86intrin.h>

FILE *log_fp;

// ------------------ Branch Array Helper ------------------
#define MAX_BRANCHES 1024

typedef void (*branch_fn_t)(void);
typedef void (*empty_fn_t)(uint64_t);

// Simple branch function that does nothing
void branch_stub() {}

void empty(uint64_t p){}; // empty function which enforces parameter passing

// ------------------ BTB Capacity Test ------------------
void btb_capacity_test() {
    fprintf(log_fp, "=== BTB Capacity Test ===\n");

    branch_fn_t *branches = malloc(MAX_BRANCHES * sizeof(branch_fn_t));
    for (int i = 0; i < MAX_BRANCHES; i++) branches[i] = branch_stub;

    // Pointer-chase through branch functions to stress BTB
    volatile int dummy = 0;
    unsigned int temp;
    for (int num_branches = 4; num_branches <= MAX_BRANCHES; num_branches *= 2) {
        uint64_t start = __rdtscp(&temp);
        for (int repeat = 0; repeat < 100000; repeat++) {
            for (int i = 0; i < num_branches; i++) {
                branches[i](); // call branch
                dummy++;       // prevent compiler optimization
            }
        }
        uint64_t end = __rdtscp(&temp);
        fprintf(log_fp, "Using %d branch targets → total cycles = %llu\n",
                num_branches, (unsigned long long)(end - start));
    }

    free(branches);
    fprintf(log_fp, "Observe when latency increases → approximate BTB capacity\n\n");
}


void btb_cap_test() 
{
    fprintf(log_fp, "=== better btb Capacity Test ===\n");
    empty_fn_t *branches = malloc(MAX_BRANCHES * sizeof(empty_fn_t));
    for (int i = 0; i < MAX_BRANCHES; i++) branches[i] = branch_stub;

    // Pointer-chase through branch functions to stress BTB
    unsigned int temp;
    unsigned int total = 0;
    int N = 100;
    for (int num_branches = 4; num_branches <= MAX_BRANCHES; num_branches *= 2) {
        uint64_t start = __rdtscp(&temp);
        for (int repeat = 0; repeat < N; repeat++) {
            uint64_t start = __rdtscp(&temp);
            for (int i = 0; i < num_branches; i++) {
                branches[i](temp); // call branch
            }
            uint64_t end = __rdtscp(&temp);
            total += (end-start);
        }
        total /= N;
        fprintf(log_fp, "Using %d branch targets → total cycles = %u\n",
                num_branches, total);
    }
}

// ------------------ BTB Associativity Test ------------------
void btb_associativity_test() {
    fprintf(log_fp, "=== BTB Associativity Test ===\n");

    branch_fn_t branches[64];
    for (int i = 0; i < 64; i++) branches[i] = branch_stub;

    volatile int dummy = 0;
    unsigned int temp;

    // Test sets by creating collisions in BTB (stride through addresses)
    for (int stride = 1; stride <= 16; stride *= 2) {
        uint64_t start = __rdtscp(&temp);
        for (int repeat = 0; repeat < 100000; repeat++) {
            for (int i = 0; i < 64; i += stride) {
                branches[i % 64](); dummy++;
            }
        }
        uint64_t end = __rdtscp(&temp);
        fprintf(log_fp, "Stride %d → total cycles = %llu\n", stride, (unsigned long long)(end - start));
    }

    fprintf(log_fp, "Observe latency jumps → estimate BTB associativity\n\n");
}

// ------------------ BTB Tag Bits Test ------------------
void btb_tag_bits_test() {
    fprintf(log_fp, "=== BTB Tag Bits Test ===\n");

    branch_fn_t branches[16];
    for (int i = 0; i < 16; i++) branches[i] = branch_stub;

    volatile int dummy = 0;
    unsigned int temp;

    // Vary lower address bits and check latency patterns
    for (int offset = 0; offset < 16; offset++) {
        uint64_t start = __rdtscp(&temp);
        for (int repeat = 0; repeat < 100000; repeat++) {
            for (int i = 0; i < 16; i++) {
                branches[(i + offset) % 16](); dummy++;
            }
        }
        uint64_t end = __rdtscp(&temp);
        fprintf(log_fp, "Offset %d → total cycles = %llu\n", offset, (unsigned long long)(end - start));
    }

    fprintf(log_fp, "Analyze which offsets affect latency → determine BTB tag bits\n\n");
}

// ------------------ Main ------------------
int main() {
    log_fp = fopen("results_btb.txt", "w");
    if (!log_fp) {
        fprintf(stderr, "Error opening results_btb.txt\n");
        return 1;
    }

    // btb_capacity_test();
    // btb_associativity_test();
    // btb_tag_bits_test();
    btb_cap_test();

    fclose(log_fp);
    printf("BTB results written to results_btb.txt\n");
    return 0;
}


//Maybe add bonus here?