//  Author: Emil VATAI <emil.vatai@gmail.com>
//  Date: 2019-10-14

#include <cassert>

#include "comm_dict_t.h"

comm_dict_t::comm_dict_t(const idx_t npart)
    : mpi_bufs {npart}
{}

void comm_dict_t::rec_svert(
    const idx_t sp,
    const idx_t tp,
    const idx_t si,
    const idx_t ti)
{
  sdict[{sp, tp}].push_back(si);
}

void comm_dict_t::rec_ivert(
    const idx_t sp,
    const idx_t tp,
    const idx_t si,
    const idx_t ti)
{
  idict[{sp, tp}] = si;
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
        const auto &buf = siter->second;
        mpi_bufs.recvcount[to][from] = buf.size();
        mpi_bufs.sendcount[from][to] = buf.size();
        auto &rbuf = mpi_bufs.recvbuf[to];
        auto &sbuf = mpi_bufs.sendidcs[from];
        rbuf.insert(std::end(rbuf), std::begin(buf), std::end(buf));
        sbuf.insert(std::end(sbuf), std::begin(buf), std::end(buf));
      }
      const auto iiter = idict.find({from, to});
      if (iiter != idict.end()) {
        mpi_bufs.recvcount[to][from] += 1;
        mpi_bufs.sendcount[from][to] += 1;
        const auto idx = iiter->second;
        mpi_bufs.recvbuf[to].push_back(idx);
        mpi_bufs.sendidcs[from].push_back(idx);
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
