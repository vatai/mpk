#include <mpi.h>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
  char src[] = "string";
  char *ptr = new char[10];
  
  strcpy(ptr,src);
  MPI_Init(&argc, &argv);

  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  cout << "World size: " << world_size << ", ";

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  cout << "My rank: " << rank << ", ";

  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int len;
  MPI_Get_processor_name(processor_name, &len);
  cout << processor_name << endl;
  
  ptr[0] = '0'+rank;
  
  cout << ptr << endl;
  
  cout << "Bye" << endl;
  MPI_Finalize();
  return 0;
}
