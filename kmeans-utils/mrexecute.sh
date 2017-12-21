#!/bin/bash
SCBR=/home/securecloud/code/scbr/sgx/enclave_CBR_Enclave_Filter/scbr
ADDRESS=localhost:5555
WORKER=/home/securecloud/code/mapreduce/bin/worker
CLIENT=/home/securecloud/code/mapreduce/bin/client
MAPPER=/home/securecloud/code/mapreduce/kmeans_map.lua
REDUCER=/home/securecloud/code/mapreduce/kmeans_reduce.lua

if [ "$#" -ne 4 ]; then
	echo "Usage: sh [#MAPPERS] [#REDUCERS] [CENTERS] [DATAPOINTS]"
	exit
fi

killall -2 scbr worker client
$SCBR &
for mnum in $(seq 0 $(expr $1 - 1)); do
	$WORKER -e -m $mnum -p $ADDRESS &
done

for rnum in $(seq 0 $(expr $2 - 1)); do
	$WORKER -e -r $rnum -p $ADDRESS &
done

$CLIENT -e -m $MAPPER -r $REDUCER -p $ADDRESS -d $4 -k $3 &

