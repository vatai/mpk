#!/bin/bash

# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2019-12-20

# Script to execute pdmpk on matrices.

# Optional input variables
NLEVEL_ITER=${NLEVEL_ITER:="10"}
NPART_ITER=${NPART_ITER:="4"}
PREFIX=${PREFIX:=.}
MPIRUN=${MPIRUN:=mpirun}

function single_run() {
    local MATRIX=$1
    local NPART=$2
    local NLEVEL=$3
    local PREP=$PREFIX/pdmpk_prep
    local EXEC=$PREFIX/pdmpk_exec
    local TEST=$PREFIX/pdmpk_test
    echo $0: Processing $MATRIX with $NPART partitions upto level $NLEVEL
    $PREP $MATRIX $NPART $NLEVEL
    $MPIRUN -n $NPART $EXEC $MATRIX
    $TEST $MATRIX $NPART $NLEVEL
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

