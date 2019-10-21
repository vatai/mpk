/**
 * @author Emil VATAI <emil.vatai@gmail.com>
 * @date 2019-10-21
 */

#pragma once
#include <iostream>
#include <vector>
#include <metis.h>

#include "typedefs.h"
#include "csr_t.h"

class pdmpk_bufs_t {
 public:
  pdmpk_bufs_t(const csr_t &_csr, const idx_t npart);

  level_t min_level();
  bool can_add(const idx_t idx, const level_t lbelow, const idx_t t);
  void inc_level(const idx_t idx);
  void update_weights();

  bool partial_is_full(const idx_t idx);
  void partial_reset(const idx_t idx);

  void metis_partition();
  void metis_partition_with_levels();

  void debug_print_levels(std::ostream &os);
  void debug_print_partials(std::ostream &os);
  void debug_print_partitions(std::ostream &os);
  void debug_print_report(std::ostream &os, const int phase);

  std::vector<idx_t> partitions;
  std::vector<idx_t> weights;
  std::vector<level_t> levels;
  std::vector<bool> partials;

 private:
  const csr_t &csr;
  const idx_t npart;
  pdmpk_bufs_t();
};
