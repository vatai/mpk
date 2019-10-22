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

  pdmpk_bufs_t pdmpk_bufs;
  /// Map (vector index, level) pair to the (partition, mbuf index)
  /// pair where it is can be found.
  std::map<idx_lvl_t, from_to_t> store_part;
  /// In each phase, collect the communication as a map between from
  /// (source, target) to `mbuf` index in the source partition.
  std::map<std::pair<idx_t, idx_t>, idx_t> comm_dict;
  /// All the buffers such as `mbuf`, `mcsr` and and MPI buffers.
  std::vector<buffers_t> bufs;

 private:
  void phase_init();
  void phase_finalize();
  void init_communication();

  void update_levels();
  bool proc_vertex(const idx_t idx, const level_t lbelow);
  void proc_adjacent(const idx_t idx, const level_t lbelow, const idx_t t);

  void rec_comm(const idx_t cur_part, const std::pair<idx_t, idx_t> &pair);

  void set_store_part(const idx_t idx, const level_t level, const idx_t part);
  std::pair<idx_t, idx_t> get_store_part(const idx_t idx, const level_t level);

  int phase;
  /// `cur_part` is set to the partition of the vertex being processed
  /// at the beginning of `proc_vertex`.
  idx_t cur_part;
};
