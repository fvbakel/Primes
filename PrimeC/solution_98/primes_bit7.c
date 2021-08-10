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

#define NORMAL      0
#define STRIPED     1
#define STRIPED_2   2
#define STRIPED_3   3
#define MODULO      4
const char* STORE_MODES[] = {"NORMAL","STRIPED","STRIPED_2","STRIPED_3","MODULO"};
#ifndef STORE_MODE
    #define STORE_MODE NORMAL
#endif

#define BY_4 1
#define BACK_TO_FRONT_BY_4 2
#define BACK_TO_FRONT 3
#define BACK_TO_FRONT_BY_4_CACHED 4
const char* CROSSOUT_MODES[] = {"NORMAL","BY_4","BACK_TO_FRONT_BY_4","BACK_TO_FRONT","BACK_TO_FRONT_BY_4_CACHED"};
#ifndef CROSSOUT_MODE
    #define CROSSOUT_MODE BY_4
#endif



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

struct cache_elm {
    unsigned int prime;
    unsigned int prime_sqr_index;
    unsigned int prime_2;
    unsigned int prime_3;
    unsigned int prime_4;
    unsigned int save_len;
    unsigned int last_prime_index;
    unsigned int save_index;
};

struct cache_elm* CACHE;

void fill_cache(unsigned int limit) {
    unsigned int q=(unsigned int)sqrt(limit);
    unsigned int q_index=q/2;
    CACHE = (struct cache_elm*) malloc((q_index + 1 )*sizeof(struct cache_elm));
    for (unsigned int odd_natural = 3; odd_natural <= q; odd_natural += 2) {
        unsigned int index = odd_natural / 2;
        CACHE[index].prime = odd_natural;
        CACHE[index].prime_sqr_index = (odd_natural * odd_natural) / 2;
        CACHE[index].prime_2 = odd_natural * 2;
        CACHE[index].prime_3 = odd_natural * 3;
        CACHE[index].prime_4 = odd_natural * 4;
        CACHE[index].save_len = 0;
        if ((limit / 2) > CACHE[index].prime_3) {
            CACHE[index].save_len = (limit / 2) - CACHE[index].prime_3;
        }
        CACHE[index].last_prime_index = limit - ( limit % CACHE[index].prime);
        if ( !(CACHE[index].last_prime_index & 1) == 1) {
            CACHE[index].last_prime_index -= CACHE[index].prime;
        }
        CACHE[index].last_prime_index = CACHE[index].last_prime_index / 2;

        CACHE[index].save_index = CACHE[index].prime_sqr_index + CACHE[index].prime_3;
    }
}

#if STORE_MODE == STRIPED_3
unsigned int NR_OF_WORDS;
#endif

#ifdef RECORD
union
{
    unsigned int unsigned_int;
    unsigned char bytes[4];
} conv;
unsigned int nr_of_files = 0;
void dump_sieve (struct sieve_state *sieve_state) {
    FILE *write_ptr;
    char   file_name[256] ;

    sprintf(file_name,"/tmp/sieve_%u.bin",nr_of_files);
    write_ptr = fopen(file_name,"wb");
    for (unsigned int i = 0; i <= sieve_state->nr_of_words; i++) {
        conv.unsigned_int = sieve_state->bit_array[i];
        fwrite(conv.bytes,sizeof(conv.bytes),1,write_ptr);
    }
    fclose(write_ptr);

    nr_of_files++;
}
#endif

