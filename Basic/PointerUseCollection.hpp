#ifndef POINTER_USE_COLLECTION
#define POINTER_USE_COLLECTION

#include <set>
#include "llvm/IR/Instructions.h"
#include "FunctionDuplicater.hpp"
#include "PointerUse.hpp"

using namespace llvm;

class PointerUseCollection{
public:
  PointerUseCollection(FunctionDuplicater *FD, Module &M);
  std::set<PU *> PointerUses;
  std::set<PointerParameter *> PointerParams;
private:
  void CollectInstructions(std::set<Function *> Functions, Module &M);
};

#endif
