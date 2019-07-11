#!/bin/bash
# !KEEP IN SYNC WITH MAKEFILE!
#
# Simple script to build and test al programs.
#
# TODO(vatai): Expand this file with further tests, possibly for the
# time of writing mpi_mpktest, check if both mpktest versions give the
# same results.

NAME=mesh5p
SIZE=10
NPART=16
NLEVEL=100
NPHASE=6

rm *.log

MAKEFILE=makefile
which mpiicc 2>/dev/null 1>/dev/null && MAKEFILE+=.intel || MAKEFILE+=.gcc
# Force build all.
make -f $MAKEFILE -B || exit

# Delete input files.
echo rm -rf ${NAME}${SIZE}_${NPART}_${NLEVEL}_${NPHASE}
rm -rf ${NAME}${SIZE}_${NPART}_${NLEVEL}_${NPHASE}

# Check `gen`, `driver` and `mpktest`
# usage: ./gen type size ghead
# usage: ./driver ghead npart nlevel nphase
echo test_builds.sh: generating data
./gen m5p $SIZE $NAME$SIZE && ./driver $NAME$SIZE $NPART $NLEVEL $NPHASE

echo test_builds.sh: testing OpenMP version
DIRNAME=${NAME}${SIZE}_${NPART}_${NLEVEL}_${NPHASE}
# Check OpenMP version
./mpktest $DIRNAME

echo test_builds.sh: testing MPI version
# Check MPI version
mpirun -n $NPART ./mpi_mpktest $DIRNAME

# POST PROCESSING
#
# Assume 2d 5point mesh.  The level and partition of vertices is
# written 1 vertex per line.  Rearrange the level and partition values
# in a way that resembles the original 2d mesh.
for file in $(ls $DIRNAME/l[0-9]* $DIRNAME/g*part*); do
    perl -lne 'if ($. % '$SIZE' == 0) {print "$p $_"; $p=""} else { $p="$p $_"}' $file > $file.pp
done

python merge_vv.py vv_after_mpi_exec_rank*.log
