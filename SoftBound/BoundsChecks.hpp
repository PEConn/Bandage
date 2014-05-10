#ifndef BOUNDS_CHECKS_HPP
#define BOUNDS_CHECKS_HPP
#include "llvm/IR/Instruction.h"
#include "LocalBounds.hpp"

using namespace llvm;

class BoundsChecks{
public:
  BoundsChecks(LocalBounds *LB, FunctionDuplicater *FD);
  void CreateBoundsCheckFunction(Module &M, Function *Print);
  void CreateBoundsChecks();
private:
  LocalBounds *LB;
  FunctionDuplicater *FD;
  Function *BoundsCheck;
  void CreateBoundsCheck(LoadInst *L);
};
#endif
