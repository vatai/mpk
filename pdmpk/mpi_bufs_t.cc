#include "mpi_bufs_t.h"

mpi_bufs_t::mpi_bufs_t(const idx_t npart)
    : npart {npart},
      recvcount(npart, std::vector<int>(npart, 0)),
      recvdispl(npart, std::vector<int>(npart, 0)),
      sendcount(npart, std::vector<int>(npart, 0)),
      senddispl(npart, std::vector<int>(npart, 0)),
      recvbuf{std::vector<int>::size_type(npart)},
      sendbuf{std::vector<int>::size_type(npart)}
{}

void mpi_bufs_t::clear()
{
  for (auto &v : recvbuf) v.clear();
  for (auto &v : sendbuf) v.clear();
  for (auto &v : recvcount) for (auto &e : v) e = 0;
  for (auto &v : recvdispl) for (auto &e : v) e = 0;
  for (auto &v : sendcount) for (auto &e : v) e = 0;
  for (auto &v : senddispl) for (auto &e : v) e = 0;
}

void mpi_bufs_t::serialise(std::ostream &os)
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

