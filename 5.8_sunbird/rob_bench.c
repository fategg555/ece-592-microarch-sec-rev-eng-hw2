#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <immintrin.h>
#include <time.h>

typedef struct {
    uint64_t rob_cycles;
    uint64_t regfile_cycles;
} Results;

Results results = {0};

FILE *log_fp;

// ------------------ ROB Estimation ------------------
uint64_t rob_estimation(int chain_length) {
    unsigned int temp;
    uint64_t start, end;
    volatile int x = 1;

    // Long dependency chain: each operation depends on the previous
    start = __rdtscp(&temp);
    for (int i = 0; i < chain_length; i++) {
        x = x + 1; x = x + 1; x = x + 1; x = x + 1;
    }
    end = __rdtscp(&temp);

    return end - start;
}

void rob_test() {
    fprintf(log_fp, "=== Reorder Buffer (ROB) Estimation ===\n");
    int chain_lengths[] = {16, 32, 64, 128, 256, 512, 1024};
    for (int i = 0; i < sizeof(chain_lengths)/sizeof(chain_lengths[0]); i++) {
        uint64_t cycles = rob_estimation(chain_lengths[i]);
        fprintf(log_fp, "Chain length %d: %llu cycles\n", chain_lengths[i], (unsigned long long)cycles);
    }
    fprintf(log_fp, "Observe where cycles per op plateau → approximate ROB size\n\n");
}

// ------------------ Register File Pressure ------------------
uint64_t regfile_pressure_test(int num_regs) {
    unsigned int temp;
    uint64_t start, end;

    // Live variables to saturate physical registers
    volatile int r[128] = {0}; // max array size; compiler will keep in registers
    start = __rdtscp(&temp);
    for (int i = 0; i < 1000000; i++) {
        for (int j = 0; j < num_regs; j++) {
            r[j] = r[j] + 1;
        }
    }
    end = __rdtscp(&temp);

    return end - start;
}

void regfile_test() {
    fprintf(log_fp, "=== Register File Pressure Test ===\n");
    int reg_counts[] = {8, 16, 32, 64, 128};
    for (int i = 0; i < sizeof(reg_counts)/sizeof(reg_counts[0]); i++) {
        uint64_t cycles = regfile_pressure_test(reg_counts[i]);
        fprintf(log_fp, "%d live registers: %llu cycles\n", reg_counts[i], (unsigned long long)cycles);
    }
    fprintf(log_fp, "Latency jumps indicate physical register file saturation\n\n");
}

// ------------------ Main ------------------
int main() {
    log_fp = fopen("results_rob.txt", "w");
    if (!log_fp) { 
        fprintf(stderr, "Error opening results_rob.txt\n"); 
        return 1; 
    }

    rob_test();
    regfile_test();

    fclose(log_fp);
    printf("All results written to results_rob.txt\n");
    return 0;
}
