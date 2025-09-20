#!/bin/bash

#if [ $# -eq 0 ]
#then
#    echo "Использование: ;0 число1 число2 ..."
#    exit 1
#fi

#sum=0
#count=$#

#for num in "$@"
#do 
#    sum=$((sum + num))
#done

#average=$(echo "scale=2; $sum / $count" | bc)

#echo "Количество: $count"
#echo "Среднее арифметическое: $average"#

echo "$@" | awk '{
    for(i=1; i<=NF; i++) {
        sum += $i
        count++
    }
    printf "Количество: %d\nСреднее: %.2f\n", count, sum/count
}'