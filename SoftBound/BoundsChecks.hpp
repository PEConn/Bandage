#ifndef BOUNDS_CHECKS_HPP
#define BOUNDS_CHECKS_HPP
#include "llvm/IR/Instruction.h"
#include "LocalBounds.hpp"
#include "HeapBounds.hpp"

using namespace llvm;

class BoundsChecks{
public:
  BoundsChecks(LocalBounds *LB, HeapBounds *HB, FunctionDuplicater *FD);
  void CreateBoundsCheckFunction(Module &M, Function *Print);
  void CreateBoundsChecks();
private:
  LocalBounds *LB;
  FunctionDuplicater *FD;
  HeapBounds *HB;
  Function *BoundsCheck;
  void CreateBoundsCheck(Instruction *I, Value *PointerOperand);
};
#endif
