#pragma once
// use with #include <mpi.h>

#define DECLARE_TIMING(s)                                                      \
  double timeStart_##s;                                                        \
  double timeDiff_##s;                                                         \
  double timeTally_##s = 0;                                                    \
  int countTally_##s = 0

#define START_TIMING(s) timeStart_##s = MPI_Wtime()

#define STOP_TIMING(s)                                                         \
  timeDiff_##s = (double)(MPI_Wtime() - timeStart_##s);                        \
  timeTally_##s += timeDiff_##s;                                               \
  countTally_##s++

#define GET_TIMING(s) (double)(timeDiff_##s)

#define GET_AVERAGE_TIMING(s)                                                  \
  (double)(countTally_##s ? (double)(timeTally_##s / countTally_##s) : 0)

#define CLEAR_AVERAGE_TIMING(s)                                                \
  timeTally_##s = 0;                                                           \
  countTally_##s = 0
