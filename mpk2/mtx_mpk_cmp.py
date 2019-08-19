#!/usr/bin/python

#  Author: Emil VATAI <emil.vatai@gmail.com>
#  Date: 2019-08-19

# Quick and dirty implementation of#
#
# - a numpy MPK algorithm (using scipy.io.mmread()) and
#
# - comparison between the binary result file.

import struct
from os.path import expanduser

# import matplotlib.pyplot as plt
import numpy as np
import scipy.io

matname = expanduser("LFAT5.mtx")
mat = scipy.io.mmread(matname)

M = mat.tocsr().toarray()

# plt.imshow(M)
# plt.show()

n, _ = M.shape
nlev = 10

# fname = "LFAT5_2_10_3/gold_result.bin"
fname = "LFAT5.mtx.gold.bin"
file = open(fname, "rb")
curv = np.ones(n)
filev = np.zeros(n)

print(M)
for i in range(n):
    rv = struct.unpack("d", file.read(8))[0]
    filev[i] = rv

print("iter[{}] = {}".format(0, np.linalg.norm(curv - filev)))

for lvl in range(nlev):
    curv = mat.dot(curv)
    for i in range(n):
        filev[i] = struct.unpack("d", file.read(8))[0]
    print("iter[{}] = {}".format(lvl + 1, np.linalg.norm(curv - filev)))
