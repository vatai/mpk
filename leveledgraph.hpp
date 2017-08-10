#ifndef _LEVELEDGRAPH_H_
#define _LEVELEDGRAPH_H_

#include <fstream>

typedef float fp_t;

class bVector
{

public:
  bVector(int, int);
  virtual ~bVector();
  fp_t *array;
  int num_steps;
  int nn;
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
  int *levels, *vertexweights, *edgeweights;
  unsigned *partials, *currentPermutation, *globalPermutation,
    *inversePermutation, *iterArray, *partitionBegin;
  void partition();
  void wpartition();
  void updateWeights();
  void updatePermutation();
  void permute(bVector &);
  void inversePermute(fp_t **, unsigned);
  void MPK(bVector &);
  void measure();

private:
  LeveledGraph();
  unsigned *tmp_partial, *tmp_perm;
  int *tmp_level, *tmp_part, *tmp_old_part;
  int *tmp_ptr, *tmp_col;
  fp_t *tmp_val;

};

#endif // _LEVELEDGRAPH_H_
