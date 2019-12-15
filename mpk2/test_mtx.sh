#!/bin/bash

# Convert an MTX matrix (to the metis format) and run DMPK,
# measurements and verification (see run_script.sh) of the results
# based on the (optional) parameters below.

MTXFILE=$1
NPART=${NPART:=4}
NLEVEL=${NLEVEL:=10}
NPHASE=${NPHASE:=3}

make || exit -1

NAME=$(echo $MTXFILE | sed s/\.mtx$// | sed s/_/-/g)
G0FILE=${NAME}.g0
LOOPFILE=${NAME}.loop.g0


DIRNAME=${NAME}_${NPART}_${NLEVEL}_${NPHASE}
rm -rf ${DIRNAME}

test -f $G0FILE || test -f $LOOPFILE || test -f $NAME.loop.val || ./mtx1gen $MTXFILE
./driver $NAME $NPART $NLEVEL $NPHASE
cp -f $LOOPFILE $DIRNAME/loop.g0

./run_script.sh $DIRNAME
