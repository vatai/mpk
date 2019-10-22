#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

#include <utility>

#include <metis.h>

typedef unsigned short level_t;
typedef std::pair<idx_t, idx_t> src_tgt_t;
typedef std::pair<idx_t, level_t> idx_lvl_t;

#endif

