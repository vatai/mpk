#!/bin/sh

SIZEITER="10 100"
NPARTITER="4 8"
NPHASEITER=$(seq 0 4)
# SIZEITER="10 20"
# NPARTITER="2 4"
# NPHASEITER=$(seq 1 2)

for NAME in $(ls *.mtx); do #
# for NAME in m5p t5p cube; do #
    for SIZE in $SIZEITER; do #
        for NPART in $NPARTITER; do #
            for NPHASE in $NPHASEITER; do #
                ./mtx2gen $NAME
                SIZE=$SIZE NPART=$NPART NPHASE=$NPHASE ./test_mtx.sh $NAME
            done
        done
    done
done
