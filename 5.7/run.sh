#!/bin/bash
name=$(echo $(hostname) | awk -F'.' '{print $1}')
gcc -O0 -fno-tree-vectorize -g tlb_bench.c -o tlb_bench
./tlb_bench > $name/$name.csv