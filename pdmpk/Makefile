CXX=mpicxx
METISDIR=metis/build/Linux-x86_64
CXXFLAGS=-I$(METISDIR)/include -L$(METISDIR)/libmetis -g
LDLIBS=-lmetis
#/libmetis.a

all : pdmpk_prep comm_dict_demo

pdmpk_prep : partial_cd.o csr_t.o buffers_t.o buffers_t_new.o comm_dict_t.o mpi_bufs_t.o
comm_dict_demo: comm_dict_t.o mpi_bufs_t.o mpi_bufs_t.o
