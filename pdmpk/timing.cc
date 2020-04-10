// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2020-04-10

#include "timing.h"
#include <iostream>
#include <mpi.h>

Timing::Timing(const size_t &size)
    : comp_start_time(size, 0.0), comp_end_time(size, 0.0),
      comm_start_time(size, 0.0), comm_end_time(size, 0.0) {}

void Timing::StartDoComp(const size_t &phase) {
  comp_start_time[phase] = MPI_Wtime();
}

void Timing::StopDoComp(const size_t &phase) {
  comp_end_time[phase] = MPI_Wtime();
}

void Timing::StartDoComm(const size_t &phase) {
  comm_start_time[phase] = MPI_Wtime();
}

void Timing::StopDoComm(const size_t &phase) {
  comm_end_time[phase] = MPI_Wtime();
}

void Timing::CollectData() {
  int rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  if (rank > 0) {
    // MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int
    // tag, MPI_Comm comm)
    std::cout << "Send times from " << rank << std::endl;
  } else {
    for (int k = 1; k < world_size; k++) {
      // MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int
      // tag, MPI_Comm comm, MPI_Status *status)
      std::cout << "Recv times from " << k << std::endl;
    }
  }
}
