#!/bin/sh -x

SIZEITER="10 100"
NPARTITER="4 8"
NPHASEITER=$(seq 0 4)
# SIZEITER="10 20"
# NPARTITER="2 4"
# NPHASEITER=$(seq 1 2)

for NAME in m{5,7,9}p; do #
# for NAME in m5p t5p cube; do #
    for SIZE in $SIZEITER; do #
        for NPART in $NPARTITER; do #
            for NPHASE in $NPHASEITER; do #
                NLEVEL=20 NAME=$NAME SIZE=$SIZE NPART=$NPART NPHASE=$NPHASE ./test_builds.sh
            done
        done
    done
done