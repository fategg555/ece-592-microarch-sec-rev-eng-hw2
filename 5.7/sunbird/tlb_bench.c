#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <x86intrin.h>
#include <sys/mman.h>   // mlock
#include <inttypes.h>

#define FOUR_KB 4096
#define TWO_MB 2 * 1024 * 1024

unsigned int junk;

void tlb_levels(FILE *log, int num_pages, uint64_t page_size)
{
    printf("=== %d pages @ size %ld ===\n", num_pages, page_size);
    void **pages = malloc(num_pages * sizeof(void *));
    for (size_t i = 0; i < num_pages; i++) {
        /* aligned allocation of a whole page */
        int *p;
        int rc = posix_memalign((void **) &p, page_size, page_size);
        *p = i;
        pages[i] = p;
    }

    // warm up tlb
    for (size_t i = 0; i < num_pages; i++) {
        *((int* ) pages[i]) ^= 1; // do some simple op on it to touch val and put into TLB
    } 
    
    // actually time access in TLB
    for (int i = 0; i < num_pages - 1; i++) {
        uint64_t start = __rdtscp(&junk);
        *((int* ) pages[i]) ^= *((int* ) pages[i + 1]); // access current page and next page
        uint64_t end = __rdtscp(&junk);
        // printf("value of page[%d]: %d\n", i, *((int* ) pages[i]));
        fprintf(log, "%u, %u\n", i, (end - start));
    }
    printf("\n");
}

void tlb_assoc(FILE *log)
{

}


void test_tlb_levels()
{
    for (int i = 1024; i < 16384; i *= 2) {
        int page_size = FOUR_KB;
        char name[20];
        snprintf(name, 15, "%d_%s_tlb.csv", i, page_size == FOUR_KB ? "4KB" : "2MB");
        FILE *log = fopen(name, "w");
        tlb_levels(log, i, page_size);
        fclose(log);
    }
}

int main(void)
{
    test_tlb_levels();
    return 0;  
}