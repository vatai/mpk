/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-09-17
 *
 * Communication data for with partial vertices.
 */

#pragma once

#include <string>
#include <vector>
#include <map>

#include <metis.h>

#include "typedefs.h"
#include "pdmpk_bufs_t.h"
#include "buffers_t.h"
#include "csr_t.h"

class partial_cd {

 public:
  partial_cd(const char *fname, const idx_t npart, const level_t nlevels);

  const idx_t npart;
  const level_t nlevels;
  const csr_t csr;

  /// All MPK buffers such as levels, weights, partitions, and
  /// partials.
  pdmpk_bufs_t pdmpk_bufs;

  /// `bufs[part]` is constins all the buffers such as `mcsr` and MPI
  /// buffers for partition `part`.
  std::vector<buffers_t> bufs;

 private:
  /// Map (vector index, level) pair to the (partition, mbuf index)
  /// pair where it is can be found.
  std::map<idx_lvl_t, part_sidx_t> store_part;

  /// In each phase, collect the communication of complete indices as
  /// a map from (source, target) pairs to `mbuf` indices of the
  /// source partition.
  comm_dict_t comm_dict;

  /// In each phase, collect the communication of partial indices (for
  /// initialization) as a map from (source, target) pairs to (mbuf
  /// indices of source partition, mcol indices in the target
  /// partition) pairs.
  init_dict_t init_dict;

  void phase_init();
  void init_communication();

  void update_levels();
  bool proc_vertex(const idx_t idx, const level_t lbelow);
  void proc_adjacent(const idx_t idx, const level_t lbelow, const idx_t t);

  void phase_finalize();
  void proc_comm_dict(const comm_dict_t::const_iterator &iter,
                      const std::vector<idx_t> &count);
  void proc_init_dict(const init_dict_t::const_iterator &iter,
                      const std::vector<idx_t> &count);
  void mbuf_insert_rbuf(const idx_t src);

  void rec_mbuf_idx(const idx_lvl_t idx_lvl, const idx_t part);

  /// `src_send_base(src, tgt)` gives the base (0th index) of the send
  /// buffer in the source buffer.
  idx_t src_send_base(const sidx_tidx_t src_tgt) const;

  /// `tgt_recv_base(src, tgt)` gives the base (0th index) of the
  /// receive buffer in the target buffer.
  idx_t tgt_recv_base(const sidx_tidx_t src_tgt) const;

  /// The current phase is set at the beginning of each phase.
  int phase;
  /// `cur_part` is set to the partition of the vertex being processed
  /// at the beginning of `proc_vertex`.
  idx_t cur_part;
};