static inline struct sieve_state *create_sieve(unsigned int limit) {
    struct sieve_state *sieve_state=malloc(sizeof *sieve_state);
    sieve_state->size = limit >>1;
    sieve_state->nr_of_words=(limit >> MEMSHIFT) + 5;
    #if STORE_MODE == STRIPED_3
    NR_OF_WORDS = sieve_state->nr_of_words;
    #endif

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
#else // NORMAL

static inline unsigned int index_to_word(unsigned int bit_index) {
    return bit_index >> SHIFT;
}
static inline unsigned int index_to_offset(unsigned int bit_index) {
    return bit_index & MASK;
}
#endif

#ifdef RECORD
unsigned int frame_counter = 0;
#endif

static inline void setBit(struct sieve_state *sieve_state,unsigned int index) {
    #ifdef RECORD
    frame_counter++;
    if (frame_counter > 5000) {
        frame_counter =0;
        dump_sieve(sieve_state);
    }
    #endif
    sieve_state->bit_array[index_to_word(index)] |=  OFFSET_MASK[index_to_offset(index)];
}

static inline TYPE getBit (struct sieve_state *sieve_state,unsigned int index) {
    return sieve_state->bit_array[index_to_word(index)] & OFFSET_MASK[index_to_offset(index)];
}

static inline void bit_cross_out_simple(
    struct sieve_state *sieve_state,
    unsigned int prime_index
) {
    unsigned int prime = (prime_index << 1U)+1U;
    unsigned int start_index = ((prime * prime)>>1U);
    for ( unsigned int i = start_index; i <sieve_state->size ; i +=prime ) {
        setBit(sieve_state,i);
    }
}

static inline void bit_cross_out_reverse(
    struct sieve_state *sieve_state,
    unsigned int prime_index
) {
    unsigned int prime = (prime_index << 1U)+1U;
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
    unsigned int prime_index
) {
    unsigned int prime = (prime_index << 1U)+1U;
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
    unsigned int prime_index
) {
    unsigned int prime = (prime_index << 1U)+1U;
    unsigned int prime_2 = prime * 2;
    unsigned int prime_3 = prime * 3;
    unsigned int prime_4 = prime * 4;

    unsigned int end_index = ((prime * prime)>>1U);
    unsigned int last_prime= sieve_state->limit - (sieve_state->limit % prime);
    if ( !(last_prime & 1) == 1) {
        last_prime -= prime;
    }
    unsigned int current_index = last_prime / 2;

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

static inline void bit_cross_out_by_4_cached(
    struct sieve_state *sieve_state,
    unsigned int prime_index
) {
    struct cache_elm cur_elm = CACHE[prime_index];
    unsigned int current_index = cur_elm.prime_sqr_index;

    while (current_index < cur_elm.save_len) {
        setBit(sieve_state,current_index);
        setBit(sieve_state,current_index + cur_elm.prime);
        setBit(sieve_state,current_index + cur_elm.prime_2);
        setBit(sieve_state,current_index + cur_elm.prime_3);
        current_index += cur_elm.prime_4;
    }

    while (current_index <sieve_state->size) {
        setBit(sieve_state,current_index);
        current_index += cur_elm.prime;
    }
}

static inline void bit_cross_out_by_4_reverse_cached(
    struct sieve_state *sieve_state,
    unsigned int prime_index
) {
    struct cache_elm cur_elm = CACHE[prime_index];
    unsigned int current_index = cur_elm.last_prime_index;

    while (current_index > cur_elm.save_index) {
        setBit(sieve_state,current_index);
        setBit(sieve_state,current_index - cur_elm.prime);
        setBit(sieve_state,current_index - cur_elm.prime_2);
        setBit(sieve_state,current_index - cur_elm.prime_3);
        current_index -= cur_elm.prime_4;
    }

    while (current_index >= cur_elm.prime_sqr_index) {
        setBit(sieve_state,current_index);
        current_index -= cur_elm.prime;
    }
}


void run_sieve(struct sieve_state *sieve_state) {
    unsigned int factor_index = 1U;
    unsigned int q=(unsigned int)sqrt(sieve_state->limit);
    unsigned int q_index=q>>1U;
    int forward = 1;
    
    while (factor_index < q_index) {
        if ( getBit(sieve_state,factor_index) == ON ) {
            #if CROSSOUT_MODE == BY_4
            bit_cross_out_by_4(sieve_state,factor_index);
            #elif CROSSOUT_MODE == BACK_TO_FRONT_BY_4
            if (forward) {
                bit_cross_out_by_4(sieve_state,factor_index);
                forward = 0;  
            } else {
                bit_cross_out_by_4_reverse(sieve_state,factor_index);
                forward = 1;
            }
            #elif CROSSOUT_MODE == BACK_TO_FRONT_BY_4_CACHED
            if (forward) {
                bit_cross_out_by_4_cached(sieve_state,factor_index);
                forward = 0;  
            } else {
                bit_cross_out_by_4_reverse_cached(sieve_state,factor_index);
                forward = 1;
            }
            #elif CROSSOUT_MODE == BACK_TO_FRONT
            if (forward) {
                bit_cross_out_simple(sieve_state,factor_index);
                forward = 0;  
            } else {
                bit_cross_out_reverse(sieve_state,factor_index);
                forward = 1;
            }
            #else
            bit_cross_out_simple(sieve_state,factor_index);
            #endif
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
	printf("fvbakel_Cbit7_%s___%s;%d;%f;1;algorithm=base,faithful=yes,bits=%lu\n", STORE_MODES[STORE_MODE],CROSSOUT_MODES[CROSSOUT_MODE] ,passes, duration,1LU);
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
        #ifdef RECORD
        dump_sieve(sieve_state);
        #endif
        run_sieve(sieve_state);
        passes++;
        clock_gettime(CLOCK_MONOTONIC,&now);
        duration=now.tv_sec+now.tv_nsec*1e-9-start.tv_sec-start.tv_nsec*1e-9;

        
        if (duration>maxtime 
            #ifdef RECORD
             || passes == 1
            #endif
        ) {    
            if (print_sumary) {
                #ifdef RECORD
                dump_sieve(sieve_state);
                #endif
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

    fill_cache(limit);

    speed = run_timed_sieve(limit,maxtime,show_result,1);

    return 0;
}