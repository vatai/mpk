#!/bin/bash
#SBATCH -p p
#SBATCH -N 2
#
# Simple script to build and test all programs.
#
# Set these variables to modify them
NAME=${NAME:=mesh5p}
SIZE=${SIZE:=100}
NPART=${NPART:=2}
NLEVEL=${NLEVEL:=10}
NPHASE=${NPHASE:=6}

DIRNAME=${NAME}${SIZE}_${NPART}_${NLEVEL}_${NPHASE}
echo dirname is $DIRNAME


# Remove logs.
rm *.log

# Force build all.
# make || exit -1

# Delete input files.
rm -rf ${NAME}${SIZE}_${NPART}_${NLEVEL}_${NPHASE}

# Check `gen`, `driver` and `mpktest`
# usage: ./gen type size ghead
# usage: ./driver ghead npart nlevel nphase
# Old ./gen needs to be run for ./driver (metis doesn't support loops).
srun -N 1 ./gen m5p $SIZE $NAME$SIZE && srun -N 1 ./driver $NAME$SIZE $NPART $NLEVEL $NPHASE 1>/dev/null || exit 1
# Our program supports loops, so overwrite the g0 file.
srun -N 1 ./gen2 m5p $SIZE $NAME$SIZE && cp $NAME$SIZE.loop.g0 $DIRNAME/loop.g0 || exit 2

# POST PROCESSING
#
# Assume 2d 5point mesh.  The level and partition of vertices is
# written 1 vertex per line.  Rearrange the level and partition values
# in a way that resembles the original 2d mesh.
for file in $(ls $DIRNAME/l[0-9]* $DIRNAME/g*part*); do
    perl -lne 'if ($. % '$SIZE' == 0) {print "$p $_"; $p=""} else { $p="$p $_"}' $file > $file.pp
done

set -x # Uncomment this line to make verbose.
# OpenMP version
# ./mpktest $DIRNAME || exit 3

# MPI2 version: read/write buffers
srun -N $NPART mpirun -n $NPART ./mpi2_mpkwrtbufs $DIRNAME 1>/dev/null || exit 10
srun -N $NPART mpirun -n $NPART ./mpi2_mpkexecbufs $DIRNAME || exit 11
srun -N $NPART mpirun -n $NPART ./mpi2_mpkexecbufs_val $DIRNAME || exit 12
# ./mpkrun $DIRNAME || exit 13
./verify_val $DIRNAME || exit 14
diff $DIRNAME/gold_result.bin $DIRNAME/results.bin
# python merge_vv.py $DIRNAME/vv_after_mpi_exec_rank*.log || exit