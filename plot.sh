#!/bin/sh

data_size="10 100 1000 20000";

test_data="./test_data/test.txt";

for d_size in $data_size;
do
    (for i in {1..$d_size}; do echo $RANDOM; done) > $test_data;
    output="./out/data/out_lockfree.$d_size.dat";

    ./test.sh "$test_data" | tee $output;

done
