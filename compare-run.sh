#/bin/bash
source ~/node/add-path.sh
OUTPUT_FILE=/home/fvbakel/Documenten/primes/results/output-multi-2020-07-29.csv
i=0
while [ $i -lt 303 ]
do
    make DIRECTORY=PrimeC/solution_1 FORMATTER=csv >>$OUTPUT_FILE
    make DIRECTORY=PrimeC/solution_2 FORMATTER=csv >>$OUTPUT_FILE
    make DIRECTORY=PrimeC/solution_3 FORMATTER=csv >>$OUTPUT_FILE
    make DIRECTORY=PrimeCython/solution_1 FORMATTER=csv >>$OUTPUT_FILE
    make DIRECTORY=PrimeMixed/solution_1 FORMATTER=csv >>$OUTPUT_FILE
    i=$[$i+1]
done