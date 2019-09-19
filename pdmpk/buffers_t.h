#ifndef _BUFFERS_T_H_
#define _BUFFERS_T_H_

#include <forward_list>
#include <utility>
#include <map>

#include <metis.h>

#include "typedefs.h"

// static void mpk_exec_bufs_val(buffers_t *bufs) {
//   do_task(bufs, 0);
//   for (int phase = 1; phase <= bufs->nphase; phase++) {
//     do_comm(phase, bufs);
//     do_task(bufs, phase);
//   }
// }

class buffers_t {
public:
  // // npart * (nphase + 1)
  // int *recvcounts;
  // int *sendcounts;
  // int *rdispls;
  // int *sdispls;

  // // nphase + 1 // int *rcount; // int *mcount; // int *scount;

  // int *rbuf_offsets;
  // int *mbuf_offsets;
  // int *sbuf_offsets;
  // int *mptr_offsets;
  // int *mcol_offsets;

  std::forward_list<std::pair<idx_t, level_t>> pair_buf;
  // long *idx_buf; // Important note in iterator() function comments
  // long *idx_sbuf;
  std::forward_list<std::pair<idx_t, level_t>> pair_sbuf;
  // long *mptr_buf;
  // long *mcol_buf;

  // double *vv_buf;
  // double *vv_sbuf;
  // double *mval_buf;
  void dump(const int rank);
  void load(const int rank);
};

#endif