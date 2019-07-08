import sys
import glob

N = 4
fname_pattern = sys.argv[1]


def open_files(fname_pattern):
    """Open files. """
    files = []
    for fname in sorted(glob.glob(fname_pattern)):
        print("Opening {}".format(fname))
        files.append(open(fname, 'r'))

    for i, file in enumerate(files):
        fname0 = file.name
        fname1 = fname_pattern.replace('*', str(i))
        assert fname0 == fname1

    return files


files = open_files(fname_pattern)

lines = ["" for i in range(len(files))]
print("while loop")
for i, file in enumerate(files):
    lines[i] = file.readlines()

nlines = len(lines[0])

out_lines = []
for i in range(nlines):
    out_lines.append(lines[0][i])
    cur_floats = None
    for file_lines in lines:
        arr_line = file_lines[i].split(" ")
        filtered = list(filter(lambda t: t != '', arr_line))
        filtered[-1] = filtered[-1][:-1]
        try:
            floats = list(map(float, filtered))
            if cur_floats is None:
                cur_floats = floats
            else:
                for i in range(len(cur_floats)):
                    cur_floats[i] = max(cur_floats[i], floats[i])
        except:
            pass
    if cur_floats is not None:
        out_lines[-1] = " ".join(map(str, cur_floats))
        cur_floats = None

for line in out_lines:
    print(line)
