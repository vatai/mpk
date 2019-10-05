/*  Author: Emil VATAI <emil.vatai@gmail.com> */
/*  Date: 2019-09-17 */

#ifndef _CRS_T_
#define _CRS_T_

#include <fstream>
#include <vector>

#include <metis.h>

class csr_t {
 public:
  csr_t(const char* fname);
  idx_t n;
  idx_t nnz;
  std::vector<idx_t> ptr;
  std::vector<idx_t> col;
  std::vector<double> val;

 private:
  void mtx_check_banner(std::ifstream &file);
  void mtx_fill_size(std::ifstream &file);
  void mtx_fill_vectors(std::ifstream &file);
};

#endif
