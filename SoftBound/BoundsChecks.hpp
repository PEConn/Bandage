#ifndef BOUNDS_CHECKS_HPP
#define BOUNDS_CHECKS_HPP
#include "llvm/IR/Instruction.h"
#include "LocalBounds.hpp"

using namespace llvm;

class BoundsChecks{
public:
  BoundsChecks(LocalBounds *LB, Module &M);
private:
  LocalBounds *LB;
  Module *M;
  void CreateBoundsChecks();
  void CreateBoundsCheck(LoadInst *L);
};
#endif
