#!/bin/bash

# Create the data for one table for the .mtx matrix $NAME (=$1).  The
# *ITER variables below are used to generate the rows of one table.
#
# The results is piped into the $OUTPUT.txt file where $OUTPUT is the
# input $NAME without the .mtx extension.
#
# Details: uses `mtx2gen` and `test_mtx.sh`.  It should delete all
# files it generates.

NLEVELITER="10 20"
NPARTITER="4 8"
NPHASEITER=$(seq 0 4)
# NLEVELITER="10"
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
ls summary-* | while read f; do echo -n $f: && tail -1 $f ; done > $OUTPUT.txt
ls times-* | while read f; do echo -n $f: && tail -1 $f ; done > $OUTPUT.times.txt
rm {summary,times}-$OUTPUT*.log
rm $OUTPUT*.{g0,val}
rm -rf ${OUTPUT}_*
