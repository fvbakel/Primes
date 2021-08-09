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


// The setting below is set to 16kb
// this L1 cache size is assumed if it can not be determined
#define DEFAULT_L1_SIZE (16*1024)
#define KEEP_FREE 1500
//
// The configuration parameter below determines 
unsigned int BLOCK_SIZE = (DEFAULT_L1_SIZE - KEEP_FREE ) / sizeof(TYPE);

// the const below is to reduce the multiplications
const unsigned int BITS_IN_WORD=8*sizeof(TYPE);

// the constant below is a cache of all the possible bit masks
const TYPE offset_mask[] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824,2147483648};


struct sieve_state {
  TYPE *bit_array;
  unsigned int limit;
  unsigned int nr_of_words;
};

static inline struct sieve_state *create_sieve(int limit) {
  struct sieve_state *sieve_state=malloc(sizeof *sieve_state);

  sieve_state->nr_of_words=(limit >> MEMSHIFT) + 1;
  sieve_state->bit_array=calloc(sieve_state->nr_of_words,sizeof(TYPE));
  sieve_state->limit=limit;
  return sieve_state;
}

static inline void delete_sieve(struct sieve_state *sieve_state) {
  free(sieve_state->bit_array);
  free(sieve_state);
}

static inline void setBit(struct sieve_state *sieve_state,unsigned int index) {
    unsigned int word_offset = index >> SHIFT;                // 1 word = 2ˆ5 = 32 bit, so shift 5, much faster than /32
    unsigned int offset  = index & MASK;                      // use & (and) for remainder, faster than modulus of /32
    sieve_state->bit_array[word_offset] |=  offset_mask[offset];
}

static inline TYPE getBit (struct sieve_state *sieve_state,unsigned int index) {
    unsigned int word_offset = index >> SHIFT;  
    unsigned int offset  = index & MASK;
    return sieve_state->bit_array[word_offset] & offset_mask[offset];     // use a mask to only get the bit at position bitOffset.
}

static inline unsigned int get_first_index(unsigned int word_index) {
    return word_index * BITS_IN_WORD;
}

static inline unsigned int get_last_index(unsigned int word_index) {
    return ((word_index +1) * BITS_IN_WORD ) - 1;
}
static inline unsigned int index_to_word(unsigned int bit_index) {
    return bit_index >> SHIFT;
}
static inline unsigned int index_to_natural(unsigned int bit_index) {
    return (bit_index << 1) +1 ;
}
static inline unsigned int natural_to_index(unsigned int natural) {
    return (natural - 1) >> 1 ;
}


/*
    Purpose:
    Crossout bit by bit

*/
static inline void bit_cross_out(
    struct sieve_state *sieve_state,
    unsigned int prime,
    unsigned int max_index
) {
    unsigned int start_index = ((prime * prime)>>1U);
    for ( unsigned int i = start_index; i <= max_index; i +=prime ) {
        setBit(sieve_state,i);
    }
}

static inline void bit_cross_out_by_block(
    struct sieve_state *sieve_state,
    unsigned int start_prime_index,
    unsigned int max_prime_index,
    unsigned int start_word, // index 0 of this word will be included
    unsigned int end_word    // including this word will be processed
) {
    unsigned int prime;
    unsigned int start_index;
    unsigned int min_index = get_first_index(start_word);
    unsigned int max_index = get_last_index(end_word);
    unsigned int min_natural;
    unsigned int next_natural;

    for (unsigned int idx = start_prime_index; idx <=max_prime_index;idx++) {
        if (getBit(sieve_state,idx) == ON) {
            prime = index_to_natural(idx);
            start_index = ((prime * prime)>>1U);
            if (start_index > max_index) {
                // start index is larger than this block
                break;
            } else if (start_index < min_index) {
                // continue crossout
                // calculate new start point
                min_natural = index_to_natural(min_index);
                next_natural = (((unsigned int) ((min_natural-1) / prime)) +1 ) * prime;
                if ((next_natural & 1U) !=1U) {
                    // even number add one more prime
                    next_natural += prime;
                }
                start_index = natural_to_index(next_natural);
            } else {
                // start index is in this block
                // no action needed
            }

            for ( unsigned int i = start_index; i <= max_index; i +=prime ) {
                setBit(sieve_state,i);
            }

        }
    }

    
}

void run_sieve(struct sieve_state *sieve_state) {
    unsigned int factor_index = 1U;
    unsigned int prime;
    unsigned int max_index=sieve_state->limit>>1U;
    unsigned int q=(unsigned int)sqrt(sieve_state->limit);
    unsigned int q_index=q>>1U;
    
    unsigned int max_index_1_block=get_last_index( BLOCK_SIZE);
    unsigned int current_start_block = 0;
    unsigned int current_end_block = sieve_state->nr_of_words-1;
    
    if (max_index > max_index_1_block) {
        max_index = max_index_1_block;
        current_end_block = BLOCK_SIZE -1;
    }
  
    while (factor_index < q_index) {
        // search next
        if ( getBit(sieve_state,factor_index) == ON ) {
            prime = (factor_index << 1U)+1U;
            // crossout to the end of the first block
            for (unsigned int num = (prime * prime)>>1U ; num <= max_index; num += prime) {
                setBit(sieve_state,num);
            }  
        }
        factor_index++;
    }

    current_start_block = current_end_block +1;
    current_end_block = current_start_block + BLOCK_SIZE;
    while (current_start_block < sieve_state->nr_of_words) {
        if (current_end_block >= sieve_state->nr_of_words) {
            current_end_block = sieve_state->nr_of_words -1;
        }
        bit_cross_out_by_block(sieve_state,1,q_index,current_start_block,current_end_block);
        current_start_block = current_end_block +1;
        current_end_block = current_start_block + BLOCK_SIZE;
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
	printf("fvbakel_Cbit3;%d;%f;1;algorithm=base,faithful=yes,bits=%lu\n", passes, duration,1LU);
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

        if (duration>maxtime ) {
            if (print_sumary) {
                print_results ( sieve_state, show_result, duration, passes);
            }
            break;
        }

        delete_sieve(sieve_state);
    }

    return passes/duration;
}

/*
    Purpose:
    Determine the optimal size for the BLOCK_SIZE 
    parameter on this hardware based on sample runs
    and set that parameter
*/
void set_word_block_size(const unsigned int limit) {
    const double sample_time = 0.2;
    const unsigned int block_size_samples_kb[] = {4,16,32,64,128,(1024*4)};
    const unsigned int nr_of_samples = 6;
    unsigned int block_size_samples[(nr_of_samples)];
    double speed[(nr_of_samples)];
    unsigned int max_speed_index = 0;

    for (unsigned int i = 0; i <nr_of_samples; i ++) {
        block_size_samples[i] = ((block_size_samples_kb[i] * 1024) - KEEP_FREE) / sizeof(TYPE);
        BLOCK_SIZE = block_size_samples[i];
        speed[i] = run_timed_sieve(limit,sample_time,0,0);
    }

    for (unsigned int i = 1; i <(nr_of_samples); i ++) {
        if (speed[i]> speed[max_speed_index]) {
            max_speed_index = i;
        }
    }
    BLOCK_SIZE = block_size_samples[max_speed_index];
}

int main(int argc, char **argv) {
    unsigned int        limit       = 1000000;
    double              maxtime     = 5.;
    unsigned int        show_result = 0;
    
    double              speed;

    set_word_block_size(limit);
    printf("using blocksize=%u\n",BLOCK_SIZE);
    speed = run_timed_sieve(limit,maxtime,show_result,1);

    return 0;
}