#include "mpi_bufs_t.h"
#include <ostream>
#include <vector>

mpi_bufs_t::mpi_bufs_t(const idx_t npart)
    : npart {npart},
      recvcount(npart, std::vector<int>(npart, 0)),
      recvdispl(npart, std::vector<int>(npart, 0)),
      sendcount(npart, std::vector<int>(npart, 0)),
      senddispl(npart, std::vector<int>(npart, 0)),
      recvbuf{std::vector<int>::size_type(npart)},
      sendbuf{std::vector<int>::size_type(npart)}
{}

void mpi_bufs_t::add_svect(
    const idx_t src,
    const idx_t tgt,
    const std::vector<idx_t> &src_idcs)
{
  recvcount[tgt][src] = src_idcs.size();
  sendcount[src][tgt] = src_idcs.size();
  auto &rbuf = recvbuf[tgt];
  auto &sbuf = sendbuf[src];
  rbuf.insert(std::end(rbuf), std::begin(src_idcs), std::end(src_idcs));
  sbuf.insert(std::end(sbuf), std::begin(src_idcs), std::end(src_idcs));
}

void mpi_bufs_t::add_ivect(const idx_t src, const idx_t tgt, const idx_t idx)
{
  recvcount[tgt][src] += 1;
  sendcount[src][tgt] += 1;
  recvbuf[tgt].push_back(idx);
  sendbuf[src].push_back(idx);
  return;
}

void mpi_bufs_t::fill_displs()
{
  for (idx_t r = 0; r < npart; r++) {
    recvdispl[r][0] = 0;
    senddispl[r][0] = 0;
    for (idx_t i = 1; i < npart; i++) {
      recvdispl[r][i] = recvdispl[r][i - 1] + recvcount[r][i - 1];
      senddispl[r][i] = senddispl[r][i - 1] + sendcount[r][i - 1];
    }
  }
}

void mpi_bufs_t::clear()
{
  for (auto &v : recvbuf) v.clear();
  for (auto &v : sendbuf) v.clear();
  for (auto &v : recvcount) for (auto &e : v) e = 0;
  for (auto &v : recvdispl) for (auto &e : v) e = 0;
  for (auto &v : sendcount) for (auto &e : v) e = 0;
  for (auto &v : senddispl) for (auto &e : v) e = 0;
}

std::ostream &operator<<(std::ostream &os, const mpi_bufs_t &bufs)
{
  os << bufs.npart << "\n";
  os << "recvcount: ";
  for (auto v : bufs.recvcount)
    for (auto e : v) os << e << ", ";
  os << "\n";
  os << "recvdispl: ";
  for (auto v : bufs.recvdispl)
    for (auto e : v) os << e << ", ";
  os << "\n";
  os << "sendcount: ";
  for (auto v : bufs.sendcount)
    for (auto e : v) os << e << ", ";
  os << "\n";
  os << "senddispl: ";
  for (auto v : bufs.senddispl)
    for (auto e : v) os << e << ", ";
  os << "\n";

  for (int i = 0; i < bufs.npart; i++) {
    os << i << "recv: ";
    for (auto e : bufs.recvbuf[i]) os << e << ", ";
    os << "\n";
    os << i << "send: ";
    for (auto e : bufs.sendbuf[i]) os << e << ", ";
    os << "\n";
  }
  return os;
}

