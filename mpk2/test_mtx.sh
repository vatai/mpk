#!/bin/bash -x

MTXFILE=$1
NAME=$(echo $MTXFILE | sed s/\.mtx$//)
G0FILE=${NAME}.g0
LOOPFILE=${NAME}.loop.g0

NPART=${NPART:=2}
NLEVEL=${NLEVEL:=10}
NPHASE=${NPHASE:=3}

DIRNAME=${NAME}_${NPART}_${NLEVEL}_${NPHASE}
echo dirname: $DIRNAME
echo loopfile: $LOOPFILE

test -f $NAME.g0 || test -f $NAME.loop.g0 || test -f $NAME.val || ./mtx2gen $MTXFILE
./driver $NAME $NPART $NLEVEL $NPHASE
cp -f $LOOPFILE $DIRNAME/loop.g0

# set -x # Uncomment this line to make verbose.
# OpenMP version
# ./mpktest $DIRNAME || exit 3

# MPI2 version: read/write buffers
mpirun -n $NPART ./mpi2_mpkwrtbufs $DIRNAME 1>/dev/null || exit 10
mpirun -n $NPART ./mpi2_mpkexecbufs $DIRNAME || exit 11
mpirun -n $NPART ./mpi2_mpkexecbufs_val $DIRNAME || exit 12
# ./mpkrun $DIRNAME || exit 13
./verify_val $DIRNAME || exit 14
diff $DIRNAME/gold_result.bin $DIRNAME/results.bin
