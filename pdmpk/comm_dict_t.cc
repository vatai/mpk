#include <cassert>

#include "comm_dict_t.h"

comm_dict_t::comm_dict_t(const idx_t npart)
    : npart {npart}, //v{npart, std::vector<int>(npart, 0)}
      recvcount(npart, std::vector<int>(npart, 0)),
      recvdispl(npart, std::vector<int>(npart, 0)),
      sendcount(npart, std::vector<int>(npart, 0)),
      senddispl(npart, std::vector<int>(npart, 0)),
      recvbuf(npart),
      sendbuf(npart)
{}


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
  os << "recvcount: ";
  for (auto v : recvcount)
    for (auto e : v) os << e << ", ";
  os << "\n";
  os << "recvdispl: ";
  for (auto v : recvdispl)
    for (auto e : v) os << e << ", ";
  os << "\n";
  os << "sendcount: ";
  for (auto v : sendcount)
    for (auto e : v) os << e << ", ";
  os << "\n";
  os << "senddispl: ";
  for (auto v : senddispl)
    for (auto e : v) os << e << ", ";
  os << "\n";

  for (int i = 0; i < npart; i++) {
    os << i << "recv: ";
    for (auto e : recvbuf[i]) os << e << ", ";
    os << "\n";
    os << i << "send: ";
    for (auto e : sendbuf[i]) os << e << ", ";
    os << "\n";
  }
}

void comm_dict_t::process()
{
  for (idx_t from = 0; from < npart; from++) {
    for (idx_t to = 0; to < npart; to++) {
      const auto iter = this->find({from, to});
      if (iter != this->end()) {
        recvcount[to][from] = iter->second.size();
        sendcount[from][to] = iter->second.size();
        auto &rbuf = recvbuf[to];
        auto &sbuf = sendbuf[from];
        const auto &buf = iter->second;
        rbuf.insert(std::end(rbuf), std::begin(buf), std::end(buf));
        sbuf.insert(std::end(sbuf), std::begin(buf), std::end(buf));
      }
    }
  }
  for (idx_t r = 0; r < npart; r++) {
    recvdispl[r][0] = 0;
    senddispl[r][0] = 0;
    for (idx_t i = 1; i < npart; i++) {
      recvdispl[r][i] = recvdispl[r][i - 1] + recvcount[r][i - 1];
      senddispl[r][i] = senddispl[r][i - 1] + sendcount[r][i - 1];
    }
  }
}
