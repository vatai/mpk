#!/bin/bash

# Create the data for each table corresponding to one $SIZE of the
# type $NAME (=$1).  The *ITER variables below are used to generate
# the rows of one table.
#
# The results is piped into the $OUTPUT.txt file where $OUTPUT is the
# input $NAME without the .mtx extension.
#
# Details: uses `mtx2gen` and `test_gen.sh`.  It should delete all
# files it generates.

NAME=$1
NAME=${NAME:=m5p}
SIZEITER="10 100"
NPARTITER="4 8"
NPHASEITER=$(seq 0 4)
NLEVELITER="10 20"
# SIZEITER="10"
# NPARTITER="2"
# NPHASEITER=$(seq 1 2)

# for NAME in m{5,7,9}p; do #
for SIZE in $SIZEITER; do #
    for NLEVEL in $NLEVELITER; do #
        for NPART in $NPARTITER; do #
            for NPHASE in $NPHASEITER; do #
                NLEVEL=$NLEVEL NAME=$NAME SIZE=$SIZE NPART=$NPART NPHASE=$NPHASE ./test_gen.sh
            done
        done
    done
    OUTPUT=${NAME}${SIZE}
    ls summary-* | while read f; do echo -n $f: && tail -1 $f ; done > $OUTPUT.txt
    ls times-* | while read f; do echo -n $f: && tail -1 $f ; done > $OUTPUT.times.txt
    rm {summary,times}-$OUTPUT*.log
    rm $OUTPUT*.{co,g0,val}
    rm -rf ${OUTPUT}_*
done
# done
