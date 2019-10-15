#pragma once
#include <vector>

#include <metis.h>

struct mcsr_t {
  std::vector<idx_t> mptr;
  std::vector<size_t> mptr_count;
  std::vector<size_t> mptr_offset;

  std::vector<idx_t> mcol;
  std::vector<size_t> mcol_count;
  std::vector<size_t> mcol_offset;
};
