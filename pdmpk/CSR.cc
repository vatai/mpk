//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-09-17

#include <sstream>

#include "CSR.h"

CSR::CSR(const char *fname)
{
  std::ifstream file{fname};

  MtxCheckBanner(file);
  MtxFillSize(file);
  MtxFillVectors(file);
}

std::vector<double> CSR::SpMV(const std::vector<double> &vec) const {
  std::vector<double> result(vec.size());
  for (size_t i = 0; i < ptr.size(); i++) {
    double tmp = 0.0;
    for (int t = ptr[i]; t < ptr[i + 1]; t++) {
      tmp += val[t] * vec[col[t]];
    }
    result[i] = tmp;
  }
  return result;
}

void CSR::MPK(const int nlevels, std::vector<double> &vec) const {
  for (int i = 0; i < nlevels; i++)
    vec = SpMV(vec);
}

void CSR::MtxCheckBanner(std::ifstream &file)
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

void CSR::MtxFillSize(std::ifstream &file)
{
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

void CSR::MtxFillVectors(std::ifstream &file)
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
#ifdef _DEBUG_CSR_VALS
  for (int i = 0; i < n; i++) {
    double v = 1. / (ptr[i + 1] - ptr[i]);
    for (int t = ptr[i]; t < ptr[i + 1]; t++) {
      val[t] = v;
    }
  }
#endif
}
