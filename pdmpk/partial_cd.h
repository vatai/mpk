#ifndef _PARTIAL_CD_
#define _PARTIAL_CD_

#include <string>
#include <fstream>

class partial_cd {
public:
  partial_cd(const char* dir, const int rank);

private:
  const std::string dir;
  const int rank;
};

#endif
