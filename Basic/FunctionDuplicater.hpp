#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <set>
#include <map>
#include "llvm/IR/Module.h"
#include "FatPointers.hpp"
#include "TypeDuplicater.hpp"

using namespace llvm;

class FunctionDuplicater{
public:
  FunctionDuplicater(Module &M, TypeDuplicater *TD);

  std::set<Function *> GetRawFunctions();
  std::set<Function *> GetFPFunctions();
  Function *GetFPVersion(Function *F);

  // TODO: Make this private with an accessor
  std::map<Function *, Function *> RawToFPMap;
  ValueToValueMapTy VMap;
private:
  void DuplicateFunctions(Module &M, TypeDuplicater *TD);
  void RenameMain();

  Function *Main;
  std::set<Function *> RawFunctions;
  std::set<Function *> FPFunctions;
};

#endif
