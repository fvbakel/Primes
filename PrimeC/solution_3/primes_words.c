#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define BITONE 1U
#define BITZERO 0U

#define ON BITZERO
#define OFF BITONE

#ifdef COMPILE_64_BIT
#define TYPE uint64_t
#define MASK 63U
#define SHIFT 6U
#else
#define TYPE uint32_t
#define MASK 31U 
#define SHIFT 5U
#endif

struct sieve_state {
  TYPE *bit_array;
  unsigned int limit;
  unsigned int nr_of_words;
};

struct sieve_state *create_sieve(int limit) {
  struct sieve_state *sieve_state=malloc(sizeof *sieve_state);

  sieve_state->bit_array=calloc(limit/sizeof(TYPE)+1,sizeof(TYPE));
  sieve_state->limit=limit;
  sieve_state->nr_of_words=limit/sizeof(TYPE)+1;
  return sieve_state;
}

void delete_sieve(struct sieve_state *sieve_state) {
  free(sieve_state->bit_array);
  free(sieve_state);
}

void setWord(struct sieve_state *sieve_state,unsigned int word_offset, TYPE word_value) {
    sieve_state->bit_array[word_offset] = word_value;
}

void setWords(struct sieve_state *sieve_state,unsigned int word_offset, TYPE *word_values, unsigned int size) {
    memcpy(&(sieve_state->bit_array[word_offset]), word_values, size * sizeof(TYPE));
}

void repeatWords2end (
    struct sieve_state *sieve_state,
    unsigned int word_offset, 
    TYPE *word_values, 
    unsigned int size,
    unsigned int end
    ) {
    //size_t mem_size = size * sizeof(TYPE);
    unsigned int start_at = word_offset;
    if (end == 0) {
        end = sieve_state->nr_of_words;
    }

  /*  while ((start_at += size) < sieve_state->nr_of_words) {
        memcpy(&(sieve_state->bit_array[start_at]), word_values, mem_size);
        start_at += size;
    }
    */

    // so in the end this mass copy does not work, 
    // we have to do those one by one
    int i =0;
    while (start_at < end) {
        sieve_state->bit_array[start_at] = word_values[i];
        start_at++;
        i++;
        if (i == size) {
            i = 0;
        }
    }
}

TYPE getWord(struct sieve_state *sieve_state,unsigned int word_offset) {
    return sieve_state->bit_array[word_offset];
}

void setBit(struct sieve_state *sieve_state,unsigned int index) {
    unsigned int word_offset = index >> SHIFT;                // 1 word = 2ˆ5 = 32 bit, so shift 5, much faster than /32
    unsigned int offset  = index & MASK;                      // use & (and) for remainder, faster than modulus of /32
    sieve_state->bit_array[word_offset] |= (1 << offset);
}

TYPE getBit (struct sieve_state *sieve_state,unsigned int index) {
    unsigned int word_offset = index >> SHIFT;  
    unsigned int offset  = index & MASK;
    return sieve_state->bit_array[word_offset] & (1 << offset);     // use a mask to only get the bit at position bitOffset.
}

void run_sieve_segment ( 
    struct sieve_state *sieve_state,
    unsigned int start_nr,
    unsigned int end_nr,
    unsigned int stop_prime
    ) {

    unsigned int q=(unsigned int)sqrt(end_nr);
    unsigned int factor=start_nr;
    while (factor <= q) {
        // search next
        for (unsigned int num = factor; num <= end_nr; num+=2) {
            if ( getBit(sieve_state,num) == ON ) {
                factor = num;
                break;
            }
        }
        
        // crossout
        for (unsigned int num = factor * factor; num <= end_nr; num += factor * 2) {
            setBit(sieve_state,num);
        }
        
        if (factor == stop_prime) break;

        factor += 2;
    }

}

void run_sieve(struct sieve_state *sieve_state) {
    unsigned int prime_word_cpy;
    unsigned int prime_product = 1;
    unsigned int next_prime_product = 0;
    unsigned int max_product = (sieve_state->limit >> 2) / 100; // 1% of the total justifies the 

    // STEP 1:
    // First calculate all the primes in the first half word
    run_sieve_segment(sieve_state,3,8*sizeof(TYPE),0);

    // STEP 2:
    // find the optimal product
    for (unsigned int num = 3; num <= 8*sizeof(TYPE); num += 2) {
        if (getBit(sieve_state,num) == ON ) {
            next_prime_product = prime_product * num;
            //printf("Found prime: %d: next_prime_product= %d \n",num,next_prime_product);
            if (next_prime_product < max_product) {
                prime_product = next_prime_product;
                prime_word_cpy = num;
            }
        }
    }

   // printf("Found prime to copy: %d found prime product: %d, max product: %d\n", prime_word_cpy, prime_product, max_product);

    // STEP 3
    // crossout from begin to product for upto found prime
    run_sieve_segment(sieve_state,3,2*prime_product*8*sizeof(TYPE), prime_word_cpy );

    // STEP 4
    // now we can do a word crossout for the first few primes that
    // are determined by the login above
    repeatWords2end (
        sieve_state,
        prime_product+1, 
        &(sieve_state->bit_array[1]), 
        prime_product,
        0
    );

    // STEP 5
    // crossout the remaining
    run_sieve_segment(sieve_state,prime_word_cpy +2,sieve_state->limit,0);
}

void print_primes (struct sieve_state *sieve_state) {
    printf("%i,",2);
    for (unsigned int i = 3; i <= sieve_state->limit; i+=2) {
        if (getBit(sieve_state,i) == ON ) {
            printf("%i,",i);    
        }
    }
    printf("\n");
}

unsigned int count_primes (struct sieve_state *sieve_state) {
    unsigned int count = 1;
    for (unsigned int i = 3; i <= sieve_state->limit; i+=2) {
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
	printf("fvbakel_Cwords;%d;%f;1;algorithm=base,faithful=yes,bits=%lu\n", passes, duration,1LU);
}

int main(int argc, char **argv) {
    unsigned int        limit     = 1000000;
    double              maxtime     = 5.;
    unsigned int        show_result = 0;
    
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
        double duration=now.tv_sec+now.tv_nsec*1e-9-start.tv_sec-start.tv_nsec*1e-9;

        if (duration>=maxtime) {
            print_results ( sieve_state, show_result, duration, passes);
            break;
        }

        delete_sieve(sieve_state);
    }
    return 0;
}