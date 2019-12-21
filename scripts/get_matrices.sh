#!/bin/bash

# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2019-12-21

### Download (and extract) a matrix or a list of matrices. The first
### parameter is either an URL to a matrix (usually in the sparse
### matrix collection) or a text file containing one URL per line.
### The second parameter is the target where the matrices should be
### downloaded.
###
### The also script checks if the matrix was already downloaded.

FILE_OR_URL=$1
OUTPUT=${2:-.}

function download_untar () {
    wget -c $1 -O - | tar -xz -C $OUTPUT
}

# If $1 is a file.
if [ -f "$FILE_OR_URL" ]; then
    FILE="$FILE_OR_URL"
    cat "${FILE}" | while read LINE; do
        MATRIX=$(basename $LINE | sed 's/\.tar\.gz$//')
        # Download if doesn't exist.
        [ -e $OUTPUT/$MATRIX ] || download_untar $LINE
    done
else
    download_untar $FILE_OR_URL
fi
