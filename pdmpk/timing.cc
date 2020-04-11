// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2020-04-10

#include "timing.h"
#include <iostream>
#include <mpi.h>

Timing::Timing(const size_t &size)
    : comp_start_time(size, 0.0), comp_end_time(size, 0.0),
      comm_start_time(size, 0.0), comm_end_time(size, 0.0),
      comp_start_recv(size, 0.0), comp_end_recv(size, 0.0),
      comm_start_recv(size, 0.0), comm_end_recv(size, 0.0) {}

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
    SendVector(comp_start_time, 0);
    SendVector(comp_end_time, 0);
    SendVector(comm_start_time, 0);
    SendVector(comm_end_time, 0);
  } else {
    InitSummary();
    for (int r = 1; r < world_size; r++) {
      RecvVector(&comp_start_recv, r);
      RecvVector(&comp_end_recv, r);
      RecvVector(&comm_start_recv, r);
      RecvVector(&comm_end_recv, r);
      UpdateSummary();
      std::cout << "Recv times from " << r << std::endl;
    }
  }
}

/// PRIVATE

void Timing::SendVector(const std::vector<double> &vec, const int rank) const {
  MPI_Send(vec.data(), vec.size(), MPI_DOUBLE, rank, 0, MPI_COMM_WORLD);
}

void Timing::RecvVector(std::vector<double> *vec, const int rank) {
  MPI_Status status;
  MPI_Recv(vec->data(), vec->size(), MPI_DOUBLE, rank, 0, MPI_COMM_WORLD,
           &status);
}

void Timing::InitSummary() {}

void Timing::UpdateSummary() {}
