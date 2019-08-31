#!/bin/sh

# SIZEITER="10 50 100 200"
# NPARTITER="2 4 8 16"
# NPHASEITER=$(seq 0 5)
SIZEITER="10 20"
NPARTITER="2 4"
NPHASEITER=$(seq 1 2)

# for NAME in m{5,7,9}p t{5,7,9}p cube; do #
for NAME in m5p t5p cube; do #
    for SIZE in $SIZEITER; do #
        for NPART in $NPARTITER; do #
            for NPHASE in $NPHASEITER; do #
                NAME=$NAME SIZE=$SIZE NPART=$NPART NPHASE=$NPHASE sbatch -N $NPART mpi_gen.sh
            done
        done
    done
done
