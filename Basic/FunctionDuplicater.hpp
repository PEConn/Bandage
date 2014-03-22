#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <set>
#include <map>
#include "llvm/IR/Module.h"
#include "FatPointers.hpp"

using namespace llvm;

class FunctionDuplicater{
public:
  FunctionDuplicater(FatPointers *FPS, Module &M);

  std::set<Function *> GetRawFunctions();
  std::set<Function *> GetFPFunctions();
  Function *GetFPVersion(Function *F);

  // TODO: Make this private with an accessor
  std::map<Function *, Function *> RawToFPMap;
private:
  void DuplicateFunctions(FatPointers *FPS, Module &M);

  Function *Main;
  std::set<Function *> RawFunctions;
  std::set<Function *> FPFunctions;
};

#endif
