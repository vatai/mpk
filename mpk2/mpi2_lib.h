#ifndef _MPI_LIB_H_
#define _MPI_LIB_H_

#include "lib.h"

typedef struct {
  int n;
  int nlevel;
  int npart;
  int nphase;
  // Each phase communicates a different amount, hence pointer to
  // pointer.  E.g. `vv_sbufs[phase][i]` is the `i`-th element of the
  // send buffer in phase `p`

  // Send and receive buffers for vertex values (from vv).
  double **vv_sbufs;
  double **vv_rbufs;
  // Send and receive buffers for (vv) indices.
  long **idx_sbufs;
  long **idx_rbufs;

  // `sendcounts[phase * npart + p]` and `recvcounts[phase * npart +
  // p]` are is the number of elements sent/received to/from partition
  // `p`.
  int *sendcounts;
  int *recvcounts;

  // `sdispls[phase * npart + p]` and `rsdispls[phase * npart + p]` is
  // the displacement (index) in the send/receive buffers where the
  // elements sent to partition/process `p` start.
  int *sdispls;
  int *rdispls;

  int **mptr;
  int **mcol;
  // double **mval;
} comm_data_t;

void mpi_exec_mpk(mpk_t *mg, double *vv, comm_data_t *cd, char *dir);

void mpi_prep_mpk(mpk_t*, comm_data_t *);

void mpi_del_cd(comm_data_t *);

void print_values_of_vv(int rank, int phase, int n, int nlevel, double *vv, char *dir);

#endif
