#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

#include "partial_cd.h"
#include "metis.h"

int metistmp()
{
  int i,j;
  printf("Hello metis\n");
  idx_t ov[1];
  idx_t xadj[] = {0, 2, 5, 8, 11, 13, 16, 20, 24, 28, 31, 33, 36, 39, 42, 44};
  idx_t adjncy[] = {
    /* 0 */ 1, 5,          // 0
    /* 2 */ 0, 2, 6,       // 1
    /* 5 */ 1, 3, 7,       // 2
    /* 8 */ 2, 4, 8,       // 3
    /* 11 */ 3, 9,         // 4
    /* 13 */ 0, 6, 10,     // 5
    /* 16 */ 1, 5, 7, 11,  // 6
    /* 20 */ 2, 6, 8, 12,  // 7
    /* 24 */ 3, 7, 9, 13,  // 8
    /* 28 */ 4, 8, 14,     // 9
    /* 31 */ 5, 11,        // 10
    /* 33 */ 6, 10, 12,    // 11
    /* 36 */ 7, 11, 13,    // 12
    /* 39 */ 8, 12, 14,    // 13
    /* 42 */ 9, 13         // 14
  };
  idx_t N = sizeof(xadj)/sizeof(*xadj)-1;
  idx_t nc = 1;
  idx_t part[N];
  idx_t k = 2;
  printf("nv %d\n",N);
  printf("ne %d\n",nc);
  METIS_PartGraphRecursive(&N, &nc, xadj, adjncy, NULL,
		      NULL, NULL, &k, NULL, NULL,
		      NULL, ov, part);
  for (i = 0; i < N; i++){
    //for (j = xadj[i]; j < xadj[i+1]; j++)
    // printf("[%d] = %d, ", i, part[i]);
    // printf("\n");
  }
  return 0;
}

void partial_cd::check_banner(std::ifstream &file)
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

void partial_cd::fill_size(std::ifstream &file)
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

void partial_cd::fill_vectors(std::ifstream &file)
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

partial_cd::partial_cd(const char *_dir, const int _rank) //
    : dir{_dir}, rank{_rank}                              //
{
  if (rank == 0) {
    std::ifstream file{this->dir};
    this->check_banner(file);
    this->fill_size(file);
    this->fill_vectors(file);

    // metistmp();

    // std::cout << "ptr: ";
    // for (auto v : this->ptr)
    //   std::cout << v << ", ";
    // std::cout << std::endl;

    // std::cout << "col: ";
    // for (auto v : this->col)
    //   std::cout << v << ", ";
    // std::cout << std::endl;
  }
}
