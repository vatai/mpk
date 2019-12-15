/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-12-01

/// @todo(vatai): Make these types more verbose using `struct`s. Also
/// it might be a good idea to stick to bla_bla_map naming
/// conventions.
#pragma once

#include <map>
#include <set>
#include <utility>
#include <vector>

#include <metis.h>

typedef unsigned short level_t;

/// (source, target) pair.
typedef std::pair<idx_t, idx_t> src_tgt_t;
/// (source index, target index) pair.
typedef std::pair<idx_t, idx_t> sidx_tidx_t;

/// (partition, source index) pair.
typedef std::pair<idx_t, idx_t> part_sidx_t;

/// (index, level) pair.
typedef std::pair<idx_t, level_t> idx_lvl_t;

typedef std::map<idx_lvl_t, part_sidx_t> StorePart;
