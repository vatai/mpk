#!/bin/bash

make || exit -1

MTXFILE=$1
NAME=$(echo $MTXFILE | sed s/\.mtx$// | sed s/_/-/g)
G0FILE=${NAME}.g0
LOOPFILE=${NAME}.loop.g0

NPART=${NPART:=4}
NLEVEL=${NLEVEL:=10}
NPHASE=${NPHASE:=3}

DIRNAME=${NAME}_${NPART}_${NLEVEL}_${NPHASE}
rm -rf ${DIRNAME}

test -f $G0FILE || test -f $LOOPFILE || test -f $NAME.val || ./mtx2gen $MTXFILE
./driver $NAME $NPART $NLEVEL $NPHASE
cp -f $LOOPFILE $DIRNAME/loop.g0

./run_script.sh $DIRNAME

rm -rf $DIRNAME
