#ifndef _PARTIAL_CD_
#define _PARTIAL_CD_

#include <fstream>
#include <string>
#include <vector>

#include <metis.h>

class partial_cd {
public:
  typedef unsigned short level_t;
  typedef unsigned long long partials_t;

  partial_cd(const char *_dir, const int _rank, const int _world_size,
             const idx_t _npart, const level_t nlevels);

  const std::string dir;
  const int rank;
  const int world_size;
  idx_t npart;
  const level_t nlevels;

  idx_t n;
  idx_t nnz;
  std::vector<idx_t> ptr;
  std::vector<idx_t> col;
  std::vector<double> val;

  std::vector<idx_t> partitions;
  std::vector<idx_t> weights;
  std::vector<level_t> levels;
  std::vector<partials_t> partials;

private:
  void mtx_check_banner(std::ifstream &file);
  void mtx_fill_size(std::ifstream &file);
  void mtx_fill_vectors(std::ifstream &file);

  void metis_partition();
  void metis_partition_with_levels();

  void pdmpk_update_levels();
  bool pdmpk_proc_vertex(const idx_t idx, const level_t level);
  void pdmpk_update_weights();
};

#endif
