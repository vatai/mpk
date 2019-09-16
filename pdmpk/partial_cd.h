#ifndef _PARTIAL_CD_
#define _PARTIAL_CD_

#include <fstream>
#include <string>
#include <vector>

#include <metis.h>

struct csr {
  size_t n;
  std::vector<idx_t> ptr;
};

class partial_cd {
public:
  partial_cd(const char* _dir, const int _rank);

  const std::string dir;
  const int rank;

  std::vector<idx_t> ptr;
  std::vector<idx_t> col;
  std::vector<double> val;
  idx_t n;
  idx_t nnz;

private:
  void check_banner(std::ifstream &file);
  void fill_size(std::ifstream &file);
  void fill_vectors(std::ifstream &file);
  void metistmp();
};

#endif
