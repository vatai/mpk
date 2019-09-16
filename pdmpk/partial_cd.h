#ifndef _PARTIAL_CD_
#define _PARTIAL_CD_

#include <fstream>
#include <string>
#include <vector>

#include <metis.h>

class partial_cd {
public:
  partial_cd(const char *_dir, const int _rank, const idx_t _npart,
             const int nlevels);

  const std::string dir;
  const int rank;
  idx_t npart;
  const int nlevels;

  idx_t n;
  idx_t nnz;
  std::vector<idx_t> ptr;
  std::vector<idx_t> col;
  std::vector<double> val;

  std::vector<idx_t> partitions;
  std::vector<idx_t> weights;
  std::vector<unsigned short> levels;
  std::vector<unsigned long> partials;

private:
  void mtx_check_banner(std::ifstream &file);
  void mtx_fill_size(std::ifstream &file);
  void mtx_fill_vectors(std::ifstream &file);

  void metis_partition();
  void metis_partition_with_levels();

  void pdmpk_update_levels();
  void pdmpk_update_weights();
};

#endif
