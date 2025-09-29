// tlb.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>   // mlock
#include <inttypes.h>

static inline void cpuid_serialize(void) {
    /* serialize using cpuid (eax=0) */
    uint32_t eax, ebx, ecx, edx;
    eax = 0;
    __asm__ volatile ("cpuid"
                      : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                      : "a"(eax)
                      : "memory");
}

static inline uint64_t rdtsc(void) {
    uint32_t hi, lo;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t rdtscp_serialized(void) {
    uint32_t aux, hi, lo;
    __asm__ volatile ("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}

/* Build a cyclic pointer-chase in the first pointer-sized bytes of each page.
 * Return avg cycles per pointer-dereference over 'iterations' dereferences.
 */
uint64_t tlb_test(size_t num_pages, size_t page_size, size_t iterations) {
    /* allocate array of page pointers */
    void **pages = malloc(num_pages * sizeof(void *));
    if (!pages) {
        perror("malloc pages");
        exit(1);
    }

    /* allocate each page */
    for (size_t i = 0; i < num_pages; ++i) {
        /* aligned allocation of a whole page */
        void *p;
        int rc = posix_memalign(&p, page_size, page_size);
        if (rc != 0) {
            fprintf(stderr, "posix_memalign failed for page %zu\n", i);
            exit(1);
        }
        /* optionally lock into RAM to avoid page faults / swapping */
        if (mlock(p, page_size) != 0) {
            /* non-fatal: just warn */
            /* perror("mlock"); */
        }
        pages[i] = p;
    }

    /* write the pointer-chase: store pointer to next page at start of each page */
    for (size_t i = 0; i < num_pages; ++i) {
        size_t next = (i + 1) % num_pages;
        *(uintptr_t *)pages[i] = (uintptr_t)pages[next];
    }

    /* warm up: touch each page to ensure mapping and TLB fill */
    for (size_t i = 0; i < num_pages; ++i) {
        volatile char tmp = *((volatile char *)pages[i]);
        (void)tmp;
    }

    /* pointer-chase many times. keep a volatile accumulator so the compiler can't optimize */
    volatile uintptr_t acc = 0;
    void *p = pages[0];

    /* serialize, start */
    cpuid_serialize();
    uint64_t t0 = rdtsc();

    for (size_t it = 0; it < iterations; ++it) {
        p = (void *)*(uintptr_t *)p;  /* follow pointer */
        acc += (uintptr_t)p;         /* use value so not optimized away */
    }

    /* end */
    uint64_t t1 = rdtscp_serialized();
    cpuid_serialize();

    uint64_t total_cycles = (t1 - t0);

    /* free pages */
    for (size_t i = 0; i < num_pages; ++i) {
        munlock(pages[i], page_size);
        free(pages[i]);
    }
    free(pages);

    /* print acc to stdout to avoid optimizing out (compiler still sees volatile) */
    if (acc == 0xdeadbeef) printf("impossible\n");

    /* return average cycles per dereference */
    return total_cycles / iterations;
}

int main(void) {
    FILE *log_fp = fopen("results_tlb.txt", "w");
    if (!log_fp) {
        perror("fopen");
        return 1;
    }

    /* You can tweak these to stress different TLBs */
    size_t iterations = 2000000;    /* 2M dereferences */
    size_t page_size_4k = 4 * 1024;
    size_t page_size_2m = 2 * 1024 * 1024;

    /* Choose a range of pages: try enough pages to overflow L1/L2 TLBs */
    size_t num_pages_4k = 65536 / 16; /* ~4096 pages → ~16 MB (you can increase) */
    /* For a more aggressive test: num_pages_4k = 16384; */

    /* For 2MB pages fewer pages (since each page is large) */
    size_t num_pages_2m = 2048 / 16; /* adjust to available RAM; this is example */

    /* If you want strict sizes, use absolute e.g. num_pages_4k = 16384 (64MB) */
    num_pages_4k = 16384;  /* 64 MB of 4 KiB pages */
    num_pages_2m = 512;    /* 1 GB of 2 MiB pages (ensure you have RAM) */

    fprintf(log_fp, "=== TLB Microbenchmark (debug) ===\n");
    fprintf(log_fp, "iterations=%zu\n", iterations);

    uint64_t avg4 = tlb_test(num_pages_4k, page_size_4k, iterations);
    fprintf(log_fp, "4 KiB pages (%zu pages): avg cycles/access = %" PRIu64 "\n",
            num_pages_4k, avg4);

    uint64_t avg2 = tlb_test(num_pages_2m, page_size_2m, iterations);
    fprintf(log_fp, "2 MiB pages (%zu pages): avg cycles/access = %" PRIu64 "\n",
            num_pages_2m, avg2);

    fclose(log_fp);
    printf("Done — results in results_tlb.txt\n");
    return 0;
}
