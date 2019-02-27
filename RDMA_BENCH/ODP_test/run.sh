#!/bin/bash
SEQ=0
RAND=1
ODP=0
REG=1
ITER=100000
SMALL=2097152
BIG=1073741824
SERVER=114.212.84.179
CLIENT=114.212.85.112
DEV=mlx5_0

# test odp at sender & receiver
mkdir odp_both
cd odp_both
for SIZE in $SMALL $BIG
do
# seq
../a.out --device $DEV --mode $SEQ --memorytype $ODP --iteration $ITER --memory $SIZE &
ssh qhn@$CLIENT "~/ODP_test/a.out --device $DEV --memorytype $ODP --iteration $ITER --memory $SIZE --server $SERVER; exit"
# rand
../a.out --device $DEV --mode $RAND --memorytype $ODP --iteration $ITER --memory $SIZE &
ssh qhn@$CLIENT "~/ODP_test/a.out --device $DEV --memorytype $ODP --iteration $ITER --memory $SIZE --server $SERVER ; exit"
done

cd ..

mkdir odp_at_sender
cd odp_at_sender
for SIZE in $SMALL $BIG
do
# seq
../a.out --device $DEV --mode $SEQ --memorytype $ODP --iteration $ITER --memory $SIZE &
ssh qhn@$CLIENT "~/ODP_test/a.out --device $DEV --memorytype $REG --iteration $ITER --memory $SIZE --server $SERVER ; exit"
# rand
../a.out --device $DEV --mode $RAND --memorytype $ODP --iteration $ITER --memory $SIZE &
ssh qhn@$CLIENT "~/ODP_test/a.out --device $DEV --memorytype $REG --iteration $ITER --memory $SIZE --server $SERVER ; exit"
done
cd ..

mkdir odp_at_receiver
cd odp_at_receiver
for SIZE in $SMALL $BIG
do
# seq
../a.out --device $DEV --mode $SEQ --memorytype $REG --iteration $ITER --memory $SIZE &
ssh qhn@$CLIENT "~/ODP_test/a.out --device $DEV --memorytype $ODP --iteration $ITER --memory $SIZE --server $SERVER ; exit"
# rand
../a.out --device $DEV --mode $RAND --memorytype $REG --iteration $ITER --memory $SIZE &
ssh qhn@$CLIENT "~/ODP_test/a.out --device $DEV --memorytype $ODP --iteration $ITER --memory $SIZE --server $SERVER ; exit"
done
cd ..

mkdir reg_at_both
cd reg_at_both
for SIZE in $SMALL $BIG
do
# seq
../a.out --device $DEV --mode $SEQ --memorytype $REG --iteration $ITER --memory $SIZE &
ssh qhn@$CLIENT "~/ODP_test/a.out --device $DEV --memorytype $REG --iteration $ITER --memory $SIZE --server $SERVER ; exit"
# rand
../a.out --device $DEV --mode $RAND --memorytype $REG --iteration $ITER --memory $SIZE &
ssh qhn@$CLIENT "~/ODP_test/a.out --device $DEV --memorytype $REG --iteration $ITER --memory $SIZE --server $SERVER ; exit"
done
cd ..
