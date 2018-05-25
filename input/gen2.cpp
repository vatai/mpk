#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
  if (argc != 5) {
    cerr << "usage: " << argv[0] << " type size file pfile" << endl
         << "type: m5p t5p" << endl;
    return 1;
  }
  
  string type(argv[1]);

  stringstream ss;
  int N = -1; // size
  ss << argv[2]; ss >> N;
  if (N <= 0) {
    cerr << "Invalid argument: wrong size." << endl;
    return 1;
  }

  int i,j;

  ofstream f(argv[3]);
  ofstream pf(argv[4]);
  
  if (type == "m5p") {
    int nnz = N*N+2*2*(N)*(N-1);
    f << "%%MatrixMarket matrix coordinate real symmetric\n";
    f << N*N << " " << N*N << " " << nnz << endl;
    for (i = 0; i < N; ++i) {
      for (j = 0; j < N; ++j) {
        f << N*i+j+1 << " " << N*(i)+j+1 << " " << 4 << endl;
        if (i > 0)   f << N*(i-1)+j+1 << " " << N*i+j+1 << " " << -1 << endl;
        if (j > 0)   f << N*i+j+1 << " " << N*i+j-1+1 << " " << -1 << endl;
      }
    }
  }

  if (type == "t5p") {
    f << "%%MatrixMarket matrix coordinate real general\n";
    for (i = 0; i < N; ++i) {
      for (j = 0; j < N; ++j) {
        if (i == j) f << i << " " << j << " " << -4 << endl;
        if (i > 0) f << i-1 << " " << j << " " << 1 << endl;
        else f << N-1 << " " << j << " " << 1 << endl;
        if (i < N-1) f << i+1 << " " << j << " " << 1 << endl;
        else f << 0 << " " << j << " " << 1 << endl;
        if (j > 0) f << i << " " << j-1 << " " << 1 << endl;
        else f << i << " " << N-1 << " " << 1 << endl;
        if (j < N-1) f << i << " " << j+1 << " " << 1 << endl;
        else f << i << " " << 0 << " " << 1 << endl;
      }
    }
  }
  
  f.close();
  pf.close();
  return 0;
}
