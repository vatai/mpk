#!/bin/bash -x

BASEDIR=~/Downloads
MCP=${BASEDIR}/mincom-paper
OPP=${BASEDIR}/opt-paper
BOP=${BASEDIR}/both-paper

function proc_file {
    NAME=$(echo $(basename $1) | sed s/\.tar\.gz//)
    tar xzf $1
    mv ${NAME}/${NAME}.mtx .
    for lvl in $(seq 10 10 20); do # 10 20 .. 50
        for phs in 2; do
            NPHASE=${phs} NLEVEL=${lvl} ./test_mtx.sh ${NAME}.mtx
        done
    done
    # rm -rf ${NAME}
}

function proc_dir {
    ls $1
    ls $1 | while read file; do
        FULL=${BOP}/$file
        proc_file ${FULL}
        break ## remove this
    done
}

proc_dir ${BOP}
