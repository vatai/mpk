#include <cassert>

#include "comm_dict_t.h"

comm_dict_t::comm_dict_t(const idx_t npart)
    : npart {npart}
{
  recvcount.resize(npart * npart, 0);
  recvdispl.resize(npart * npart, 0);
  sendcount.resize(npart * npart, 0);
  senddispl.resize(npart * npart, 0);
}


void comm_dict_t::record(const idx_t from, const idx_t to, const idx_t idx)
{
  auto &self = *this;
  self[{from, to}].push_back(idx);
}

std::vector<idx_t> &comm_dict_t::view(const idx_t from, const idx_t to)
{
  return this->at({from, to});
}

void comm_dict_t::serialise(std::ostream &os)
{
  os << npart << "\n";
  for (auto e : recvcount) os << e << ", ";
  os << "\n";
  for (auto e : recvdispl) os << e << ", ";
  os << "\n";
  for (auto e : sendcount) os << e << ", ";
  os << "\n";
  for (auto e : senddispl) os << e << ", ";
  os << "\n";
}

void comm_dict_t::process()
{
  for (idx_t from = 0; from < npart; from++) {
    for (idx_t to = 0; to < npart; to++) {
      auto iter = this->find({from, to});
      if (iter != this->end()) {
        recvcount[from * npart + to] = iter->second.size();
        sendcount[to * npart + from] = iter->second.size();
      }
    }
  }
  for (idx_t r = 0; r < npart; r++) {
    recvdispl[r * npart] = 0;
    senddispl[r * npart] = 0;
    for (idx_t i = 1; i < npart; i++) {
      recvdispl[r * npart + i] = recvdispl[r * npart + i - 1] + recvcount[r * npart + i - 1];
      senddispl[r * npart + i] = senddispl[r * npart + i - 1] + sendcount[r * npart + i - 1];
    }
  }
}
