#!/bin/bash

DIRNAME=$1
NAME=$(echo $1 | cut -d_ -f1)
NPART=$(echo $1 | cut -d_ -f2)
NLEVEL=$(echo $1 | cut -d_ -f3)
NPHASE=$(echo $1 | cut -d_ -f4)
# OpenMP version
# ./mpktest $DIRNAME || exit 3

set -x # Uncomment this line to make verbose.
# MPI2 version: read/write buffers
srun --ntasks=$NPART --tasks-per-node=8 -p p --mpi=pmi2 ./mpi2_mpkwrtbufs $DIRNAME || exit 10
# srun --ntasks=$NPART --tasks-per-node=8 -p p --mpi=pmi2 ./mpi2_mpkexecbufs $DIRNAME || exit 11
srun --ntasks=$NPART --tasks-per-node=8 -p p --mpi=pmi2 ./mpi2_mpkexecbufs_val $DIRNAME || exit 12
# ./mpkrun $DIRNAME || exit 13
./work_count $DIRNAME
./verify_val $DIRNAME
diff $DIRNAME/gold_result.bin $DIRNAME/results.bin
