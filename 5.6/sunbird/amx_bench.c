#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>    // AMX intrinsics
#include <x86intrin.h>    // __rdtscp
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

int main(int argc, char **argv) {
    if(argc!=6){
        fprintf(stderr,"Usage: %s <INT8|BF16> M N K zero_frac\n",argv[0]);
        return 1;
    }
    char *type=argv[1];
    int M=atoi(argv[2]), N=atoi(argv[3]), K=atoi(argv[4]);
    float zf=strtof(argv[5],NULL);

    pin_thread_to_core(0);
    uint64_t cycles=0;

    if(strcmp(type,"INT8")==0){
        cycles = measure_int8(M,N,K,zf);
    } else if(strcmp(type,"BF16")==0){
        cycles = measure_bf16(M,N,K,zf);
    } else {
        fprintf(stderr,"Unknown type '%s'\n",type);
        return 1;
    }

    printf("%lu\n",(unsigned long)cycles);
    return 0;
}