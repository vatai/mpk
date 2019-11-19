#!/bin/bash
make -B && ./pdmpk_prep m5p4.loop.mtx 4 4 && orterun -n 4 ./pdmpk_exec
