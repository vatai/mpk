#pragma once

#include <iostream>
#include <map>
#include <vector>

#include <metis.h>

class comm_dict_t : std::map<std::pair<idx_t, idx_t>,
                     std::vector<idx_t>>
{
 public:
  comm_dict_t (const idx_t npart);
  void record(const idx_t from, const idx_t to, const idx_t idx);
  std::vector<idx_t> &view(const idx_t from, const idx_t to);
  void serialise(std::ostream &os);

  // Call this before accessing the MPI vectors.
  void process();

  // Getters:
  // std::vector<int>::const_iterator rcbegin(size_t r);
  // std::vector<int>::const_iterator rdbegin(size_t r);
  // std::vector<int>::const_iterator scbegin(size_t r);
  // std::vector<int>::const_iterator sdbegin(size_t r);

 private:
  idx_t npart;
  comm_dict_t();
  std::vector<std::vector<int>> recvbuf;
  std::vector<std::vector<int>> sendbuf;
  std::vector<int> recvcount;
  std::vector<int> sendcount;
  std::vector<int> recvdispl;
  std::vector<int> senddispl;
  // std::vector
  std::vector<std::vector<int>> v;
};
