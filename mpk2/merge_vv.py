"""Merge text log files. """

import sys
import re


def open_files():
    """Open files. """
    files = []
    for fname in sorted(sys.argv[1:]):
        print("Opening {}".format(fname))
        files.append(open(fname, 'r'))

    for i, file in enumerate(files):
        fname0 = file.name
        fname1 = re.sub(r'[0-9]', str(i), sys.argv[1])
        assert fname0 == fname1

    return files


def all_same(lines):
    line = lines[0]
    for other in lines:
        if other != line:
            return False
    return True


def proc_lines(lines):
    mf = None
    for line in lines:
        floats = map(float, filter(lambda t: t != '', line.split(" ")))
        floats = list(floats)
        if mf is None:
            mf = floats
        else:
            for i, m in enumerate(mf):
                mf[i] = max(floats[i], m)
    return ", ".join(map(str, mf))


files = open_files()

all_lines = [file.readlines() for file in files]

transposed = zip(*all_lines)

for lines in transposed:
    if all_same(lines):
        print(lines[0][:-1])
    else:
        mline = proc_lines(lines)
        print(mline)
