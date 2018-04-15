#!/bin/bash
SCBR=../../scbr/bin/scbr
ADDRESS=localhost:5555
WORKER=../bin/worker
CLIENT=../bin/client
MAPPER=kmeans_map.lua
REDUCER=kmeans_reduce.lua

if [ "$#" -ne 4 ]; then
	echo "Usage: sh [#MAPPERS] [#REDUCERS] [CENTERS] [DATAPOINTS]"
	exit
fi

killall -9 scbr worker client
sleep 2
$SCBR &
sleep 5
for mnum in $(seq 0 $(expr $1 - 1)); do
	$WORKER -e -m $mnum -p $ADDRESS &
done

for rnum in $(seq 0 $(expr $2 - 1)); do
	$WORKER -e -r $rnum -p $ADDRESS &
done
sleep 3

$CLIENT -e -m $MAPPER -r $REDUCER -p $ADDRESS -d $4 -k $3
killall -9 scbr worker

