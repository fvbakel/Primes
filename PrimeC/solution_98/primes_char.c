#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define BITONE '\1'
#define BITZERO '\0'


#define ON BITZERO
#define OFF BITONE

#define TYPE char

struct sieve_state {
  TYPE *bit_array;
  unsigned int limit;
};

struct sieve_state *create_sieve(int limit) {
  struct sieve_state *sieve_state=malloc(sizeof *sieve_state);

  sieve_state->bit_array=calloc(limit + 1, sizeof(TYPE) );
  sieve_state->limit=limit;
  return sieve_state;
}

void delete_sieve(struct sieve_state *sieve_state) {
  free(sieve_state->bit_array);
  free(sieve_state);
}

void setBit(struct sieve_state *sieve_state,unsigned int index) {
    sieve_state->bit_array[index] = BITONE;
    }

TYPE getBit (struct sieve_state *sieve_state,unsigned int index) {
    return sieve_state->bit_array[index];
}

void run_sieve(struct sieve_state *sieve_state) {
    unsigned int factor = 3;
    unsigned int q=(unsigned int)sqrt(sieve_state->limit);

    while (factor <= q) {
        // search next
        if ( getBit(sieve_state,factor) == ON ) {
            // crossout
            for (unsigned int num = factor * factor; num <= sieve_state->limit; num += factor * 2) {
                setBit(sieve_state,num);
            }
        }  
       
        factor += 2;
    }
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
	printf("fvbakel_Cchar;%d;%f;1;algorithm=base,faithful=yes,bits=%lu\n", passes, duration,8*sizeof(char));
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