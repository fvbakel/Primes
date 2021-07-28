#/bin/bash
source ~/node/add-path.sh
OUTPUT_FILE=/home/fvbakel/Documenten/primes/results/output-words-2020-07-28.csv
i=0
while [ $i -lt 303 ]
do
    make DIRECTORY=PrimeC/solution_98 FORMATTER=csv >>$OUTPUT_FILE
    i=$[$i+1]
done