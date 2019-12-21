#!/bin/bash

# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2019-12-20

### Script to generate matrices.
###
### @param $1 Output directory. The directory should exist.

# Optional input variables
TYPE_ITER=${TYPE_ITER:="m5p m9p"}
SIZE_ITER=${SIZE_ITER:="10 20"}
OUTPUTDIR=${1:-.}
PREFIX=${PREFIX:=$(dirname $0)/../mpk2}


function list_matrices() {
    echo "The following matrices will be generated in the current directory."
    for TYPE in $TYPE_ITER; do
        for SIZE in $SIZE_ITER; do
            local NAME=$OUTPUTDIR/${TYPE}${SIZE}
            echo $NAME
        done
    done
    # Pause if in an interactive shell.
    if [[ $- == *i* ]]; then
        read -n 1 -p "Press any key to continue"
        echo
    fi

}

function gen_matrix() {
    local GEN2=$PREFIX/gen2
    local GEN2MTX=$PREFIX/gen2mtx
    local TYPE=$1
    local SIZE=$2
    local NAME=$OUTPUTDIR/$TYPE$SIZE
    $GEN2 $TYPE $SIZE $NAME
    $GEN2MTX $NAME.loop.g0
    rm $NAME.loop.{co,val,g0}
}

function gen_matrices() {
    for TYPE in $TYPE_ITER; do
        for SIZE in $SIZE_ITER; do
            gen_matrix $TYPE $SIZE
        done
    done
}

list_matrices
gen_matrices

