#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>

FILE *log_fp;

// ------------------ Max Instructions per Cycle ------------------
uint64_t max_ipc_test(int num_ops) {
    unsigned int temp;
    uint64_t start, end;

    // Independent operations to stress fetch/decode width
    volatile int a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8;
    start = __rdtscp(&temp);
    for (int i = 0; i < num_ops; i++) {
        a++; b++; c++; d++; e++; f++; g++; h++;
    }
    end = __rdtscp(&temp);

    return end - start;
}

// ------------------ Pipeline Depth Estimation ------------------
uint64_t pipeline_depth_test(int chain_length) {
    unsigned int temp;
    uint64_t start, end;

    // Dependent chain: each op depends on previous
    volatile int x = 1;
    start = __rdtscp(&temp);
    for (int i = 0; i < chain_length; i++) {
        x = x + 1;
    }
    end = __rdtscp(&temp);

    return end - start;
}

// ------------------ Superscalar Microbenchmark ------------------
void superscalar_microbench() {
    fprintf(log_fp, "=== Superscalar Dimensions ===\n");

    int num_ops = 10000000;
    uint64_t cycles_indep = max_ipc_test(num_ops);
    double ipc = (double)(num_ops*8) / cycles_indep; // 8 independent ops per loop iteration
    fprintf(log_fp, "Max instructions per cycle (approx) = %.2f\n", ipc);

    // Pipeline depth estimate: dependent chain
    int chain_lengths[] = {16, 32, 64, 128, 256, 512};
    for (int i = 0; i < sizeof(chain_lengths)/sizeof(chain_lengths[0]); i++) {
        uint64_t cycles = pipeline_depth_test(chain_lengths[i]);
        double latency_per_op = (double)cycles / chain_lengths[i];
        fprintf(log_fp, "Dependent chain length %d → avg cycles/op = %.2f\n",
                chain_lengths[i], latency_per_op);
    }

    fprintf(log_fp, "Observe latency per op → pipeline depth estimate\n\n");
}

// ------------------ Main ------------------
int main() {
    log_fp = fopen("results_superscalar.txt", "w");
    if (!log_fp) {
        fprintf(stderr, "Error opening results_superscalar.txt\n");
        return 1;
    }

    superscalar_microbench();

    fclose(log_fp);
    printf("Superscalar results written to results_superscalar.txt\n");
    return 0;
}
