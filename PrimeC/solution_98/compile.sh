#!/bin/sh
#CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops -DCOMPILE_64_BIT"
#CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops -DCOMPILE_8_BIT" 
CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops"
#CC="gcc -pedantic -Ofast -march=native -mtune=native -funsafe-math-optimizations -funroll-all-loops"
for x in primes_words7 primes_words6 primes_words5 primes_words3 primes_words primes_words2 primes_bit2 primes_bit primes_char primes_int ; do
    $CC -o $x $x.c -lm
done
