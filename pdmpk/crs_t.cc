//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-09-17

#include <sstream>

#include "crs_t.h"

crs_t::crs_t(const char *fname)
{
  std::ifstream file{fname};

  mtx_check_banner(file);
  mtx_fill_size(file);
  mtx_fill_vectors(file);
}

void crs_t::mtx_check_banner(std::ifstream &file)
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

void crs_t::mtx_fill_size(std::ifstream &file)
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
}

void crs_t::mtx_fill_vectors(std::ifstream &file)
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