#ifndef _BUFFERS_H_
#define _BUFFERS_H_

#include "comm_data.h"

typedef struct buffers_t {
  int n;
  int npart;
  int nlevel;
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

  // nphase + 1
  int *rbuf_offsets;
  int *mbuf_offsets;
  int *sbuf_offsets;

  int buf_count;
  int buf_scount;
  long *idx_buf; // Important note in iterator() function comments
  long *idx_sbuf;
  double *vv_buf;
  double *vv_sbuf;

  // nphase + 1
  long **mptr; // cd->mcount[phase] + 1
  long **mcol; // cd->mptr[phase][cd->mcount[phase]]
  double **mval;
} buffers_t;

buffers_t *new_bufs(comm_data_t *);

void del_bufs(buffers_t *);

void fill_buffers(comm_data_t *, buffers_t *);

void mpi_exec_mpk(buffers_t *);

void write_buffers(buffers_t *, char *);

buffers_t *read_buffers(char *);

#endif
