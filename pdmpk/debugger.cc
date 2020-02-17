// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2020-02-17

#include <algorithm>
#include <cassert>

#include "comm_comp_patterns.h"
#include "debugger.h"

Debugger::Debugger(const CommCompPatterns *const ccp) : ccp{ccp} {}

void Debugger::DbgPhaseSummary(const level_t &min_level,
                               const size_t &level_sum) const {
  const auto count = std::count(std::begin(ccp->pdmpk_bufs.levels),
                                std::end(ccp->pdmpk_bufs.levels), min_level);
  std::cout << "Phase: " << ccp->phase << ", "
            << "ExactLevelSum(): " << level_sum << "; "
            << "min_level: " << min_level << ", "
            << "count: " << count << std::endl;
}

void Debugger::DbgAsserts() const {
  /// Check all vertices reach `nlevels`.
  for (auto level : ccp->pdmpk_bufs.levels) {
    assert(level == ccp->args.nlevel);
  }
  /// Assert mbuf + rbuf = mptr size (for each buffer and phase).
  for (auto b : ccp->bufs) {
    for (auto phase = 1; phase < ccp->phase; phase++) {
      auto mbd = b.mbuf.phase_begin[phase] - b.mbuf.phase_begin[phase - 1];
      auto mpd = b.mcsr.mptr.phase_begin[phase] //
                 - b.mcsr.mptr.phase_begin[phase - 1];
      auto rbs = b.mpi_bufs.RbufSize(phase - 1);
      if (mbd != mpd + rbs) {
        std::cout << "phase: " << phase << ", "
                  << "mbd: " << mbd << ", "
                  << "mpd: " << mpd << ", "
                  << "rbs: " << rbs << std::endl;
      }
      assert(mbd == mpd + rbs);
    }
  }
}

void Debugger::DbgMbufChecks() {
  // Check nothing goes over mbufIdx.
  for (auto buffer : ccp->bufs) {
    auto mbuf_idx = buffer.mbuf_idx;
    // Check init_idcs.
    for (auto i = buffer.mpi_bufs.init_idcs.phase_begin[ccp->phase];
         i < buffer.mpi_bufs.init_idcs.size(); i++) {
      auto pair = buffer.mpi_bufs.init_idcs[i];
      if (pair.first >= mbuf_idx) {
        std::cout << pair.first << ", " << mbuf_idx << std::endl;
      }
      assert(pair.first < mbuf_idx);
      assert(pair.second < mbuf_idx);
    }
    // Check sbuf_idcs.
    for (auto i = buffer.mpi_bufs.sbuf_idcs.phase_begin[ccp->phase];
         i < buffer.mpi_bufs.sbuf_idcs.size(); i++) {
      auto value = buffer.mpi_bufs.sbuf_idcs[i];
      assert(value < mbuf_idx);
    }
    // Check mcol.
    for (auto value : buffer.mcsr.mcol) {
      assert(value < mbuf_idx);
    }
  }
}
