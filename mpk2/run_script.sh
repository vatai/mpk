#!/bin/bash

# Run
#
# - DMPK (`mpi2_mpkworkbufs` and `mpi2_mpkexecbufs_val`) 
#
# - Measurements (`word_count`)
#
# - Verification (`verify_val`)

DIRNAME=$1
NAME=$(echo $1 | cut -d_ -f1)
NPART=$(echo $1 | cut -d_ -f2)
NLEVEL=$(echo $1 | cut -d_ -f3)
NPHASE=$(echo $1 | cut -d_ -f4)
# OpenMP version
# ./mpktest $DIRNAME || exit 3

set -x # Uncomment this line to make verbose.
# MPI2 version: read/write buffers
mpirun -n $NPART ./mpi2_mpkwrtbufs $DIRNAME || exit 10
# mpirun -n $NPART ./mpi2_mpkexecbufs $DIRNAME || exit 11
mpirun -n $NPART ./mpi2_mpkexecbufs_val $DIRNAME || exit 12
# ./mpkrun $DIRNAME || exit 13
./work_count $DIRNAME
./verify_val $DIRNAME
diff $DIRNAME/gold_result.bin $DIRNAME/results.bin
