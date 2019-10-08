#include <iostream>
#include "comm_dict_t.h"

int main(int argc, char *argv[])
{
  comm_dict_t cdict(3);
  cdict.record(0, 2, 10);
  cdict.record(0, 1, 20);
  cdict.process();
  cdict.serialise(std::cout);
  return 0;
}
