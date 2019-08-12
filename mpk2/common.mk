default : gen gen2 gen2mtx comp skirt format driver stat mpktest mpi_mpktest mpi2_mpktest mpkrun

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
gen2mtx : lib.o

MPKOBJ = lib.o mpkread.o mpkprep.o mpkexec.o mpktexec.o spmvexec.o
mpktest : mpktest.o $(MPKOBJ)

MPIMPKOBJ = mpi_mpkprep.o mpi_mpkexec.c
mpi_mpktest : mpi_mpktest.o $(MPIMPKOBJ) $(MPKOBJ)

MPI2MPKOBJ = mpi2_mpkprep.o mpi2_mpkexec.o buffers.o
mpi2_mpktest : mpi2_mpktest.o $(MPI2MPKOBJ) $(MPKOBJ)

mpkrun : mpkvexec.o lib.o mpkread.o mpkprep.o spmvexec.o
