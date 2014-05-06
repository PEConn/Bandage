#ifndef FUNCTION_DUPLICATER_HPP
#define FUNCTION_DUPLICATER_HPP

#include <map>
#include <set>
#include "llvm/IR/Module.h"

using namespace llvm;

class FunctionDuplicater{
public:
  FunctionDuplicater(Module &M);

  std::set<Function *> RawFunctions;
  std::set<Function *> FPFunctions;
  std::map<Function *, Function *> RawToFPMap;
private:
  Function *Main;
  void DuplicateFunctions(Module &M);
  void RenameMain();
};

#endif
