#!/bin/bash

# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2019-12-21

### Run a single test for generated matrices. The parameters are TYPE
### and SIZE of the mesh, NPART number of processes/partitions and
### NLEVEL number of levels calculated by MPK.

SCRIPTS_DIR=${SCRIPTS_DIR:=$(dirname $0)/../scripts}
MATRICES_DIR=${MATRICES_DIR:=$(dirname $0)/../matrices}

TYPE=${1:-m5p} # $1 or defaults to m5p.
SIZE=${2:-4}   # $2 or defaults to 4.
NPART=${3:-4}  # $3 or defaults to 4.
NLEVEL=${4:-4} # $4 or defaults to 4.
MIRROR=${5:-0} # $5 or defaults to 0.
UPDATE=${6:-0} # $6 or defaults to 0.
MATRIX="$MATRICES_DIR/${TYPE}${SIZE}.loop.mtx"
shift 6

# Run test.
$SCRIPTS_DIR/prep_exec_test.sh $MATRIX $NPART $NLEVEL $MIRROR $UPDATE $*
