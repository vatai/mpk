#!/bin/bash

# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2020-03-12

# Script to execute pdmpk programs on matrices.

# Optional input variables
MATRIX=$1
NPART=${2:-4}
NLEVEL=${3:-10}

PREFIX=${PREFIX:=../pdmpk}

MPIRUN=${MPIRUN:=mpirun --oversubscribe}
$MPIRUN 2>&1 | grep "unrecognized argument oversubscribe" >/dev/null && MPIRUN=mpirun

PREP=$PREFIX/pdmpk_prep
EXEC=$PREFIX/pdmpk_exec
TEST=$PREFIX/pdmpk_test
echo $0: Processing $MATRIX with $NPART partitions upto level $NLEVEL
echo $PREP --matrix $MATRIX --npart $NPART --nlevel $NLEVEL || exit 1
$PREP --matrix $MATRIX --npart $NPART --nlevel $NLEVEL || exit 1
echo $MPIRUN -n $NPART $EXEC --matrix $MATRIX --nlevel $NLEVEL || exit 2
$MPIRUN -n $NPART $EXEC --matrix $MATRIX --nlevel $NLEVEL || exit 2
echo $TEST --matrix $MATRIX --npart $NPART --nlevel $NLEVEL || exit 3
$TEST --matrix $MATRIX --npart $NPART --nlevel $NLEVEL || exit 3
