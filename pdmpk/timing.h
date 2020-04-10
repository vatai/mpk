/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2020-04-10

#pragma once

#include <vector>

/// Timing data with methods to process them.
class Timing {
public:
  /// Constructor initialising all data members.
  Timing(const size_t &size);

  /// Call before @ref Buffers::DoComp.
  void StartDoComp(const size_t &phase);

  /// Call after @ref Buffers::DoComp.
  void StopDoComp(const size_t &phase);

  /// Call before @ref Buffers::DoComm.
  void StartDoComm(const size_t &phase);

  /// Call after @ref Buffers::DoComm.
  void StopDoComm(const size_t &phase);

  /// Collect all the data using MPI communication.
  void CollectData();

private:
  /// Constructor initialising all timing data.
  Timing();

  /// Starting time of @ref Buffers::DoComp call for each phase.
  std::vector<double> comp_start_time;

  /// End time of @ref Buffers::DoComp call for each phase.
  std::vector<double> comp_end_time;

  /// Starting time of @ref Buffers::DoComm call for each phase.
  std::vector<double> comm_start_time;

  /// End time of @ref Buffers::DoComm call for each phase.
  std::vector<double> comm_end_time;
};
