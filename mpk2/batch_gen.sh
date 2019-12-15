#!/bin/sh -x

SIZEITER="10 100"
NPARTITER="4 8"
NPHASEITER=$(seq 0 4)
NLEVELITER="10 20"
# SIZEITER="10 20"
# NPARTITER="2 4"
# NPHASEITER=$(seq 1 2)

# for NAME in m{5,7,9}p; do #
NAME=$1
for SIZE in $SIZEITER; do #
    for NLEVEL in $NLEVELITER; do #
        for NPART in $NPARTITER; do #
            for NPHASE in $NPHASEITER; do #
                NLEVEL=$NLEVEL NAME=$NAME SIZE=$SIZE NPART=$NPART NPHASE=$NPHASE ./test_builds.sh
            done
        done
    done
    OUTPUT=${NAME}${SIZE}
    ./collect-logs.sh > $OUTPUT.txt
    rm {summary,times}-$OUTPUT*.log
    rm $OUTPUT*.{co,g0,val}
done
# done
