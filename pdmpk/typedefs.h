/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2019-12-01

#pragma once

#include <map>
#include <set>
#include <utility>
#include <vector>

#include <metis.h>

typedef short level_t;

/// (source index, target index) pair.
typedef std::pair<idx_t, idx_t> sidx_tidx_t;
