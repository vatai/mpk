#!/bin/bash

# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2019-12-21

### Run a single test for downloaded matrices. The parameters are the
### URL of the matrix, NPART number of processes/partitions and NLEVEL
### number of levels calculated by MPK.

SCRIPTS_DIR=${SCRIPTS_DIR:=$(dirname $0)/../scripts}
MATRICES_DIR=${MATRICES_DIR:=$(dirname $0)/../matrices}

URL=$1 # $1 or defaults to m5p.
NPART=${2:-4}  # $2 or defaults to 4.
NLEVEL=${3:-4} # $3 or defaults to 4.
MIRROR=${4:-0} # $4 or defaults to 0.
UPDATE=${5:-0} # $5 or defaults to 0.

# Download input matrix if it doesn't exist.
if [ ! -f "$MATRIX" ]; then
    ${SCRIPTS_DIR}/get_matrices.sh ${URL} ${MATRICES_DIR}
fi

# Run test.
MTX=$(basename $URL | sed 's/\.tar\.gz$//')
MATRIX=$MATRICES_DIR/$MTX/$MTX.mtx
$SCRIPTS_DIR/prep_exec_test.sh $MATRIX $NPART $NLEVEL $MIRROR $UPDATE
