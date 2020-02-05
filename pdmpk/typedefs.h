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

/// (source index, target index) pair.
typedef std::pair<idx_t, idx_t> sidx_tidx_t;
