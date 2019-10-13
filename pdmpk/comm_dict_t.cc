#include <cassert>

#include "comm_dict_t.h"

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

/**
 * Details of the `process()`: go through all the `from` and `to`
 * partitions, and collect all the information about communication.
 */
void comm_dict_t::process()
{
  const auto npart = mpi_bufs.npart;
  for (idx_t from = 0; from < npart; from++) {
    for (idx_t to = 0; to < npart; to++) {
      const auto siter = sdict.find({from, to});
      if (siter != sdict.end()) {
        mpi_bufs.recvcount[to][from] = siter->second.size();
        mpi_bufs.sendcount[from][to] = siter->second.size();
        auto &rbuf = mpi_bufs.recvbuf[to];
        auto &sbuf = mpi_bufs.sendbuf[from];
        const auto &buf = siter->second;
        rbuf.insert(std::end(rbuf), std::begin(buf), std::end(buf));
        sbuf.insert(std::end(sbuf), std::begin(buf), std::end(buf));
      }
      const auto iiter = idict.find({from, to});
      if (iiter != idict.end()) {
        mpi_bufs.recvcount[to][from] += 1;
        mpi_bufs.sendcount[from][to] += 1;
        const auto idx = iiter->second;
        mpi_bufs.recvbuf[to].push_back(idx);
        mpi_bufs.sendbuf[from].push_back(idx);
      }
    }
  }
  /// Finally, from the "count" buffers, the "displacement" buffers
  /// can be computed.
  mpi_bufs.fill_displs();
}

void comm_dict_t::clear()
{
  sdict.clear();
  idict.clear();
  mpi_bufs.clear();
}
