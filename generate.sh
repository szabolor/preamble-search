#!/bin/sh
STARTBITLEN=2
STOPBITLEN=28
THRESHOLD=4

for BITLEN in $(seq -f "%02g" $STARTBITLEN $STOPBITLEN);
do
	echo "Executing for bitlen = $BITLEN"
   ./search_binary $BITLEN $THRESHOLD 0 -1 > output/raw_${BITLEN}bit_${THRESHOLD}thres
    sort -t $'\t' -k 4,4 -n output/raw_${BITLEN}bit_${THRESHOLD}thres > output/sorted_${BITLEN}bit_${THRESHOLD}thres
    echo
done