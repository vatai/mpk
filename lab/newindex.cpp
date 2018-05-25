#include <iostream>
#include <utility>
#include <iomanip>

// 4x5 mesh
// 3*5 + 4*4 = 15 + 16 = 31
// 31*2 + 20 = 82
const int nnz = 82;
const int n = 20;
const int num_part = 2;
const int part_n =  n/num_part; 

int partitions[n] = {
  0,0,0,1,1,
  0,0,0,1,1,
  0,0,1,1,1,
  0,0,1,1,1
};

// rng(val) = \R
float val[nnz] = {
  4, -1, -1, // ptr[0] = 0
  -1, 4, -1, -1, // ptr[1] = 3
  -1, 4, -1, -1, // ptr[2] = 7
  -1, 4, -1, -1, // ptr[3] = 11
  -1, 4, -1, // ptr[4] = 15
  
  -1, 4, -1, -1, // ptr[5] = 18
  -1, -1, 4, -1, -1, // ptr[6] = 22
  -1, -1, 4, -1, -1, // ptr[7] = 27
  -1, -1, 4, -1, -1, // ptr[8] = 32
  -1, -1, 4, -1, // ptr[9] = 37

  -1, 4, -1, -1, // ptr[10] = 41
  -1, -1, 4, -1, -1, // ptr[11] = 45
  -1, -1, 4, -1, -1, // ptr[12] = 50
  -1, -1, 4, -1, -1, // ptr[13] = 55
  -1, -1, 4, -1, // ptr[14] = 60
  
  -1, 4, -1, // ptr[15] = 64
  -1, -1, 4, -1, // ptr[16] = 67
  -1, -1, 4, -1, // ptr[17] = 71
  -1, -1, 4, -1, // ptr[18] = 75
  -1, -1, 4  // ptr[19] = 79
  // ptr[20] = 82
};
float valp0[nnz];
float valp1[nnz];
float *valp[num_part] = {valp0, valp1};
// rng(col) = [0,..,n-1]
int col[nnz] = {
  0, 1, 5,
  0, 1, 2, 6,
  1, 2, 3, 7,
  2, 3, 4, 8,
  3, 4, 9,

  0, 5, 6, 10,
  1, 5, 6, 7, 11,
  2, 6, 7, 8, 12,
  3, 7, 8, 9, 13,
  4, 8, 9, 14,

  5, 10, 11, 15,
  6, 10, 11, 12, 16,
  7, 11, 12, 13, 17,
  8, 12, 13, 14, 18,
  9, 13, 14, 19,

  10, 15, 16,
  11, 15, 16, 17,
  12, 16, 17, 18,
  13, 17, 18, 19,
  14, 18, 19
};
int colp0[nnz];
int colp1[nnz];
int *colp[num_part] = {colp0, colp1};

// rng(ptr) = [0,..,nnz]
int ptr[n+1] = {
  0,  3,  7,  11, 15,
  18, 22, 27, 32, 37,
  41, 45, 50, 55, 60,
  64, 67, 71, 75, 79, 82
};
int ptrp0[part_n+1];
int ptrp1[part_n+1];
int *ptrp[num_part] = {ptrp0, ptrp1};

int newindex[n];

float vec[n] = {
  10, 11, 12, 13, 14,
  22, 23, 24, 25, 29,
  1,  10,  2, 20, 33,
  0,  1,   3, 55,  1
};
float vecp0[part_n];
float vecp1[part_n];
float *vecp[num_part] = {vecp0, vecp1};
float rvec[n];
float rvecp0[part_n];
float rvecp1[part_n];
float *rvecp[num_part] = {rvecp0, rvecp1};


template <typename T>
void print5(T *a, int n)
{
  for(int i = 0; i < n; ++i){
    if(i % 5 == 0) { std::cout << std::endl; }
    std::cout << std::setw(3) << a[i] << ",";
  }
  std::cout << std::endl;
}

void calc_newindex(int *partitions, int *newindex, int n)
{
  int counter[num_part] = {0, 0};
  for(int i = 0; i < n; ++i){
    int p = partitions[i];
    newindex[i] = counter[p];
    counter[p]++;
  }
}

void calc_newvec(int *partitions, int n)
{
  for(int i = 0; i < n; ++i){
    const int p = partitions[i];
    const int inew = newindex[i];
    vecp[p][inew] = vec[i];
  }
}

void
calc_newmat (int *partitions, int n)
{
  for (int p = 0; p < num_part; ++p) { ptrp[p][0] = 0; }
  int counter[num_part] = {0, 0};
  for (int i = 0; i < n; ++i){
    const int p = partitions[i];
    const int ii = counter[p];
    const int end = ptr[i+1];
    ptrp[p][ii+1] = ptrp[p][ii];
    for (int k = ptr[i]; k < end; ++k) {
      const int j = col[k];
      if (partitions[j] == p) {
        const int jp = ptrp[p][ii+1];
        colp[p][jp] = newindex[j];
        valp[p][jp] = val[k];
        ptrp[p][ii+1]++;
      }
    }
    counter[p]++;
  }
  std::cout << counter[0] << "::"  << counter[1]  << std::endl;
}

void
matmul (int n,
        float *rvec,
        int* ptr,
        int* col,
        float* val,
        float* vec)
{
  for(int i = 0; i < n; ++i){
    const int end = ptr[i+1];
    rvec[i] = 0;
    for(int k = ptr[i]; k < end; ++k){
      const int j = col[k];
      rvec[i] += val[k] * vec[j];
    }
  }
}
// (- (* 4 55) (+ 4 20))

int main(int argc, char *argv[])
{
  std::cout << std::endl << "matmul" << std::endl;
  matmul(n, rvec, ptr, col, val, vec);
  print5(vec, n);
  print5(rvec, n);

  // std::cout << std::endl << "partitions" << std::endl;
  // print5(partitions, n);

  calc_newindex(partitions, newindex, n);
  // std::cout << "newindex" << std::endl;
  // print5(newindex, n);

  calc_newvec(partitions, n);
  // std::cout << std::endl << "vecp0";
  // print5(vecp0, part_n);
  // std::cout << "vecp1";
  // print5(vecp1, part_n);

  std::cout << std::endl << "newmat";
  calc_newmat(partitions, n);
  print5(colp[0], 10);
  print5(valp[0], 10);
  print5(ptrp[0], 10);

  std::cout << std::endl << "matmulp[]";
  matmul(part_n, rvecp[0], ptrp[0], colp[0], valp[0], vecp[0]);
  print5(rvecp[0], part_n);
  std::cout << "--- END ---" << std::endl;
  return 0;
}
