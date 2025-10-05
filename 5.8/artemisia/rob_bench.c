#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>

volatile uint64_t regfile_sink = 0;

// ------------------ ROB Estimation ------------------
uint64_t rob_estimation(int chain_length) {
    unsigned int tmp;
    uint64_t start, end;
    volatile int x = 1;

    start = __rdtscp(&tmp);
    for (int i = 0; i < chain_length; i++) {
        x = x + 1; x = x + 1; x = x + 1; x = x + 1;
    }
    end = __rdtscp(&tmp);

    return end - start;
}

void rob_test() {
    printf("=== Reorder Buffer (ROB) Estimation ===\n");
    int chain_lengths[] = {16, 32, 64, 128, 256, 512, 1024};
    int n = sizeof(chain_lengths)/sizeof(chain_lengths[0]);
    for (int i=0; i<n; i++) {
        uint64_t cycles = rob_estimation(chain_lengths[i]);
        printf("Chain length %d: %llu cycles\n", chain_lengths[i], (unsigned long long)cycles);
    }
    printf("Observe where cycles per op plateau → approximate ROB size\n\n");
}

// ------------------ Register File Pressure ------------------
uint64_t regfile_pressure_test(int num_regs) {
    unsigned int tmp;
    uint64_t start, end;

    // Declare up to 64 live registers as scalars (adjustable)
    volatile uint64_t r[64];
    for (int i = 0; i < 64; i++) r[i] = 0;

    const int ITER = 200000;
    start = __rdtscp(&tmp);

    for (int i = 0; i < ITER; i++) {
        switch(num_regs) {
            case 64: r[63]++;
            case 32: r[31]++; r[32]++; r[33]++; r[34]++; r[35]++; r[36]++; r[37]++; r[38]++; r[39]++;
                     r[40]++; r[41]++; r[42]++; r[43]++; r[44]++; r[45]++; r[46]++; r[47]++;
                     r[48]++; r[49]++; r[50]++; r[51]++; r[52]++; r[53]++; r[54]++; r[55]++;
                     r[56]++; r[57]++; r[58]++; r[59]++; r[60]++; r[61]++; r[62]++; r[63]++;
            case 16: r[15]++; r[16]++; r[17]++; r[18]++; r[19]++; r[20]++; r[21]++; r[22]++;
            case 8:  r[7]++; r[0]++; // always increment at least some
        }
    }

    end = __rdtscp(&tmp);

    // Sink sum to prevent compiler optimization
    for (int i=0;i<64;i++) regfile_sink += r[i];

    return end - start;
}

void regfile_test() {
    printf("=== Register File Pressure Test ===\n");
    int reg_counts[] = {8, 16, 32, 64};
    int n = sizeof(reg_counts)/sizeof(reg_counts[0]);
    for (int i=0;i<n;i++) {
        uint64_t cycles = regfile_pressure_test(reg_counts[i]);
        printf("%d live registers: %llu cycles\n", reg_counts[i], (unsigned long long)cycles);
    }
    printf("Latency jumps indicate physical register file saturation\n\n");
}

// ------------------ Main ------------------
int main() {
    FILE *log_fp = fopen("results_rob.txt", "w");
    if (!log_fp) {
        fprintf(stderr, "Error opening results_rob.txt\n");
        return 1;
    }

    // ROB
    fprintf(log_fp, "=== Reorder Buffer (ROB) Estimation ===\n");
    int chain_lengths[] = {16, 32, 64, 128, 256, 512, 1024};
    for (int i = 0; i < sizeof(chain_lengths)/sizeof(chain_lengths[0]); i++) {
        uint64_t cycles = rob_estimation(chain_lengths[i]);
        fprintf(log_fp, "Chain length %d: %llu cycles\n", chain_lengths[i], (unsigned long long)cycles);
    }
    fprintf(log_fp, "Observe where cycles per op plateau → approximate ROB size\n\n");

    // Register File
    fprintf(log_fp, "=== Register File Pressure Test ===\n");
    int reg_counts[] = {8, 16, 32, 64};
    for (int i = 0; i < sizeof(reg_counts)/sizeof(reg_counts[0]); i++) {
        uint64_t cycles = regfile_pressure_test(reg_counts[i]);
        fprintf(log_fp, "%d live registers: %llu cycles\n", reg_counts[i], (unsigned long long)cycles);
    }
    fprintf(log_fp, "Latency jumps indicate physical register file saturation\n\n");

    fclose(log_fp);
    printf("All results written to results_rob.txt\n");
    return 0;
}
