#include <emmintrin.h>
#include <stdbool.h>
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

typedef struct page_block_t {
    struct page_block_t *prev;
    uint64_t data;
} block;

struct page_block_t *create_block(int num_pages, uint64_t page_size, struct page_block_t *prev, int val)
{
    struct page_block_t *p = malloc(sizeof(struct page_block_t));
    int rc = posix_memalign((void **) &p, page_size, page_size);
    p->data = (double) val;
    p->prev = prev;
    return p;
}

void tlb_levels(FILE *log, int num_pages, uint64_t page_size)
{
    // printf("=== %d pages @ size %ld ===\n", num_pages, page_size);
    printf("access_times\n");
    struct page_block_t *start = create_block(num_pages, page_size, NULL, 0);
    struct page_block_t **head = &start;
    struct page_block_t **prev = &start;

    // create linked list
    for (size_t i = 1; i < num_pages; i++) {
        /* allocate page-aligned blocks */
        struct page_block_t *p = create_block(num_pages, page_size, *prev, i);
        prev = &p;
    }

    // make sure we have reference to head node after running through the list
    head = prev;

    // warm up TLB
    while ((*prev)->prev) {
        prev = &(*prev)->prev;
    }

    volatile uint64_t b;
    prev = head;
    bool run = true;
    while (run) {
        _mm_clflush(*prev);
        uint64_t start_time = __rdtscp(&junk);
        b = (*prev)->data;
        uint64_t end_time = __rdtscp(&junk);

        if (!(*prev)->prev) {
            run = false;
        } else {
            prev = &(*prev)->prev;
        }
        printf("%lu\n",(end_time - start_time));
    }
}

void tlb_assoc(FILE *log)
{

}


void test_tlb_levels()
{
    // for (int i = 1024; i < 16384; i *= 2) {
    //     int page_size = FOUR_KB;
    //     char name[20];
    //     // snprintf(name, 15, "%d_%s_tlb.csv", i, page_size == FOUR_KB ? "4KB" : "2MB");
    //     // FILE *log = fopen(name, "w");
    //     // tlb_levels(NULL, i, page_size);
    //     // fclose(log);
    // }
     tlb_levels(NULL, 409, FOUR_KB);
}

int main(void)
{
    test_tlb_levels();
    return 0;  
}