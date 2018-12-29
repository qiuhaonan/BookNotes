#!/bin/bash

total_iter=1048576 # 1M
data_size=65536 # 16KB
tx_depth=64

for qp_num in 1 2 4 8 16 32
do
    ./integration_test.sh qp_num_${qp_num} 1 33 1 $data_size $tx_depth $total_iter $qp_num
done
