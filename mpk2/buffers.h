/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-10-31

#ifndef _BUFFERS_H_
#define _BUFFERS_H_

#include "comm_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Buffers for processing.
/// @details Contains the buffers
/// created by the by the `mpi2_mpkprep.c` program and processed by
/// the
typedef struct buffers_t {
  int n;
  int npart;
  int nlevel;
  int nphase;
  int rank;
  int buf_count;
  int buf_scount;
  int mptr_count;
  int mcol_count;

  // npart * (nphase + 1)
  int *recvcounts;
  int *sendcounts;
  int *rdispls;
  int *sdispls;

  // nphase + 1
  int *rcount;
  int *mcount;
  int *scount;
  int *rbuf_offsets;
  int *mbuf_offsets;
  int *sbuf_offsets;
  int *mptr_offsets;
  int *mcol_offsets;

  long *idx_buf; // Important note in iterator() function comments
  long *idx_sbuf;
  long *mptr_buf;
  long *mcol_buf;

  double *vv_buf;
  double *vv_sbuf;
  double *mval_buf;
} buffers_t;

void check_args(int argc, char *argv0);

buffers_t *new_bufs(comm_data_t *);

void alloc_val_bufs(buffers_t *);

void del_bufs(buffers_t *);

void fill_buffers(comm_data_t *, buffers_t *);

void write_buffers(buffers_t *, char *);

buffers_t *read_buffers(char *, int);

#ifdef __cplusplus
}
#endif

#endif
