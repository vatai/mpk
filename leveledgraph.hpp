#ifndef _LEVELEDGRAPH_H_
#define _LEVELEDGRAPH_H_

#include <fstream>
#include <map>

typedef double fp_t;

class bVector
{
public:
  bVector(int, int);
  virtual ~bVector();
  void octave_check(const char*);
  fp_t *array;
  int n;
  int num_steps;
private:
  bVector();
};


class CSRMatrix
{
public:
  CSRMatrix(const char*);
  virtual ~CSRMatrix();
private:
  CSRMatrix();
public:
  int n, nnz, *ptr, *col;
  fp_t *val;
};


class PartitionedGraph : public CSRMatrix
{
public:
  PartitionedGraph(const char*, const int);
  virtual ~PartitionedGraph();
public:
  int *partitions;
  int *old_partitions;
  int num_part;
private:
  PartitionedGraph();
};


class LeveledGraph : public PartitionedGraph
{
public:
  LeveledGraph(const char*, const int );
  virtual ~LeveledGraph();
public:
  int *levels, *old_levels, *vertexweights, *edgeweights;
  unsigned *partials, *currentPermutation, *globalPermutation,
    *inversePermutation, *iterArray, *partitionBegin;
  void partition();
  void printPartitions();
  void printLevels();
  void wpartition();
  void optimisePartitions();
  void updateWeights();
  void permute(bVector &);
  void inversePermute(bVector &);
  void MPK(bVector &);
  void printStats();

  std::map<std::pair<int,int>,int> comm_log;
private:
  LeveledGraph();
  unsigned *tmp_partial, *tmp_perm;
  int *tmp_level, *tmp_old_level, *tmp_part, *tmp_old_part;
  int *tmp_ptr, *tmp_col;
  fp_t *tmp_val;
  template<typename T>
  void printArray(T*);
};

#endif // _LEVELEDGRAPH_H_
