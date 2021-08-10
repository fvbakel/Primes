#!/bin/sh
#for x in primes_words primes_words10 primes_words09 primes_bit primes_bit2 primes_bit3 primes_bit4 primes_bit5 primes_bit6; do
#    ./$x
#done

x=primes_bit7
for store_mode in 0 1 2 3 4; do
    for crossout_mode in 0 1 2 3 4; do
        ./${x}_${store_mode}___${crossout_mode}
    done
done