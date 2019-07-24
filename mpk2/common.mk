default : gen comp skirt format driver stat mpktest mpi_mpktest

gen : gen.c
	gcc -O3 -o gen gen.c

lib.o : lib.c lib.h
comp.o : comp.c lib.h
skirt.o : skirt.c lib.h
format.o : format.c lib.h
stat.o : stat.c lib.h
driver.o : driver.c lib.h

comp : comp.o lib.o
skirt : skirt.o lib.o
format : format.o lib.o
stat : stat.o lib.o
driver : driver.o

MPKOBJ = lib.o mpkread.o mpkprep.o mpkexec.o mpktexec.o spmvexec.o
mpktest : mpktest.o $(MPKOBJ)

MPIMPKOBJ = mpi_mpkprep.o mpi_mpkexec.c
mpi_mpktest : mpi_mpktest.o $(MPIMPKOBJ) $(MPKOBJ)
