#ifndef _MPI_LIB_H_
#define _MPI_LIB_H_

#include "lib.h"

typedef struct {
  mpk_t *mg;
  int n;
  int nlevel;
  int npart;
  int nphase;
  // Each phase communicates a different amount, hence pointer to
  // pointer.  E.g. `vv_sbufs[phase][i]` is the `i`-th element of the
  // send buffer in phase `p`

  // Receive and send buffers for vertex values (from vv).
  double **vv_rbufs;
  double **vv_sbufs;
  // Receive and send buffers for (vv) indices.
  long **idx_rbufs;
  long **idx_sbufs;

  // `sendcounts[phase * npart + p]` and `recvcounts[phase * npart +
  // p]` are is the number of elements sent/received to/from partition
  // `p`.
  int *phase_rcnt;
  int *phase_scnt;
  int *recvcounts;
  int *sendcounts;

  // `sdispls[phase * npart + p]` and `rsdispls[phase * npart + p]` is
  // the displacement (index) in the send/receive buffers where the
  // elements sent to partition/process `p` start.
  int *rdispls;
  int *sdispls;

  long **mptr;
  long **mcol;
  double **mval;

  long *idx_buf;
} comm_data_t;

void mpi_exec_mpk(mpk_t *mg, double *vv, comm_data_t *cd, char *dir);

void mpi_prep_mpk(comm_data_t *);

comm_data_t *new_comm_data(mpk_t *);

void del_comm_data(comm_data_t *);

void print_values_of_vv(int rank, int phase, int n, int nlevel, double *vv, char *dir);

#endif
