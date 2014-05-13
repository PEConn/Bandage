#ifndef FUNCTION_DUPLICATER_HPP
#define FUNCTION_DUPLICATER_HPP

#include <map>
#include <set>
#include <string>
#include "llvm/IR/Module.h"

#include "PointerReturn.hpp"

using namespace llvm;

class FunctionDuplicater{
public:
  FunctionDuplicater(Module &M, std::string FuncFile="");
  ~FunctionDuplicater();

  std::set<Function *> RawFunctions;
  std::set<Function *> FPFunctions;
  std::map<Function *, Function *> RawToFPMap;
private:
  PointerReturn *PR;
  Function *Main;
  void DuplicateFunctions(Module &M);
  void RenameMain();
};

#endif
