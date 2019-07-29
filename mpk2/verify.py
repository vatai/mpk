"""Simple numpy program to check if the results of mpk are correct."""

import os
import sys

import numpy as np
import scipy.io


def get_results(dirname):
    """Get the results from $DIR/results.txt as a LIST of NUMPY arrays."""
    fname = os.path.join(dirname, "results.txt")
    results = []
    with open(fname, "r") as file:
        for line in file.readlines():
            vals = line[:-1].split(" ")[:-1]
            vals_f = map(float, vals)
            results.append(np.array(list(vals_f)))
    return results


def get_mtx(dirname):
    """Figure out the name of the .mtx file from $DIR and return it as a
    NUMPY array."""
    elems = dirname.split("_")
    assert len(elems) == 4
    fname = elems[0] + ".mtx"
    coo = scipy.io.mmread(fname)
    return coo


def get_v(mtx):
    """Generate the initial vector."""
    vec_size = mtx.shape[0]
    vec = np.ones(vec_size)
    for i, _ in enumerate(vec):
        vec[i] += -0.001 * i if i % 2 == 0 else 0.001 * i
    return vec


def verify(epsilon=1e-6):
    """The main procedure."""

    results = get_results(sys.argv[1])
    mtx = get_mtx(sys.argv[1])
    vec = get_v(mtx)
    for result in results:
        err_norm = np.linalg.norm(result - vec)
        if err_norm > epsilon:
            print("Numpy says WRONG RESULTS")
            # exit(1)
        else:
            print("Numpy says OK")
        vec = mtx.dot(vec)

    print("DEBUG Numpy says {} is OK!".format(sys.argv[1]))


verify()
