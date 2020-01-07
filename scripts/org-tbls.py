#!/usr/bin/python
# Author: Emil VATAI <emil.vatai@gmail.com>
# Date: 2020-01-07

# Script to make .org files from old results.


import os
import sys


def header(name, n):
    return (
        f"* {name}, $n={n:,}$\n" + "  | $P$ | $m$ | $s$ | comm | red.calc | mincalc |\n"
    )


def proc_line(line):
    arr = line.split(":")
    logfn, arr = arr
    logfn = logfn[8:-4].split("_")
    arr = arr[:-1].split(" ")
    arr = filter(lambda t: t != "", arr)
    comm, calc, mincalc, nnlvl = map(int, arr)
    redcalc = calc - mincalc
    npart, nlevel, nphase = map(int, logfn[1:])
    n = int(nnlvl / nlevel)
    return npart, nlevel, nphase, comm, redcalc, mincalc, n


def proc_file(fname):
    data = []
    with open(fname, "r") as file:
        for line in file.readlines():
            data.append(proc_line(line))
    return data


def make_org(name, data):
    fname = os.path.basename(f"{name}.org")
    n = data[0][-1]
    result = header(fname[:-4], n)
    for npart, nlevel, nphase, comm, redcalc, mincalc, n in data:
        if nphase == 0:
            result += "  |---\n"
        s = f"  | {npart:2} | {nlevel:3} | {nphase:2} | {comm:10,} | {redcalc:11,} | {mincalc:14,} |\n"
        result += s
    with open(fname, "w") as file:
        file.write(result)


def main(fname):
    name = fname[:-4]
    data = proc_file(fname)

    make_org(name, data)


for fname in sys.argv[1:]:
    main(fname)
