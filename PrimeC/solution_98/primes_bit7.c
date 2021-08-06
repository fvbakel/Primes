#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#define BITZERO 0U

#define ON BITZERO

#define TYPE uint32_t
#define MASK 31U 
#define SHIFT 5U
#define MEMSHIFT 6U
#define SHIFTSIZE 2U 

// the const below is to reduce the multiplications
const unsigned int BITS_IN_WORD=8*sizeof(TYPE);

// the constant below is a cache of all the possible bit masks
const TYPE OFFSET_MASK[] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648};

struct sieve_state {
    TYPE *bit_array;
    unsigned int limit;
    unsigned int size;
    unsigned int nr_of_words;
};

unsigned int NR_OF_WORDS;
static inline struct sieve_state *create_sieve(int limit) {
    struct sieve_state *sieve_state=malloc(sizeof *sieve_state);
    sieve_state->size = limit >>1;
    sieve_state->nr_of_words=(limit >> MEMSHIFT) + 5;
    NR_OF_WORDS = sieve_state->nr_of_words;
    sieve_state->bit_array=calloc(sieve_state->nr_of_words,sizeof(TYPE));
    sieve_state->limit=limit;
    return sieve_state;
}

static inline void delete_sieve(struct sieve_state *sieve_state) {
    free(sieve_state->bit_array);
    free(sieve_state);
    }

static inline unsigned int get_first_index(unsigned int word_index) {
    return word_index * BITS_IN_WORD;
}

static inline unsigned int get_last_index(unsigned int word_index) {
    return ((word_index +1) * BITS_IN_WORD ) - 1;
}
static inline unsigned int index_to_natural(unsigned int bit_index) {
    return (bit_index << 1) +1 ;
}
static inline unsigned int natural_to_index(unsigned int natural) {
    return (natural - 1) >> 1 ;
}

#define NORMAL      1
#define STRIPED     2
#define STRIPED_2   3
#define STRIPED_3   4
#define MODULO      5
#define STORE_MODE NORMAL

#if STORE_MODE == STRIPED
static inline unsigned int index_to_word(unsigned int bit_index) {
    return (bit_index >> SHIFT) + (bit_index & 1);
}
static inline unsigned int index_to_offset(unsigned int bit_index) {
    return (bit_index & 63U) >> 1;
}
#elif STORE_MODE == STRIPED_2
static inline unsigned int index_to_word(unsigned int bit_index) {
    return (bit_index >> SHIFT) + (bit_index & 3);
}
static inline unsigned int index_to_offset(unsigned int bit_index) {
    return (bit_index & 127U) >> 2;
}
#elif STORE_MODE == MODULO
static inline unsigned int index_to_word(unsigned int bit_index) {
    return (bit_index / BITS_IN_WORD);
}
static inline unsigned int index_to_offset(unsigned int bit_index) {
    return (bit_index % BITS_IN_WORD);
}
#elif STORE_MODE == STRIPED_3
static inline unsigned int index_to_word(unsigned int bit_index) {
    return (bit_index % NR_OF_WORDS);
}
static inline unsigned int index_to_offset(unsigned int bit_index) {
    return (bit_index / NR_OF_WORDS);
}
#else

static inline unsigned int index_to_word(unsigned int bit_index) {
    return bit_index >> SHIFT;
}
static inline unsigned int index_to_offset(unsigned int bit_index) {
    return bit_index & MASK;
}
#endif

static inline void setBit(struct sieve_state *sieve_state,unsigned int index) {
    sieve_state->bit_array[index_to_word(index)] |=  OFFSET_MASK[index_to_offset(index)];
}

static inline TYPE getBit (struct sieve_state *sieve_state,unsigned int index) {
    return sieve_state->bit_array[index_to_word(index)] & OFFSET_MASK[index_to_offset(index)];
}

static inline void bit_cross_out_simple(
    struct sieve_state *sieve_state,
    unsigned int prime
) {
    unsigned int start_index = ((prime * prime)>>1U);
    for ( unsigned int i = start_index; i <sieve_state->size ; i +=prime ) {
        setBit(sieve_state,i);
    }
}

static inline void bit_cross_out_reverse(
    struct sieve_state *sieve_state,
    unsigned int prime
) {
    unsigned int end_index = ((prime * prime)>>1U);
    unsigned int last_prime= sieve_state->limit - (sieve_state->limit % prime);
    if ( !(last_prime & 1) == 1) {
        last_prime -= prime;
    }

    unsigned int start_index = last_prime /2;
    for ( unsigned int i = start_index; i >= end_index ; i -=prime ) {
        setBit(sieve_state,i);
    }
}

static inline void bit_cross_out_by_4(
    struct sieve_state *sieve_state,
    unsigned int prime
) {
    unsigned int current_index = ((prime * prime)>>1U);
    unsigned int prime_2 = prime * 2;
    unsigned int prime_3 = prime * 3;
    unsigned int prime_4 = prime * 4;

    unsigned int save_len = 0;
    if (sieve_state->size > prime_3) {
        save_len = sieve_state->size - prime_3;
    }

    while (current_index < save_len) {
        setBit(sieve_state,current_index);
        setBit(sieve_state,current_index + prime);
        setBit(sieve_state,current_index + prime_2);
        setBit(sieve_state,current_index + prime_3);
        current_index += prime_4;
    }

    while (current_index <sieve_state->size) {
        setBit(sieve_state,current_index);
        current_index += prime;
    }
}

