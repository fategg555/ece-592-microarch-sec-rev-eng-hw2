#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <x86intrin.h>
#define JMP_LABEL(x) "jmp label_" #x "\n\t" "nop\n\t" "nop\n\t" "label_"#x": "
#define JMP(x) "jmp label_" #x "\n\t"

unsigned int aux;

int main() {
    volatile uint64_t start, end;
    bool run = false;
    // warm up BTB, store initial entries
    __asm__ volatile (
        <label>
        :::
    );

    if (!run) {
        run = true;
        // actually do the timing op
        start = __rdtscp(&aux);
        __asm__ volatile (
            JMP(1)
            :::
        );
    } else {
        end = __rdtscp(&aux);
    }
    uint64_t volatile total = end - start;
    printf("%lu\n", total);
    
    return 0;
}