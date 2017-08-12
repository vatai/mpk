default : gen comp skirt format komp gen2 dmpk

.PHONY: clean
clean:
	rm *~ *.o
CC = gcc

CFLAGS = -g -Imetis/include -Lbuild/metis/build/Linux-x86_64/libmetis/
CXXFLAGS = -std=c++14 -g -Imetis/include -Lbuild/metis/build/Linux-x86_64/libmetis/
LDLIBS = -lm -lmetis 

lib.o : lib.c lib.h
comp.o : comp.c lib.h
skirt.o : skirt.c lib.h
format.o : format.c lib.h
util.o : util.c util.h

komp : komp.o lib.o util.o
comp : comp.o lib.o
skirt : skirt.o lib.o
format : format.o lib.o
gen : gen.c


leveledgraph.o : leveledgraph.hpp leveledgraph.cpp
dmpk : dmpk.cpp leveledgraph.hpp leveledgraph.o
gen2 : gen2.cpp
