// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2019-09-17

#include <sstream>
#include <string>

#include "csr.h"

Csr::Csr(const std::string &fname) {
  std::ifstream file{fname};

  MtxCheckBanner(file);
  MtxFillSize(file);
  MtxFillVectors(file);
}

std::vector<double> Csr::SpMV(const std::vector<double> &vec) const {
  std::vector<double> result(vec.size());
  auto const n = ptr.size() - 1;
  for (size_t i = 0; i < n; i++) {
    double tmp = 0.0;
    for (int t = ptr[i]; t < ptr[i + 1]; t++) {
      tmp += val[t] * vec[col[t]];
    }
    result[i] = tmp;
  }
  return result;
}

void Csr::MPK(const int &nlevels, std::vector<double> &vec) const {
  for (int i = 0; i < nlevels; i++)
    vec = SpMV(vec);
}

void Csr::MtxCheckBanner(std::ifstream &file) {
  std::string banner;
  std::getline(file, banner);
  std::stringstream tmp;
  std::string word;
  tmp << banner;

  tmp >> word;
  if (word != "%%MatrixMarket")
    throw std::logic_error("Incorrect mtx banner!");

  tmp >> word;
  if (word != "matrix")
    throw std::logic_error("Incorrect mtx banner!");

  tmp >> word;
  if (word != "coordinate")
    throw std::logic_error("Not coordinate (sparse) matrix!");

  tmp >> word;
  pattern = (word == std::string("pattern"));
  if (not pattern and word != std::string("real"))
    throw std::logic_error("Not real or pattern matrix!");

  tmp >> word;
  symmetric = (word == std::string("symmetric"));
  if (not symmetric and word != std::string("general"))
    throw std::logic_error("Not general or symmetric matrix!");
}

void Csr::MtxFillSize(std::ifstream &file) {
  std::string line;
  std::stringstream ss;
  std::getline(file, line);
  while (line[0] == '%')
    std::getline(file, line);

  ss << line;
  idx_t m;
  ss >> m >> n >> nnz;
  if (m != n) {
    throw std::logic_error("Matrix not square");
  }

  ptr.resize(n + 1);
  col.reserve(nnz);
  val.reserve(nnz);
}

void Csr::MtxFillVectors(std::ifstream &file) {
  std::string line;
  std::vector<std::vector<idx_t>> Js(this->n);
  std::vector<std::vector<double>> vs(this->n);
  while (std::getline(file, line)) {
    if (line[0] != '%') {
      std::stringstream ss(line);
      double val;
      int i, j;
      ss >> i >> j;
      if (not pattern)
        ss >> val;
      else
        val = 1.0;
      i--;
      j--;
      ptr[i + 1]++;
      Js[i].push_back(j);
      vs[i].push_back(val);
      if (symmetric and i != j) {
        Js[j].push_back(i);
        vs[j].push_back(val);
      }
    }
  }
  for (int i = 0; i < n; i++) {
    ptr[i + 1] += ptr[i];
    col.insert(std::end(col), std::begin(Js[i]), std::end(Js[i]));
    val.insert(std::end(val), std::begin(vs[i]), std::end(vs[i]));
  }
  if (pattern) {
    for (int i = 0; i < n; i++) {
      double v = 1. / (ptr[i + 1] - ptr[i]);
      for (int t = ptr[i]; t < ptr[i + 1]; t++) {
        val[t] = v;
      }
    }
  }
}
