#!/bin/bash
# Add "-x" after #!/bin/bash to see what's happening.

# This is an alternative implementation to driver.c since it
# (driver.c) is just a "hub/controller" program, calling other
# programs with different parameters, and a shell script seems to be
# more appropriate for this job.

function usage() {
    if [ $# -gt 0 ]; then
        >&2 echo $*
    fi
    >&2 echo "Usage: $0 ghead npart nlevel nphase"
    exit 1
}

test $# -ne 4 && usage

GHEAD=$1
NPART=$2
NLEVEL=$3
NPHASE=$4
UFACTOR=100

test $NPART  -gt 1 || usage "npart must be more than one."
test $NLEVEL -gt 1 || usage "nlevel must be more than one."
test $NPHASE -ge 0 || usage "nphase cannot be nagative."

DIRNAME=${GHEAD}_${NPART}_${NLEVEL}_${NPHASE}
mkdir $DIRNAME
G0=$DIRNAME/g0
cp $GHEAD.g0 $G0

LOGFILE=$DIRNAME/log

for PHASE in $(seq 0 $(($NPHASE - 1)) ); do
    ./gpmetis -ufactor=$UFACTOR $DIRNAME/g$PHASE $NPART > $DIRNAME/metis.log$PHASE
    if [ $PHASE -ne 0 ]; then
        PREVL="$DIRNAME/l$(($PHASE - 1))"
    fi # else it will be an empty string
    GPART=$DIRNAME/g$PHASE.part.$NPART
    LEVEL=$DIRNAME/l$PHASE
    ./comp level $G0 x $GPART $PREVL $LEVEL
    ./stat $DIRNAME/l$PHASE $NLEVEL >> $LOGFILE; tail -1 $LOGFILE
    GNEXT=$DIRNAME/g$(($PHASE + 1))
    ./comp weight $G0 x $GPART $GNEXT
done

GPART=$DIRNAME/g0.part.$NPART
SKIRT=$DIRNAME/s$NPHASE
if [ $NPHASE -ne 0 ]; then
    LEVEL=$DIRNAME/l$(($NPHASE - 1))
else
    ./gpmetis -ufactor=$UFACTOR $G0 $NPART > $DIRNAME/metis.log0
fi
./skirt $NLEVEL $G0 x $GPART $LEVEL $SKIRT
./stat ${LEVEL:=none} $SKIRT $NLEVEL >> $LOGFILE; tail -1 $LOGFILE
