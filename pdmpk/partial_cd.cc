#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "partial_cd.h"
#include "metis.h"

partial_cd::partial_cd(const char *_dir, const int _rank, const int _world_size,
                       const idx_t _npart, const level_t _nlevels)
    : dir{_dir}, rank{_rank}, world_size{_world_size}, npart{_npart}, nlevels{_nlevels}
{
  std::ifstream file{this->dir};

  mtx_check_banner(file);
  mtx_fill_size(file);
  mtx_fill_vectors(file);

  metis_partition();
  pdmpk_update_levels();
}

// metis

void partial_cd::metis_partition()
{
  idx_t retval, nconstr = 1;
  METIS_PartGraphKway(&n, &nconstr, ptr.data(), col.data(), NULL, NULL, NULL,
                      &npart, NULL, NULL, NULL, &retval, partitions.data());
}

void partial_cd::metis_partition_with_levels()
{
  idx_t retval, nconstr = 1;
  idx_t opt[METIS_NOPTIONS];
  METIS_SetDefaultOptions(opt);
  opt[METIS_OPTION_UFACTOR] = 1000;
  METIS_PartGraphKway(&n, &nconstr, ptr.data(), col.data(), NULL, NULL,
                      weights.data(), &npart, NULL, NULL, opt, &retval,
                      partitions.data());
}

// mtx

void partial_cd::mtx_check_banner(std::ifstream &file)
{
  std::string banner;
  std::getline(file, banner);
  std::stringstream tmp;
  std::string word;
  tmp << banner;
  const std::string words[] = {
    "%%MatrixMarket",
    "matrix",
    "coordinate",
    "real",
    "general"
  };
  for (auto w : words) {
    tmp >> word;
    if (w != word) {
      throw std::logic_error("Incorrect mtx banner");
    }
  }
}

void partial_cd::mtx_fill_size(std::ifstream &file)
{
  std::string line;
  std::stringstream ss;
  std::getline(file, line);
  while (line[0] == '%')
    std::getline(file, line);

  ss << line;
  size_t m;
  ss >> m >> n >> nnz;
  if (m != n) {
    throw std::logic_error("Matrix not square");
  }

  ptr.resize(n + 1);
  col.reserve(nnz);
  val.reserve(nnz);
  partitions.resize(n);
  levels.resize(n);
  partials.resize(n);
}

void partial_cd::mtx_fill_vectors(std::ifstream &file)
{
  std::string line;
  std::vector<std::vector<idx_t>> Js(this->n);
  std::vector<std::vector<double>> vs(this->n);
  while (std::getline(file, line)) {
    if (line[0] != '%') {
      std::stringstream ss(line);
      double val;
      int i, j;
      ss >> i >> j >> val;
      i--; j--;
      ptr[i + 1]++;
      Js[i].push_back(j);
      vs[i].push_back(val);
    }
  }
  for (int i = 0; i < n; i++) {
    ptr[i + 1] += ptr[i];
    col.insert(std::end(col), std::begin(Js[i]), std::end(Js[i]));
    val.insert(std::end(val), std::begin(vs[i]), std::end(vs[i]));
  }
}

// pdmpk

void partial_cd::pdmpk_update_levels()
{
  int was_active = true;
  for (int k = 0; was_active and k < nlevels; k++) {
    was_active = false;
    for (int i = 0; i < n; i++) {
      was_active = was_active or pdmpk_proc_vertex(i, k + 1);
    }
  }
}

bool partial_cd::pdmpk_proc_vertex(const idx_t idx, const level_t level)
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
  return true;
}

void partial_cd::pdmpk_update_weights()
{
}
