default : gen comp skirt format driver stat mpktest

#CC = gcc -O3
CC = icc -O3 -qopenmp

gen : gen.c
	gcc -O3 -o gen gen.c

lib.o : lib.c lib.h
comp.o : comp.c lib.h
skirt.o : skirt.c lib.h
format.o : format.c lib.h
stat.o : stat.c lib.h
driver.o : driver.c

mpkread.o : mpkread.c lib.h
mpktest.o : mpktest.c lib.h
mpkprep.o : mpkprep.c lib.h
mpkexec.o : mpkexec.c lib.h
mpktexec.o : mpktexec.c lib.h
spmvexec.o : spmvexec.c lib.h

comp : comp.o lib.o
	$(CC) -o comp comp.o lib.o
skirt : skirt.o lib.o
	$(CC) -o skirt skirt.o lib.o
format : format.o lib.o
	$(CC) -o format format.o lib.o
stat : stat.o lib.o
	$(CC) -o stat stat.o lib.o
driver : driver.o
	$(CC) -o driver driver.o

MPKOBJ = lib.o mpktest.o mpkread.o mpkprep.o mpkexec.o mpktexec.o spmvexec.o
mpktest : $(MPKOBJ)
	$(CC) -o mpktest $(MPKOBJ)
