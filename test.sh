#!/bin/bash
./coordinator -n 10 -p 45 -t 8 &
sleep 3 # Give enough time for coordinator to start
./server -m 0001 &
./cashier -s 3 -b 9 -m 0001 &
./cashier -s 7 -b 7 -m 0001 &
./cashier -s 7 -b 5 -m 0001 &
sleep 3 # Give enough time for server and casher to start

INSTANCES=1
for ((i=0; $i<$INSTANCES; ++i))
do
    ./client -i 17 -e 2 -m 0001 &
done

INSTANCES=5
for ((i=0; $i<$INSTANCES; ++i))
do
    ./client -i 15 -e 2 -m 0001 &
done

INSTANCES=6
for ((i=0; $i<$INSTANCES; ++i))
do
    ./client -i 18 -e 2 -m 0001 &
done

INSTANCES=3
for ((i=0; $i<$INSTANCES; ++i))
do
    ./client -i 20 -e 2 -m 0001 &
done

INSTANCES=7
for ((i=0; $i<$INSTANCES; ++i))
do
    ./client -i 16 -e 2 -m 0001 &
done

INSTANCES=4
for ((i=0; $i<$INSTANCES; ++i))
do
    ./client -i 19 -e 1 -m 0001 &
done
