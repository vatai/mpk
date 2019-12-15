#!/bin/sh

NLEVELITER="10 20"
NPARTITER="4 8"
NPHASEITER=$(seq 0 4)
# SIZEITER="10"
# NPARTITER="4"
# NPHASEITER=$(seq 1 2)

NAME=$1
./mtx2gen $NAME
for NLEVEL in $NLEVELITER; do #
    for NPART in $NPARTITER; do #
        for NPHASE in $NPHASEITER; do #
            NLEVEL=$NLEVEL NPART=$NPART NPHASE=$NPHASE ./test_mtx.sh $NAME
        done
    done
done

OUTPUT=$(echo ${NAME} | sed 's/\.mtx//')
./collect-logs.sh > $OUTPUT.txt
rm {summary,times}-$OUTPUT*.log
rm $OUTPUT*.{co,g0,val}
