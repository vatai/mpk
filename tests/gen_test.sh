#!/bin/bash

# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2019-12-21

### Run a single test for generated matrices. The parameters are SIZE
### of the mesh, NPART number of processes/partitions and NLEVEL
### number of levels calculated by MPK.

SCRIPTS_DIR=${SCRIPTS_DIR:=$(dirname $0)/../scripts}
MATRICES_DIR=${MATRICES_DIR:=$(dirname $0)/../matrices}

TYPE=${1:-m5p} # $1 or defaults to m5p.
SIZE=${2:-4}   # $2 or defaults to 4.
NPART=${3:-4}  # $3 or defaults to 4.
NLEVEL=${4:-4} # $4 or defaults to 4.

# Create input matrix if it doesn't exist.
mkdir -p "$MATRICES_DIR"
MATRIX="$MATRICES_DIR/${TYPE}${SIZE}.loop.mtx"
if [ ! -f "$MATRIX" ]; then
    SIZE_ITER=${SIZE} TYPE_ITER=$TYPE ${SCRIPTS_DIR}/gen_matrices.sh ${MATRICES_DIR}
fi

# Run test.
NLEVEL_ITER=$NLEVEL NPART_ITER=$NPART $SCRIPTS_DIR/batch_proc.sh $MATRIX
