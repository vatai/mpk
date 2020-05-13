// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2020-04-10

#include "timing.h"
#include <iostream>
#include <mpi.h>

Timing::Timing() : count(0) {}

void Timing::StartGlobal() { global_time = MPI_Wtime(); }

void Timing::StopGlobal() { global_sum += MPI_Wtime() - global_time; }

void Timing::StartDoComp() { comp_time = MPI_Wtime(); }

void Timing::StopDoComp() { comp_sum += MPI_Wtime() - comp_time; }

void Timing::StartDoComm() { comm_time = MPI_Wtime(); }

void Timing::StopDoComm() { comm_sum += MPI_Wtime() - comm_time; }

void Timing::CollectData() {
  int rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  if (rank > 0) {
  } else {
    // Send times to process 0.
    for (int r = 1; r < world_size; r++) {
      // Recv times from process `r`.
    }
  }
}
