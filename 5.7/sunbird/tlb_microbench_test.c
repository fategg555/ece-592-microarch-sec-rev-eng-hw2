#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <immintrin.h>
#include <time.h>

FILE *log_fp;

// ------------------ Portable TLB Pointer-Chase ------------------
uint64_t tlb_test(size_t num_pages, size_t page_size) {
    unsigned int temp;
    uint64_t start, end;
    char **ptrs = malloc(num_pages * sizeof(char*));

    // Allocate "pages" using aligned_alloc
    for (size_t i = 0; i < num_pages; i++)
        ptrs[i] = aligned_alloc(64, page_size);

    // Build pointer-chase
    for (size_t i = 0; i < num_pages; i++) {
        size_t next = (i + 1) % num_pages;
        *(uintptr_t*)ptrs[i] = (uintptr_t)ptrs[next];
    }

    // Chase pointers to measure access latency
    volatile char *p = (char*)ptrs[0];
    start = __rdtscp(&temp);
    for (size_t i = 0; i < num_pages; i++)
        p = (char*)(*(uintptr_t*)p);
    end = __rdtscp(&temp);

    // Free memory
    for (size_t i = 0; i < num_pages; i++) free(ptrs[i]);
    free(ptrs);

    return (end - start) / num_pages; // avg cycles per access
}

// ------------------ TLB Microbenchmark ------------------
void tlb_microbench() {
    fprintf(log_fp, "=== TLB Microbenchmark ===\n");

    size_t num_pages_4k = 1024;   // ~4 MB
    size_t num_pages_2m = 128;    // ~256 MB

    uint64_t avg4k = tlb_test(num_pages_4k, 4*1024);
    uint64_t avg2m = tlb_test(num_pages_2m, 2*1024*1024);

    fprintf(log_fp, "4 KiB pages: avg TLB cycles/access = %llu\n", (unsigned long long)avg4k);
    fprintf(log_fp, "2 MiB pages: avg TLB cycles/access = %llu\n", (unsigned long long)avg2m);

    fprintf(log_fp, "Compare → smaller pages stress L1/L2 TLB; large pages reduce TLB misses\n\n");
}

// ------------------ Main ------------------
int main() {
    log_fp = fopen("results_tlb.txt", "w");
    if (!log_fp) { 
        fprintf(stderr, "Error opening results_tlb.txt\n"); 
        return 1; 
    }

    tlb_microbench();

    fclose(log_fp);
    printf("TLB results written to results_tlb.txt\n");
    return 0;
}
