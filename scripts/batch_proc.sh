#!/bin/bash

# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2019-12-20

# Script to execute pdmpk on matrices.

# Optional input variables
NLEVEL_ITER=${NLEVEL_ITER:="10"}
NPART_ITER=${NPART_ITER:="4"}
PREFIX=${PREFIX:=../apps}
MPIRUN=${MPIRUN:=mpirun --oversubscribe}
$MPIRUN 2>&1 | grep "unrecognized argument oversubscribe" >/dev/null && MPIRUN=mpirun

function single_run() {
    local MATRIX=$1
    local NPART=$2
    local NLEVEL=$3
    local PREP=$PREFIX/prep
    local EXEC=$PREFIX/exec
    local TEST=$PREFIX/check
    echo $0: Processing $MATRIX with $NPART partitions upto level $NLEVEL
    $PREP --matrix $MATRIX --npart $NPART --nlevel $NLEVEL || exit 1
    $MPIRUN -n $NPART $EXEC --matrix $MATRIX --nlevel $NLEVEL || exit 2
    $TEST --matrix $MATRIX --npart $NPART --nlevel $NLEVEL || exit 3
}

function proc_matrix() {
    local MATRIX=$1
    for NLEVEL in $NLEVEL_ITER; do
        for NPART in $NPART_ITER; do
            single_run $MATRIX $NPART $NLEVEL
        done
    done
}

for MATRIX in "$@"; do
    proc_matrix $MATRIX
done

