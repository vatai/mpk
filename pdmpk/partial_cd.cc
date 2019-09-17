//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-09-17

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "partial_cd.h"
#include "metis.h"

partial_cd::partial_cd(const char *_fname, const int _rank, const int _world_size,
                       const idx_t _npart, const level_t _nlevels)
    : crs{_fname}, rank{_rank}, world_size{_world_size}, npart{_npart}, nlevels{_nlevels}
{
  partitions.resize(crs.n);
  levels.resize(crs.n);
  partials.resize(crs.n);

  metis_partition();
  update_levels();
}

void partial_cd::update_levels()
{
  int was_active = true;
  for (int k = 0; was_active and k < nlevels; k++) {
    was_active = false;
    for (int i = 0; i < crs.n; i++) {
      was_active = was_active or proc_vertex(i, k + 1);
    }
  }
}

bool partial_cd::proc_vertex(const idx_t idx, const level_t level)
{
  /**
   * TODO(vatai): this procedure should be called only if the vertex
   * is in the current partitions.
   *
   * Vertex with index at level > 0 is processed.
   *
   * The procedure visits all neighbours of v[idx] and if they are in
   * the current partition it adds them.
   *
   * Adding a neighbour means:
   *
   * - add it to (levels, partials)
   *
   * - add it to buffers
   *
   * - update store_partition
   */
  const idx_t curpart = partitions[idx];
  for (idx_t t = crs.ptr[idx]; t < crs.ptr[idx + 1]; t++) {
    const idx_t jth = t - crs.ptr[idx];
    const idx_t j = crs.col[t];
    const idx_t jpart = partitions[j];
    if (vertex_needed(idx, level, j) and vertex_available(idx, level, j))
      add_vertex(idx, level, j);
  }
  return true;
}

bool partial_cd::vertex_needed(const idx_t idx, const level_t level, const idx_t j)
{
  return true;
}

bool partial_cd::vertex_available(const idx_t idx, const level_t level, const idx_t j)
{
  return true;
}

void partial_cd::add_vertex(const idx_t idx, const level_t level, const idx_t j)
{
  //
}

void partial_cd::update_weights()
{
  //
}

void partial_cd::metis_partition()
{
  idx_t npart = this->npart;
  idx_t n = crs.n;
  idx_t *ptr = (idx_t *)crs.ptr.data();
  idx_t *col = (idx_t *)crs.col.data();
  idx_t retval, nconstr = 1;
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL,
                      NULL, NULL, &npart, NULL, NULL, NULL, &retval,
                      partitions.data());
}

void partial_cd::metis_partition_with_levels()
{
  idx_t npart = this->npart;
  idx_t n = crs.n;
  idx_t *ptr = (idx_t *)crs.ptr.data();
  idx_t *col = (idx_t *)crs.col.data();
  idx_t retval, nconstr = 1;
  idx_t opt[METIS_NOPTIONS];
  METIS_SetDefaultOptions(opt);
  opt[METIS_OPTION_UFACTOR] = 1000;
  METIS_PartGraphKway(&n, &nconstr, ptr, col, NULL, NULL, weights.data(),
                      &npart, NULL, NULL, opt, &retval, partitions.data());
}
