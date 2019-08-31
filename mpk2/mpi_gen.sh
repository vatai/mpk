#!/bin/bash
#SBATCH -p p
NAME=${NAME:=m5p}
SIZE=${SIZE:=100}
NPART=${NPART:=2}
NLEVEL=${NLEVEL:=10}
NPHASE=${NPHASE:=6}

DIRNAME=${NAME}${SIZE}_${NPART}_${NLEVEL}_${NPHASE}

# cd "$(dirname "$0")"

make || exit -1

# Delete input files.
rm -rf ${DIRNAME}

srun -N 1 ./gen $NAME $SIZE $NAME$SIZE
srun -N 1 ./gen2 $NAME $SIZE $NAME$SIZE
srun -N 1 ./driver $NAME$SIZE $NPART $NLEVEL $NPHASE 1>/dev/null || exit 1
cp $NAME$SIZE.loop.g0 $DIRNAME/loop.g0 || exit 2

# POST PROCESSING
#
# Assume 2d 5point mesh.  The level and partition of vertices is
# written 1 vertex per line.  Rearrange the level and partition values
# in a way that resembles the original 2d mesh.
for file in $(ls $DIRNAME/l[0-9]* $DIRNAME/g*part*); do
    perl -lne 'if ($. % '$SIZE' == 0) {print "$p $_"; $p=""} else { $p="$p $_"}' $file > $file.pp
done

srun -N $NPART ./run_script.sh $DIRNAME
