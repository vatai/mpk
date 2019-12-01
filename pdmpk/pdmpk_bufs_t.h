/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-10-21

#pragma once
#include <iostream>
#include <metis.h>
#include <vector>

#include "CSR.h"
#include "typedefs.h"

class pdmpk_bufs_t {
public:
  pdmpk_bufs_t(const CSR &_csr);

  level_t min_level();
  bool can_add(const idx_t idx, const level_t lbelow, const idx_t t);
  void inc_level(const idx_t idx);
  void update_weights();

  bool partial_is_full(const idx_t idx) const;
  bool partial_is_empty(const idx_t idx) const;
  void partial_reset(const idx_t idx);

  void metis_partition(idx_t npart);
  void metis_partition_with_levels(idx_t npart);

  void debug_print_levels(std::ostream &os);
  void debug_print_partials(std::ostream &os);
  void debug_print_partitions(std::ostream &os);
  void debug_print_report(std::ostream &os, const int phase);

  std::vector<bool> partials;
  std::vector<idx_t> partitions;
  std::vector<level_t> levels;

private:
  std::vector<idx_t> weights;
  const CSR &csr;
  pdmpk_bufs_t();
};
