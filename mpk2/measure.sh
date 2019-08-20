#!/bin/bash -x

function proc_file {
    NAME=$(echo $(basename $1) | sed s/\.tar\.gz//)
    tar xzf $1
    MTXNAME=$(echo ${NAME} | sed s/_/-/g).mtx
    mv ${NAME}/${NAME}.mtx ${MTXNAME}
    for lvl in $(seq 10 10 20); do # 10 20 .. 50
        for phs in 0 1 2 3; do
            NPART=16 NPHASE=${phs} NLEVEL=${lvl} ./test_mtx.sh ${MTXNAME}
        done
    done
    # rm -rf ${NAME}
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
