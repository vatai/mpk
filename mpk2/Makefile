# CC=$(shell which mpiicc 1>/dev/null 2>/dev/null && echo mpiicc || echo mpicc)
# NOTTESTED CC=$(shell which mpiicpc 1>/dev/null 2>/dev/null && echo mpiicpc || echo mpicxx)
CC=mpicxx
CFLAGS = -g -fopenmp
LDLIBS = -fopenmp -lm

UTILS = gen gen2 gen2mtx mtx2gen
MPKBASE = comp skirt format driver stat verify_val work_count
OMPBIN = mpktest mpkrun
MPI1BIN = $(MPKBASE) mpi_mpktest
MPI2BIN = $(MPKBASE) mpi2_mpktest mpi2_mpkwrtbufs mpi2_mpkexecbufs mpi2_mpkexecbufs_val

default : $(UTILS) $(MPI2BIN)

clean :
	rm -rf $(BIN) *.o *~ *.log

gen : gen.c

# lib.o : lib.c lib.h
# comp.o : comp.c lib.h
# skirt.o : skirt.c lib.h
# format.o : format.c lib.h
# stat.o : stat.c lib.h
# driver.o : driver.c lib.h

# comm_data.o : comm_data.h lib.h
# buffers.o : buffers.h comm_data.h lib.h

# binaries
comp : comp.o lib.o
skirt : skirt.o lib.o
format : format.o lib.o
stat : stat.o lib.o
driver : driver.o
gen2mtx : lib.o
mtx2gen : mmio.o lib.o

MPKOBJ = lib.o mpkread.o mpkprep.o mpkexec.o mpktexec.o spmvexec.o
mpktest : mpktest.o $(MPKOBJ)

mpi_mpktest : mpi_mpkprep.o mpi_mpkexec.o lib.o mpkread.o mpkprep.o spmvexec.o
mpi2_mpktest : mpi2_mpktest.o mpi2_mpkexec.o comm_data.o buffers.o lib.o reduce_comm.o
mpkrun : mpkvexec.o lib.o mpkread.o mpkprep.o spmvexec.o
mpi2_mpkwrtbufs : buffers.o comm_data.o lib.o reduce_comm.o
mpi2_mpkexecbufs : buffers.o comm_data.o mpi2_mpkexec.o lib.o reduce_comm.o
mpi2_mpkexecbufs_val : buffers.o comm_data.o mpi2_mpkexec.o lib.o reduce_comm.o
verify_val : comm_data.o lib.o
work_count : comm_data.o buffers.o lib.o reduce_comm.o
