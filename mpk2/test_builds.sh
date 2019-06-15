#!/bin/bash
# !KEEP IN SYNC WITH MAKEFILE!
#
# Simple script to build and test al programs.
#
# TODO(vatai): Expand this file with further tests, possibly for the
# time of writing mpi_mpktest, check if bot bot mpktest versions
# perform give the same results.

# Force build all.
make -B

# Delete input files.
rm -rf 'five10*'

# Check `gen`, `driver` and `mpktest`
./gen m5p 10 five10 && ./driver five10 4 20 5 && ./mpktest five10_4_20_5
