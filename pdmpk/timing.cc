// Author: Emil VATAI <emil.vatai@gmail.com>
// Date: 2020-04-10

#include "timing.h"
#include <fstream>
#include <iostream>
#include <mpi.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Timing::Timing(const Args &args) : count{0}, args{args} {}

void Timing::StartGlobal() {
  count++;
  global_time = MPI_Wtime();
}

void Timing::StopGlobal() { global_sum += MPI_Wtime() - global_time; }

void Timing::StartDoComp() { comp_time = MPI_Wtime(); }

void Timing::StopDoComp() { comp_sum += MPI_Wtime() - comp_time; }

void Timing::StartDoComm() { comm_time = MPI_Wtime(); }

void Timing::StopDoComm() { comm_sum += MPI_Wtime() - comm_time; }

void Timing::CollectData() {
  int rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  double tmp;
  if (rank > 0) {
    global_sum /= count;
    MPI_Send(&global_sum, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    comp_sum /= count;
    MPI_Send(&comp_sum, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    comm_sum /= count;
    MPI_Send(&comm_sum, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  } else {
    for (int r = 1; r < world_size; r++) {
      MPI_Status status;
      MPI_Recv(&tmp, 1, MPI_DOUBLE, r, 0, MPI_COMM_WORLD, &status);
      global_sum += tmp;
      MPI_Recv(&tmp, 1, MPI_DOUBLE, r, 0, MPI_COMM_WORLD, &status);
      comp_sum += tmp;
      MPI_Recv(&tmp, 1, MPI_DOUBLE, r, 0, MPI_COMM_WORLD, &status);
      comm_sum += tmp;
    }
    global_sum /= world_size;
    comp_sum /= world_size;
    comm_sum /= world_size;
  }
}

void Timing::DumpJson() const {
  json times;
  times["global_avg"] = global_sum;
  times["comp_avg"] = comp_sum;
  times["comm_avg"] = comm_sum;

  std::ofstream of(args.Filename("times.json"));
  of << times.dump() << std::endl;
}
