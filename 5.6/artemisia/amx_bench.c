#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include <x86intrin.h>
#include <pthread.h>
#include <sched.h>

#define REPETITIONS 1000

typedef struct {
    uint8_t   palette_id;
    uint8_t   start_row;
    uint8_t   reserved_0[14];
    uint16_t  colsb[16];
    uint8_t   rows[16];
} __tilecfg;

// ------------------------
// Helpers
// ------------------------
static void pin_thread_to_core(int core_id) {
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(core_id, &cs);
    pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
}

static void fill_int8(int8_t *B, int R, int C, float zf) {
    for(int i=0;i<R*C;i++)
        B[i] = ((rand()/(float)RAND_MAX) < zf) ? 0 : (rand()%127+1);
}

static void fill_bf16(uint16_t *B, int R, int C, float zf) {
    for(int i=0;i<R*C;i++){
        if((rand()/(float)RAND_MAX) < zf){
            B[i]=0;
        } else {
            float v = (rand()/(float)RAND_MAX)*2.0f-1.0f;
            uint32_t bits;
            memcpy(&bits,&v,sizeof bits);
            B[i] = (uint16_t)(bits>>16);
        }
    }
}

// ------------------------
// Measure functions
// ------------------------
static uint64_t measure_int8(int M,int N,int K,float zf){
    int8_t  *A=aligned_alloc(64,M*K);
    int8_t  *B=aligned_alloc(64,K*N);
    int32_t *C=aligned_alloc(64,M*N*sizeof(int32_t));
    fill_int8(A,M,K,zf);
    fill_int8(B,K,N,zf);
    memset(C,0,M*N*sizeof(int32_t));

    __tilecfg cfg={0};
    cfg.palette_id=1; cfg.start_row=0;
    cfg.rows[0]=M; cfg.colsb[0]=K*sizeof(int8_t);
    cfg.rows[1]=K; cfg.colsb[1]=N*sizeof(int8_t);
    cfg.rows[2]=M; cfg.colsb[2]=N*sizeof(int32_t);
    _tile_loadconfig(&cfg);

    uint64_t sum=0; unsigned int aux;
    for(int i=0;i<REPETITIONS;i++){
        uint64_t s=__rdtscp(&aux);
        _tile_zero(2);
        _tile_loadd(0,A,K*sizeof(int8_t));
        _tile_loadd(1,B,N*sizeof(int8_t));
        _tile_dpbssd(2,0,1);
        _tile_stored(2,C,N*sizeof(int32_t));
        uint64_t e=__rdtscp(&aux);
        sum += e-s;
    }
    _tile_release();
    free(A); free(B); free(C);
    return sum/REPETITIONS;
}

static uint64_t measure_bf16(int M,int N,int K,float zf){
    uint16_t *A=aligned_alloc(64,M*K*sizeof(uint16_t));
    uint16_t *B=aligned_alloc(64,K*N*sizeof(uint16_t));
    float    *C=aligned_alloc(64,M*N*sizeof(float));
    fill_bf16(A,M,K,zf);
    fill_bf16(B,K,N,zf);
    memset(C,0,M*N*sizeof(float));

    __tilecfg cfg={0};
    cfg.palette_id=1; cfg.start_row=0;
    cfg.rows[0]=M; cfg.colsb[0]=K*sizeof(uint16_t);
    cfg.rows[1]=K; cfg.colsb[1]=N*sizeof(uint16_t);
    cfg.rows[2]=M; cfg.colsb[2]=N*sizeof(float);
    _tile_loadconfig(&cfg);

    uint64_t sum=0; unsigned int aux;
    for(int i=0;i<REPETITIONS;i++){
        uint64_t s=__rdtscp(&aux);
        _tile_zero(2);
        _tile_loadd(0,A,K*sizeof(uint16_t));
        _tile_loadd(1,B,N*sizeof(uint16_t));
        _tile_dpbf16ps(2,0,1);
        _tile_stored(2,C,N*sizeof(float));
        uint64_t e=__rdtscp(&aux);
        sum += e-s;
    }
    _tile_release();
    free(A); free(B); free(C);
    return sum/REPETITIONS;
}

// ------------------------
// Main: automatic sweep
// ------------------------
int main() {
    pin_thread_to_core(0);

    int M=64, N=64, K=64;                  // tile size
    float zero_fracs[] = {0.0,0.1,0.2,0.3,0.5,0.7,0.9,1.0};
    int num_zf = sizeof(zero_fracs)/sizeof(zero_fracs[0]);

    // Open CSV file
    FILE *f = fopen("amx_zero_skip.csv","w");
    if(!f) { perror("fopen"); return 1; }
    fprintf(f,"Type,ZeroFraction,Cycles\n");

    // Sweep INT8
    for(int i=0;i<num_zf;i++){
        float zf = zero_fracs[i];
        uint64_t cycles = measure_int8(M,N,K,zf);
        printf("INT8,%.1f, %lu cycles\n", zf, (unsigned long)cycles);
        fprintf(f,"INT8,%.1f,%lu\n", zf, (unsigned long)cycles);
    }

    // Sweep BF16
    for(int i=0;i<num_zf;i++){
        float zf = zero_fracs[i];
        uint64_t cycles = measure_bf16(M,N,K,zf);
        printf("BF16,%.1f, %lu cycles\n", zf, (unsigned long)cycles);
        fprintf(f,"BF16,%.1f,%lu\n", zf, (unsigned long)cycles);
    }

    fclose(f);
    return 0;
}
