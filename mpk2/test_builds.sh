#!/bin/bash
# !KEEP IN SYNC WITH MAKEFILE!
#
# Simple script to build and test al programs.
#
# TODO(vatai): Expand this file with further tests, possibly for the
# time of writing mpi_mpktest, check if bot bot mpktest versions
# perform give the same results.

NAME=three
SIZE=3
NPART=4
NLEVEL=10
NPHASE=5

MAKEFILE=makefile
which mpiicc 2>/dev/null 1>/dev/null && MAKEFILE+=.intel || MAKEFILE+=.gcc
# Force build all.
make -f $MAKEFILE -B

# Delete input files.
echo rm -rf ${NAME}${SIZE}_${NPART}_${NLEVEL}_${NPHASE}
rm -rf ${NAME}${SIZE}_${NPART}_${NLEVEL}_${NPHASE}

# Check `gen`, `driver` and `mpktest`
# usage: ./gen type size ghead
# usage: ./driver ghead npart nlevel nphase
echo test_builds.sh: generating data
./gen m5p $SIZE $NAME$SIZE && ./driver $NAME$SIZE $NPART $NLEVEL $NPHASE

echo test_builds.sh: testing OpenMP version
# Check OpenMP version
./mpktest ${NAME}${SIZE}_${NPART}_${NLEVEL}_${NPHASE}

echo test_builds.sh: testing MPI version
# Check MPI version
mpirun ./mpi_mpktest ${NAME}${SIZE}_${NPART}_${NLEVEL}_${NPHASE}
