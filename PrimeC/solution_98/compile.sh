#!/bin/sh
#CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops -DCOMPILE_64_BIT"
CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops" 
for x in primes_bit2 primes_bit primes_char primes_int primes_words; do
    $CC -o $x $x.c -lm
done
