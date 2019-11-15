CXX=mpic++
METISDIR=metis/build/Linux-x86_64
CXXFLAGS=-std=c++14 -Wall -pedantic -I$(METISDIR)/include -L$(METISDIR)/libmetis -g
LDLIBS=-lmetis

all : pdmpk_prep pdmpk_exec

pdmpk_prep : partial_cd.o csr_t.o buffers_t.o mpi_bufs_t.o mcsr_t.o pdmpk_bufs_t.o
pdmpk_exec : buffers_t.o mpi_bufs_t.o mcsr_t.o
