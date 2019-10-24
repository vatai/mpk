#pragma once

#include <map>
#include <vector>
#include <utility>

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

typedef std::map<src_tgt_t, std::vector<idx_t>> comm_dict_t;

typedef std::map<src_tgt_t, std::vector<sidx_tidx_t>> init_dict_t;
