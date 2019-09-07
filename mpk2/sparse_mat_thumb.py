#!/usr/bin/python

import sys

import matplotlib.pyplot as plt
import scipy.io
from PIL import Image


def proc_file(fname):
    mmat = scipy.io.mmread(fname)
    mat = mmat.tocsr()
    plt.spy(mat, markersize=1.0)
    plt.tick_params(
        # axis='x',          # changes apply to the x-axis
        which="both",  # both major and minor ticks are affected
        bottom=False,  # ticks along the bottom edge are off
        top=False,  # ticks along the top edge are off
        left=False,
        right=False,
        labelbottom=False,
        labelleft=False,
    )  # labels along the bottom edge are off
    fig = plt.gcf()
    fig.set_size_inches(1, 1)
    name = fname.split(".")[0]
    fig.savefig("{}.png".format(name), dpi=48)


proc_file(sys.argv[1])
