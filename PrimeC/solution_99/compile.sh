#!/bin/sh
#CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops -DCOMPILE_64_BIT"
CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops"
PAR="-fopenmp"
for x in sieve_1of2; do
    $CC -o $x $x.c -lm
done
