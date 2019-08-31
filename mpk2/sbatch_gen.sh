#!/bin/sh

# SIZEITER="10 50 100 200"
# NPARTITER="2 4 8 16"
# NPHASEITER=$(seq 0 5)
SIZEITER="10 20"
NPARTITER="2 4"
NPHASEITER=$(seq 1 2)

# for NAME in m{3,5,7}p t{3,5,7}p cube; do #
for NAME in m3p t3p cube; do #
    for SIZE in $SIZEITER; do #
        for NPART in $NPARTITER; do #
            for NPHASE in $NPHASEITER; do #
                NAME=$NAME SIZE=$SIZE NPART=$NPART NPHASE=$NPHASE sbatch -N $NPART mpi_gen.sh
            done
        done
    done
done
