#!/bin/bash -x
# Matrix generation step 1: ../mpk2/gen2 m5p 4 m5p4
# step 2: ../mpk2/gen2mtx m5p4.loop.g0

make && \
    ./pdmpk_prep m5p4.loop.mtx 4 4 && \
    # gdb -x cmds.gdb pdmpk_prep \
    orterun -n 4 ./pdmpk_exec
