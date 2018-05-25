#include "mpigraph.h"
#include <mpi.h>

MPI_Graph::MPI_Graph(int *argc, char ***argv){
  MPI_Init(argc, argv);
}

MPI_Graph::~MPI_Graph(){
  MPI_Finalize();
}
