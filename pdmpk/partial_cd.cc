#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "partial_cd.h"

void read_coo(std::ifstream &file);

partial_cd::partial_cd(const char *dir, const int rank)
    : dir {dir},
      rank {rank}
{
  if (rank == 0) {
    std::ifstream file{this->dir};
    read_coo(file);
  }
}

void read_coo(std::ifstream &file)
{
  std::string line;
  std::getline(file, line);
  // banner(line)
  while (std::getline(file, line)) {
    if (line[0] != '%') {
      std::stringstream ss;
      ss << line;
      double val;
      int i, j;
      ss >> i >> j >> val;
    }
  }
}
