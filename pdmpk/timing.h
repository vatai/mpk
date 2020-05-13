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

  /// Call MPI send on a given vector.
  ///
  /// @param vec Send data from here.
  ///
  /// @param rank Target MPI rank.
  void SendVector(const std::vector<double> &vec, const int rank) const;

  /// Call MPI receive on a given vector.
  ///
  /// @param vec Copy received data here.
  ///
  /// @param rank Source MPI rank.
  void RecvVector(std::vector<double> *vec, const int rank);

  /// Update summary data after receiving the a new set of
  /// measurements from another MPI process.
  void CollectUpdate();

  /// Starting time of @ref Buffers::DoComp call for each phase.
  std::vector<double> comp_start_time;

  /// End time of @ref Buffers::DoComp call for each phase.
  std::vector<double> comp_end_time;

  /// Starting time of @ref Buffers::DoComm call for each phase.  0th
  /// element is the last (sendhome).
  std::vector<double> comm_start_time;

  /// End time of @ref Buffers::DoComm call for each phase.  0th
  /// element is the last (sendhome).
  std::vector<double> comm_end_time;

  /// Receive buffers for @ref Timing::comp_start_time from another
  /// MPI process.
  std::vector<double> comp_start_recv;

  /// Receive buffers for @ref Timing::comp_end_time from another
  /// MPI process.
  std::vector<double> comp_end_recv;

  /// Receive buffers for @ref Timing::comm_start_time from another
  /// MPI process.
  std::vector<double> comm_start_recv;

  /// Receive buffers for @ref Timing::comm_end_time from another
  /// MPI process.
  std::vector<double> comm_end_recv;

  /// Sum of durations of each execution.
  double sum_total_time;

  /// Count of each execution.
  int count;

  /// Maintain the maximum of each execution.
  double max_total_time;
};