static inline void bit_cross_out_by_4_reverse(
    struct sieve_state *sieve_state,
    unsigned int prime
) {
    unsigned int prime_2 = prime * 2;
    unsigned int prime_3 = prime * 3;
    unsigned int prime_4 = prime * 4;

    unsigned int end_index = ((prime * prime)>>1U);
    unsigned int last_prime= sieve_state->limit - (sieve_state->limit % prime);
    if ( !(last_prime & 1) == 1) {
        last_prime -= prime;
    }
    unsigned int current_index = last_prime /2;

    unsigned int save_index = end_index + prime_3;
    while (current_index > save_index) {
        setBit(sieve_state,current_index);
        setBit(sieve_state,current_index - prime);
        setBit(sieve_state,current_index - prime_2);
        setBit(sieve_state,current_index - prime_3);
        current_index -= prime_4;
    }

    while (current_index >= end_index) {
        setBit(sieve_state,current_index);
        current_index -= prime;
    }
}


void run_sieve(struct sieve_state *sieve_state) {
    unsigned int factor_index = 1U;
    unsigned int prime;
    unsigned int q=(unsigned int)sqrt(sieve_state->limit);
    unsigned int q_index=q>>1U;
    int forward = 1;
    
    while (factor_index < q_index) {
        if ( getBit(sieve_state,factor_index) == ON ) {
            prime = (factor_index << 1U)+1U;
            if (forward) {
                bit_cross_out_by_4(sieve_state,prime);
                forward = 0;  
            } else {
                bit_cross_out_by_4_reverse(sieve_state,prime);
                forward = 1;
            }
        }
        factor_index++;
    }
}

void print_primes (struct sieve_state *sieve_state) {
    unsigned int max_index=(sieve_state->limit>>1U) - ((sieve_state->limit & 1U) == 1U ?  0U:1U);
    printf("%i,",2);
    for (unsigned int i = 1; i <= max_index; i++) {
        if (getBit(sieve_state,i) == ON ) {
            printf("%i,",(i<<1U) +1U);    
        }
    }
    printf("\n");
}

unsigned int count_primes (struct sieve_state *sieve_state) {
    unsigned int count = 1;
    unsigned int max_index=(sieve_state->limit>>1U) - ((sieve_state->limit & 1U) == 1U ?  0U:1U);

    for (unsigned int i = 1; i <=max_index; i++) {
        if (getBit(sieve_state,i) == ON ) {
            count++;   
        }
    }
    return count;
}

char* validate_result(unsigned int limit, int count) {
    int valid_primes;

    switch(limit) {
    case 10:
        valid_primes=4;
        break;
    case 100:
        valid_primes=25;
        break;
    case 1000:
        valid_primes=168;
        break;
    case 10000:
        valid_primes=1229;
        break;
    case 100000:
        valid_primes=9592;
        break;
    case 1000000:
        valid_primes=78498;
        break;
    case 10000000:
        valid_primes=664579;
        break;
    case 100000000:
        valid_primes=5761455;
        break;
    case 1000000000:
        valid_primes=50847534;
        break;
    default:
        valid_primes=-1;
    }

    if (valid_primes == -1 ) {
        return "Unknown";
    } else if (valid_primes == count) {
        return "True";
    } 
    return "False";
}

void print_results (    
        struct          sieve_state *sieve_state,
        unsigned int    show_result,
        double          duration,
        int             passes) 
{
    int     count = count_primes(sieve_state);
    char*   valid = validate_result(sieve_state->limit,count);

    if (show_result == 1U) {
        print_primes(sieve_state);
    }

    printf("Passes: %d, Time: %f, Avg: %f, Limit: %d, Count: %d, Valid: %s\n",
            passes,
            duration,
            (duration / passes),
            sieve_state->limit,
            count, 
            valid 
        );

	printf("\n");
	printf("fvbakel_Cbit7;%d;%f;1;algorithm=base,faithful=yes,bits=%lu\n", passes, duration,1LU);
}

double run_timed_sieve(  
    unsigned int        limit,
    double              maxtime,
    unsigned int        show_result,
    unsigned int        print_sumary
 ) {
    
    struct sieve_state *sieve_state;
    struct timespec     start,now;
    double              duration;
    int                 passes      = 0;

    clock_gettime(CLOCK_MONOTONIC,&start);

    while (1) {
        sieve_state=create_sieve(limit);
        run_sieve(sieve_state);
        passes++;
        clock_gettime(CLOCK_MONOTONIC,&now);
        duration=now.tv_sec+now.tv_nsec*1e-9-start.tv_sec-start.tv_nsec*1e-9;

        if (duration>maxtime) {
            if (print_sumary) {
                print_results ( sieve_state, show_result, duration, passes);
            }
            break;
        }

        delete_sieve(sieve_state);
    }

    return passes/duration;
}

int main(int argc, char **argv) {
    unsigned int        limit       = 1000000;
    double              maxtime     = 5.;
    unsigned int        show_result = 0;
    
    double              speed;

//    set_word_block_size(limit);
    speed = run_timed_sieve(limit,maxtime,show_result,1);

    return 0;
}