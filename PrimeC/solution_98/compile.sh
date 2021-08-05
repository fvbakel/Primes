#!/bin/sh
#CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops -DCOMPILE_64_BIT"
#CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops -DCOMPILE_8_BIT" 
CC="gcc -Ofast -march=native -mtune=native -funroll-all-loops"
#CC="gcc -pedantic -Ofast -march=native -mtune=native -funsafe-math-optimizations -funroll-all-loops"
for x in test_loop get_cache_info primes_words10 primes_words09 primes_words08 primes_words07 primes_words06 primes_words05 primes_words03 primes_words primes_words02 primes_bit6 primes_bit5 primes_bit4 primes_bit3 primes_bit2 primes_bit primes_char primes_int ; do
    $CC -o $x $x.c -lm
done
