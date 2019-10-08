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
  void process();

 private:
  idx_t npart;
  comm_dict_t ();
  std::vector<int> recvbuf;
  std::vector<int> recvcount;
  std::vector<int> recvdispl;
  std::vector<int> sendbuf;
  std::vector<int> sendcount;
  std::vector<int> senddispl;
};
