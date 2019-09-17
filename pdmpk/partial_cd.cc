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
  // `was_active` is set to true, if there was progress made. If no
  // progress is made, we should not proceed to the next level.
  int was_active = true;
  for (int level = 0; was_active and level < nlevels; level++) {
    was_active = false;
    for (int idx = 0; idx < crs.n; idx++) {
      const bool needs_calculation = levels[idx] < level + 1;
      if (needs_calculation) {
        was_active = was_active or proc_vertex(idx, level);
      }
    }
  }
}

// Process vertex v[idx] at level `level`.
bool partial_cd::proc_vertex(const idx_t idx, const level_t level)
{
  /**
   * TODO(vatai): This procedure should be called only if the vertex
   * is in the current partitions.
   *
   * Vertex with index at level >= 0 is processed.
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

  bool retval = false;
  for (idx_t t = crs.ptr[idx]; t < crs.ptr[idx + 1]; t++) {
    if (vertex_needed(idx, level, t) and vertex_available(idx, level, t)) {
      add_vertex(idx, level, t);
      retval = true;
    }
  }
  return retval;
}

// TODO(vatai): needs implementation
bool partial_cd::vertex_needed(const idx_t idx, const level_t level, const idx_t t)
{
  const bool level_check = level;
  return true;
}

// Return true if, the vertex `j`, at `level - 1` is computed and in
// the same partition as idx.
bool partial_cd::vertex_available(const idx_t idx, const level_t level, const idx_t t)
{
  const idx_t j = crs.col[t];
  const bool same_part = partitions[idx] == partitions[j];
  const bool computed = levels[j] >= level - 1;
  return same_part and computed;
}

// TODO(vatai): needs implementation
void partial_cd::add_vertex(const idx_t idx, const level_t level, const idx_t t)
{
  // 1. Adds it to partials[idx] (increment level[idx] if
  // partials[idx] are full).

  // TODO(vatai): partials[idx].add(t - ptr[idx]);

  // 2. Update store_part[idx].

  // TODO(vatai): store_part[idx][k] = partition[idx];

  // TODO(vatai): if (partials[idx].full()) { partials[idx].empty();
  // levels[idx]++;}

  // 3. Add v[t, k] to the buffers.
}

// TODO(vatai): needs implementation
void partial_cd::update_weights()
{
  //
}

bool partial_cd::partial_is_full()
{
  for (auto v : partials) {
    if (not v)
      return false;
  }
  return true;
}

void partial_cd::partial_reset(const idx_t idx)
{
  for (int t = crs.ptr[idx]; t < crs.ptr[idx + 1]; t++) {
    partials[t] = false;
  }
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
