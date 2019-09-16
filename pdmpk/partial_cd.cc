#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "partial_cd.h"
#include "metis.h"

partial_cd::partial_cd(const char *_dir, const int _rank, const int _npart)
    : dir{_dir}, rank{_rank}, npart{_npart}
{
  std::ifstream file{this->dir};
  mtx_check_banner(file);
  mtx_fill_size(file);
  mtx_fill_vectors(file);

  metistmp();

  if (rank == 0) {
    std::cout << "ptr: ";
    for (auto v : ptr) std::cout << v << ", ";
    std::cout << std::endl;

    std::cout << "col: ";
    for (auto v : col) std::cout << v << ", ";
    std::cout << std::endl;

    std::cout << "val: ";
    for (auto v : val) std::cout << v << ", ";
    std::cout << std::endl;
  }
}

void partial_cd::metistmp()
{
  idx_t ov[1];
  std::vector<idx_t> part(n);
  METIS_PartGraphRecursive(&n, &nnz, ptr.data(), col.data(), NULL, NULL, NULL,
                           &npart, NULL, NULL, NULL, ov, part.data());
  for (auto v : part)
    std::cout << v << ", ";
  std::cout << std::endl;
}

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
  ss >> m >> this->n >> this->nnz;
  if (m != this->n) {
    throw std::logic_error("Matrix not square");
  }

  this->ptr.resize(this->n + 1);
  this->col.reserve(this->nnz);
  this->val.reserve(this->nnz);
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
