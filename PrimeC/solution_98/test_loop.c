#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define TYPE uint32_t

void print_results (    
        double          duration,
        int             passes)
{

    printf("Passes: %d, Time: %f, Avg: %f\n",
            passes,
            duration,
            (duration / passes)
        );
}

int main(int argc, char **argv) {
    double              maxtime     = 5.;

    struct timespec     start,now;
    double              duration;
    int                 passes      = 0;

    unsigned int dummy = 0;

    clock_gettime(CLOCK_MONOTONIC,&start);

    while (1) {
        passes++;
        clock_gettime(CLOCK_MONOTONIC,&now);
        duration=now.tv_sec+now.tv_nsec*1e-9-start.tv_sec-start.tv_nsec*1e-9;

        if (duration>=maxtime) {
            print_results ( duration, passes);
            break;
        }
    }

    return 0;
}