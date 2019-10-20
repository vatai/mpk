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
#include "buffers_t.h"
#include "csr_t.h"

class partial_cd {

 public:
  partial_cd(const char *_fname, const idx_t _npart, const level_t _nlevels);

  const idx_t npart;
  const level_t nlevels;
  const csr_t csr;

  std::vector<idx_t> partitions;
  std::vector<idx_t> weights;
  std::map<std::pair<idx_t, level_t>, from_to_pair_t> store_part;
  std::vector<level_t> levels;
  std::vector<bool> partials;

  std::vector<buffers_t> bufs;

  std::map<std::pair<idx_t, idx_t>, idx_t> comm_dict;

private:
  void debug_print_levels(std::ostream &os);
  void debug_print_partials(std::ostream &os);
  void debug_print_partitions(std::ostream &os);
  void debug_print_report(std::ostream &os, const int phase);

  void phase_init();
  void phase_finalize();
  void init_communication();

  void update_levels();
  bool proc_vertex(const idx_t idx, const level_t lbelow);
  void proc_adjacent(const idx_t idx, const level_t lbelow, const idx_t t);

  void rec_vert(const idx_t part);
  void rec_adj(const idx_t idx, const idx_t t, const idx_t adj_buf_idx);
  void rec_comm(const idx_t cur_part, const std::pair<idx_t, idx_t> &pair);
  bool can_add(const idx_t idx, const level_t lbelow, const idx_t t);
  void inc_level(const idx_t idx);

  void set_store_part(const idx_t idx, const level_t level, const idx_t part);
  std::pair<idx_t, idx_t> get_store_part(const idx_t idx, const level_t level);

  bool partial_is_full(const idx_t idx);
  void partial_reset(const idx_t idx);

  void update_weights();
  void metis_partition();
  void metis_partition_with_levels();

  int phase;
};
