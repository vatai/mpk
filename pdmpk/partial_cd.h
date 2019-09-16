//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-09-17

#ifndef _PARTIAL_CD_
#define _PARTIAL_CD_

#include <string>
#include <vector>

#include <metis.h>

#include "crs_t.h"

class partial_cd {
  /**
   * TODO(vatai): assert number of neighbours fits into one of partials
   *
   * TODO(vatai): resize and init vectors with correct values
   *
   */
public:
  typedef unsigned short level_t;
  typedef unsigned long long partials_t;

  partial_cd(const char *_fname, const int _rank, const int _world_size,
             const idx_t _npart, const level_t nlevels);

  const int rank;
  const int world_size;
  const idx_t npart;
  const level_t nlevels;
  const crs_t crs;

  std::vector<idx_t> partitions;
  std::vector<idx_t> weights;
  std::vector<level_t> levels;
  std::vector<partials_t> partials;

private:
  void metis_partition();
  void metis_partition_with_levels();

  void update_levels();
  bool proc_vertex(const idx_t idx, const level_t level);
  void update_weights();
};

#endif
