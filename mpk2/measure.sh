#!/bin/bash -x

function proc_file {
    NAME=$(echo $(basename $1) | sed s/\.tar\.gz//)
    tar xzf $1
    CNAME=$(echo ${NAME} | sed s/_/-/g)
    mv ${NAME}/${NAME}.mtx ${CNAME}.mtx
    rm -rf ${NAME}

    npart=16
    for lvl in $(seq 10 10 20); do # 10 20 .. 50
        for phs in 0 1 2 3; do
            NPART=${npart} NPHASE=${phs} NLEVEL=${lvl} ./test_mtx.sh ${CNAME}.mtx
            rm -rf ${CNAME}_${npart}_${lvl}_${phs}
        done
    done
    rm -rf ${CNAME}*
}

function proc_dir {
     for file in $(ls $1); do
        FULL=$1/$file
        proc_file ${FULL}
    done
}

BASEDIR=~/Downloads
proc_dir ${BASEDIR}/mincom-paper
proc_dir ${BASEDIR}/opt-paper
proc_dir ${BASEDIR}/both-paper
