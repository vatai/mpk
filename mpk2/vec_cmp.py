#!/usr/bin/python

#  Author: Emil VATAI <emil.vatai@gmail.com>
#  Date: 2019-08-19
#
# A quick and dirty (numerical) comparison of binary files (prints the
# norms of the errors/diffs).

import struct
import sys

import numpy as np


def read_arr(n, arr, file):
    """Read values to `arr` from `file`."""
    for i in range(n):
        arr[i] = struct.unpack("d", file.read(8))[0]


# vf = open("bcsstk28_2_10_3/results.bin", "rb")
# gf = open("bcsstk28_2_10_3/gold_result.bin", "rb")
vname = sys.argv[3]
gname = sys.argv[4]  # gold
print("vname {}, gname {}".format(vname, gname))
vf = open(vname, "rb")
gf = open(gname, "rb")

# n, nlevel = 4410, 10
n = int(sys.argv[1])
nlevel = int(sys.argv[2])
vect = np.zeros(n)
gold = np.zeros(n)

for _ in range(nlevel + 1):
    read_arr(n, vect, vf)
    read_arr(n, gold, gf)
    norm = np.linalg.norm(vect - gold)
    print(norm)
