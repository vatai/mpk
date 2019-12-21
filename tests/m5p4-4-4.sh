#!/bin/bash -x

SCRIPTS_DIR=${SCRIPTS_DIR:=$(dirname $0)/../scripts}
MATRICES_DIR=${MATRICES_DIR:=$(dirname $0)/../matrices}

MATRIX="$MATRICES_DIR/m5p4.loop.mtx"

if [ ! -f "$MATRIX" ]; then
    SIZE_ITER=4 TYPE_ITER=m5p ${SCRIPTS_DIR}/gen_matrices.sh ${MATRICES_DIR}
fi

$SCRIPTS_DIR/batch_proc.sh $MATRIX
