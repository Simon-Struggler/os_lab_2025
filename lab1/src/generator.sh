#!/bin/bash

for i in {1..150} 
do
    num=$(od -An -N2 -i /dev/random | awk '{print $1 % 1000 + 1}')
    echo $num
done > numbers.txt

echo "Файл numbers.txt создан с 150 случайными числами из /dev/random"