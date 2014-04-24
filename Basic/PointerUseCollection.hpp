#ifndef POINTER_USE_COLLECTION
#define POINTER_USE_COLLECTION

#include <set>
#include "llvm/IR/Instructions.h"
#include "FunctionDuplicater.hpp"
#include "PointerUse.hpp"

using namespace llvm;

class PointerUseCollection{
public:
  PointerUseCollection(FunctionDuplicater *FD);
  std::set<PointerUse *> PointerUses;
private:
  void CollectInstructions(std::set<Function *> Functions);
};

#endif
