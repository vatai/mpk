#!/bin/bash

# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2020-03-12

# Script to execute pdmpk programs on matrices.

# Optional input variables
MATRIX=$1
NPART=${2:-4}
NLEVEL=${3:-10}
MIRROR=${4:-0}
WEIGHT=${5:-0}
shift 5

PREFIX=${PREFIX:=../apps}

MPIRUN=${MPIRUN:=mpirun --oversubscribe}
$MPIRUN 2>&1 | grep "unrecognized argument oversubscribe" >/dev/null && MPIRUN=mpirun

PREP=$PREFIX/prep
EXEC=$PREFIX/exec
TEST=$PREFIX/check
echo "$0: Processing $MATRIX with $NPART partitions upto level $NLEVEL (mirror $MIRROR, weight $WEIGHT)"

$PREP --matrix $MATRIX --npart $NPART --nlevel $NLEVEL --mirror $MIRROR --weight $WEIGHT $* || exit 1
$MPIRUN -n $NPART $EXEC --matrix $MATRIX --nlevel $NLEVEL --mirror $MIRROR --weight $WEIGHT $* || exit 2
$TEST --matrix $MATRIX --npart $NPART --nlevel $NLEVEL --mirror $MIRROR --weight $WEIGHT $* || exit 3
