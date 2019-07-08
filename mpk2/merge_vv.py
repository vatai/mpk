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
    cur_floats = None
    for file_lines in lines:
        arr_line = file_lines[i].split(" ")
        filtered = list(filter(lambda t: t != '', arr_line))
        filtered[-1] = filtered[-1][:-1]
        if '>' in filtered or '' in filtered:
            print(lines[0][i][:-1])
        else:
            floats = map(float, filtered)
            if cur_floats is None:
                cur_floats = list(floats)
            else:
                for j in range(len(cur_floats)):
                    cur_floats[j] = max(cur_floats[j], floats[j])
            line = " ".join(map(str, cur_floats))
            print(line)
