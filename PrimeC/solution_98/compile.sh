#!/bin/sh
#CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops -DCOMPILE_64_BIT"
#CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops -DCOMPILE_8_BIT" 
CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops"
#CC="gcc -pedantic -Ofast -march=native -mtune=native -funsafe-math-optimizations -funroll-all-loops"

#for x in primes_words primes_words10 primes_words09 primes_bit primes_bit2 primes_bit3 primes_bit4 primes_bit5 primes_bit6 primes_bit7; do

x=primes_bit7
for store_mode in 0 1 2 3 4; do
    for crossout_mode in 0 1 2 3 4; do
        $CC -DSTORE_MODE=${store_mode} -DCROSSOUT_MODE=${crossout_mode} -o ${x}_${store_mode}___${crossout_mode} $x.c -lm
    done
done