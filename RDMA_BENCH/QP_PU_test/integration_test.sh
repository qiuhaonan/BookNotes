#!/bin/bash

# script file_name foreground_flow_num foreground_tc foreground_batch_size foreground_io_size tx_depth iteration qpnum

let "FG_REG_MEM_SIZE=2*1024*1024"
TCP_PORT=2018
FG_FLOW_NUM=$2
TC_FG=$3
FG_BATCH_SIZE=$4
FG_IO_MEM_SIZE=$5
TX_DEPTH=$6
DEFAULT_ITERATION=$7
QP_NUM=$8
GID_INDEX=3
DEFAULT_TC=0
CORE=1 # attention for NUMA machines
REMOTE_CORE=0

PROCESS_NUM=0
WAIT_PID=0
PID_ARRAY[0]=0
LOCAL_IP=114.212.84.179
REMOTE_IP=114.212.85.112

# small flow
function START()
{
	taskset -c $CORE /home/qhn/rdma_about/qRdma/1_21_exp/QP_PU_test/hello -p $TCP_PORT -m $FG_REG_MEM_SIZE -o $FG_IO_MEM_SIZE -b $FG_BATCH_SIZE -t $TC_FG -g $GID_INDEX -I $DEFAULT_ITERATION -q $QP_NUM -v -P $TX_DEPTH -h -L -R > HELLO_LOG & 
	echo "Start fg hello locally, PID: $!"
    PID_ARRAY[$PROCESS_NUM]=$!
    WAIT_PID=$!
    let PROCESS_NUM++

	ssh qhn@$REMOTE_IP \
	"taskset -c $REMOTE_CORE /home/qhn/rdma_about/qRdma/1_21_exp/QP_PU_test/hello -p $TCP_PORT -m $FG_REG_MEM_SIZE -o $FG_IO_MEM_SIZE -b $FG_BATCH_SIZE -t $TC_FG -g $GID_INDEX -P $TX_DEPTH -q $QP_NUM -h -L -R $LOCAL_IP ; exit" &
    # may sleep (read socket) in ssh	
	echo "Start fg hello remotely by ssh, PID : $!"
	echo ""
    PID_ARRAY[$PROCESS_NUM]=$!
    let PROCESS_NUM++
    let "CORE=CORE+1"
    let "REMOTE_CORE=REMOTE_CORE+1"
}

function INDIVIDUAL_TEST()
{
	echo "INDIVIDUAL TEST START...."
	mkdir individual_test
	cd individual_test
    for (( i=1; i<=$FG_FLOW_NUM; i++))
    do
	    START
        let TCP_PORT++
    done
	wait
	cd ..
	echo "INDIVIDUAL TEST COMPLETE...."
	echo ""
}

function CLASS_TEST()
{
	mkdir $1
	cd $1
	INDIVIDUAL_TEST
	cd ..
}

function KILL_BACKGROUND()
{
    echo "ready to kill all background processes"
    if [ ${#PID_ARRAY[@]} -ne 0 ]
    then
        for pid in ${PID_ARRAY[@]}
        do
            if [ $pid -ne 0 ]
            then
                kill $pid
            fi
        done
	    ssh qhn@$REMOTE_IP "/home/qhn/rdma_about/qRdma/1_21_exp/write/kill.sh ; exit" 
    fi
}

#调用主函数
CLASS_TEST $1
#KILL_BACKGROUND

echo "TEST COMPLETE!"
