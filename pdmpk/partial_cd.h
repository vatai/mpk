/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-09-17
 *
 * @brief Communication data for with partial vertices.
 *
 * @todo(vatai): maybe: encapsulate mpi data
 * @todo(vatai): maybe: encapsulate parameters
 */

#ifndef _PARTIAL_CD_
#define _PARTIAL_CD_

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
  std::map<std::pair<idx_t, level_t>, idx_t> store_part;
  std::vector<level_t> levels;
  std::vector<bool> partials;

  std::vector<buffers_t> bufs;

private:
  void debug_print_levels(std::ostream &os);
  void debug_print_partials(std::ostream &os);
  void debug_print_partitions(std::ostream &os);
  void debug_print_report(std::ostream &os, const int phase);

  void init_vectors();
  void init_communication();
  void update_levels();
  bool proc_vertex(const idx_t idx, const level_t lbelow);
  void proc_adjacent(const idx_t idx, const level_t lbelow, const idx_t t);
  void record_adjacent(const idx_t idx, const idx_t t, const idx_t adj_buf_idx);
  bool can_add(const idx_t idx, const level_t lbelow, const idx_t t);
  idx_t get_adj_buf_idx(const idx_t part, const idx_t idx, const level_t level);
  void set_store_part(const idx_t idx, const level_t level, const idx_t part);
  idx_t get_store_part(const idx_t idx, const level_t level);
  void update_data(const idx_t idx, const level_t level);
  void update_weights();

  bool partial_is_full(const idx_t idx);
  void partial_reset(const idx_t idx);

  void metis_partition();
  void metis_partition_with_levels();
};

#endif
