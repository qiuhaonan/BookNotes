#!/bin/bash

total_iter=1048576 # 1M
data_size=1024 # 16KB
tx_depth=128

for qp_num in 1
do
    for pipeline in 1 2 4 8 16 32 64 128
    do
        ./integration_test.sh batch_signal_${pipeline} 1 33 1 $data_size $tx_depth $total_iter $qp_num $pipeline
    done 
done
