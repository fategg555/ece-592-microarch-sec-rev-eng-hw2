#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>

#define N 10000000

int arr[N];
int temp;

void streaming_access() {
    for (int i = 0; i < N; i++) {
        arr[i] = arr[i] * 2;  // Sequential access
    }
}

void randomized_access() {
    for (int i = 0; i < N; i++) {
        int idx = rand() % N;
        arr[idx] = arr[idx] * 2;  // Random access
    }
}

int main() {
    int begin = __rdtscp(&temp);
    streaming_access();
    int finish = __rdtscp(&temp);
    int begin_random = __rdtscp(&temp);
    randomized_access();
    int finish_random = __rdtscp(&temp);
    printf("Streaming access time: %d cycles\n", (finish-begin)); 
    printf("Randomized access time: %d cycles\n", (finish_random - begin_random));
    return 0;
}
