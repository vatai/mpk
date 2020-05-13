/// @author Emil VATAI <emil.vatai@gmail.com>
/// @date 2020-04-10

#pragma once

#include <vector>

/// Timing data with methods to process them.
class Timing {
public:
  /// Constructor initialising all timing data.
  Timing();

  /// Start global timer.
  void StartGlobal();

  /// Stop global timer.
  void StopGlobal();

  /// Call before @ref Buffers::DoComp.
  void StartDoComp();

  /// Call after @ref Buffers::DoComp.
  void StopDoComp();

  /// Call before @ref Buffers::DoComm.
  void StartDoComm();

  /// Call after @ref Buffers::DoComm.
  void StopDoComm();

  /// Collect all the data using MPI communication.
  void CollectData();

  /// Count of each execution.
  int count;

private:
  /// Stores wall time executing the everything.
  double global_time;

  /// Sums time executing the everything.
  double global_sum;

  /// Stores wall time before @ref Buffers::DoComp.
  double comp_time;

  /// Sums time spent on @ref Buffers::DoComp.
  double comp_sum;

  /// Stores wall time before @ref Buffers::DoComm.
  double comm_time;

  /// Sums time spent on @ref Buffers::DoComm.
  double comm_sum;
};
