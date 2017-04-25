#!/bin/bash

cores="1 2 4 8 16 32 64";
test_data=$1;

for c in $cores
do
    printf "%d" $c;
    ./sort "$c" "$test_data" | grep "#Elapsed_time:" | cut -d':' -f2 | cut -d'm' -f1;

done
