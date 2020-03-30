#include <iostream>
#include <ostream>
class Debugger {
public:
  Debugger(const int &var) : var{var} {};
  void Prnt() { std::cout << var << std::endl; }

private:
  const int &var;
};

int main(int argc, char *argv[]) {
  int var = 42;
  Debugger dbg(var);
  dbg.Prnt();
  var = 13;
  dbg.Prnt();
  return 0;
}
