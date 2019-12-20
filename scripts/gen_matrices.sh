#!/bin/bash


TYPE_ITER="m5p m9p"
SIZE_ITER="10 20"

function list_matrices() {
    echo "The following matrices will be generated in the current directory."
    for TYPE in $TYPE_ITER; do
        for SIZE in $SIZE_ITER; do
            NAME=${TYPE}${SIZE}
            echo $NAME
        done
    done
    read -n 1 -p "Press any key to continue"
}

function gen_matrices() {
}

list_matrices
gen_matrices

# NAME=${NAME:=m5p}
# SIZE=${SIZE:=100}
# echo $0
# pwd
# dirname $(pwd)
# basename $(pwd)
