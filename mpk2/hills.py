#!/usr/bin/python

import sys

import matplotlib.pyplot as plt
import numpy as np


def proc_file(fname):
    mat = []
    with open(fname, "r") as ppfile:
        for line in ppfile.readlines():
            arr = line[:-1].split(" ")
            arr = filter(lambda s: s != "", arr)
            arr = map(int, arr)
            mat.append(list(arr))
    return np.array(mat)


mat = proc_file(sys.argv[1])
plt.imshow(mat)
plt.show()
