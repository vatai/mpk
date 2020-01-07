#!/bin/bash
# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2020-01-07

# Find files with *.stats.txt extensions recursively and print their
# name and contents separated by a colon.

DIR=${1:-.}
find ${DIR} -type f -name '*.stats.txt' | while read f; do
    echo $f: $(cat $f);
done
