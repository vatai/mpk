import struct

fname = "dbls.bin"
with open(fname, "rb") as file:
    val = struct.unpack("d", file.read(8))[0]

print("Read {} from file {}".format(val, fname))

with open(fname, "wb") as file:
    val = input("insert value: ")
    val = float(val)
    file.write(struct.pack("d", val))
