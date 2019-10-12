#include <cassert>

#include "comm_dict_t.h"

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

comm_dict_t::comm_dict_t(const idx_t npart)
    : mpi_bufs {npart}
{}

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

void comm_dict_t::rec_svert(const idx_t from, const idx_t to, const idx_t idx)
{
  sdict[{from, to}].push_back(idx);
}

void comm_dict_t::rec_ivert(const idx_t from, const idx_t to, const idx_t idx)
{
  idict[{from, to}] = idx;
}

void comm_dict_t::process()
{
  const auto npart = mpi_bufs.npart;
  for (idx_t from = 0; from < npart; from++) {
    for (idx_t to = 0; to < npart; to++) {
      const auto iter = sdict.find({from, to});
      if (iter != sdict.end()) {
        mpi_bufs.recvcount[to][from] = iter->second.size();
        mpi_bufs.sendcount[from][to] = iter->second.size();
        auto &rbuf = mpi_bufs.recvbuf[to];
        auto &sbuf = mpi_bufs.sendbuf[from];
        const auto &buf = iter->second;
        rbuf.insert(std::end(rbuf), std::begin(buf), std::end(buf));
        sbuf.insert(std::end(sbuf), std::begin(buf), std::end(buf));
      }
    }
  }
  mpi_bufs.fill_displs();
}

void comm_dict_t::clear()
{
  sdict.clear();
  idict.clear();
  mpi_bufs.clear();
}

