#ifndef _MPI_LIB_H_
#define _MPI_LIB_H_

#include "lib.h"

typedef struct comm_table {
  mpk_t *mg;
  int n;
  int nlevel;
  int npart;
  int nphase;
  int rank;

  // npart * (nphase + 1)
  int *recvcounts;
  int *sendcounts;
  int *rdispls;
  int *sdispls;

  // nphase + 1
  int *rcount;
  int *mcount;
  int *scount;

  // {r,m,s}count[phase]
  long **idx_rbufs;
  long **idx_mbufs;
  long **idx_sbufs;
  double **vv_rbufs;
  double **vv_mbufs;
  double **vv_sbufs;

  int buf_count;
  long *idx_buf; // Important note in iterator() function comments
  double *vv_buf;

  long **mptr;
  long **mcol;
  double **mval;

} comm_data_t;

void mpi_exec_mpk(comm_data_t *cd);

void mpi_prep_mpk(comm_data_t *);

comm_data_t *new_comm_data(mpk_t *);

void del_comm_data(comm_data_t *);

#endif
