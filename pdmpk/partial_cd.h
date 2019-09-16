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
  partial_cd(const char* _dir, const int _rank, const int _npart);

  const std::string dir;
  const int rank;
  idx_t npart;

  std::vector<idx_t> ptr;
  std::vector<idx_t> col;
  std::vector<double> val;
  idx_t n;
  idx_t nnz;

private:
  void mtx_check_banner(std::ifstream &file);
  void mtx_fill_size(std::ifstream &file);
  void mtx_fill_vectors(std::ifstream &file);
  void metistmp();
};

#endif
