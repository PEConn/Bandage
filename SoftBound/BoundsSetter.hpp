#ifndef BOUNDS_SETTER
#define BOUNDS_SETTER

#include <set>
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

#include "LocalBounds.hpp"
#include "HeapBounds.hpp"


class BoundsSetter{
public:
  BoundsSetter(std::set<Function *> Functions);
  void SetBounds(LocalBounds *LB, HeapBounds *HB);
private:
  std::set<StoreInst *> Stores;
  void CollectStores(std::set<Function *> Functions);
  bool SetOnMalloc(IRBuilder<> &B, StoreInst *S, Value *&StoreToLower, Value *&StoreToUpper);
  bool SetOnConstString(IRBuilder<> &B, StoreInst *S, Value *&StoreToLower, Value *&StoreToUpper);
  bool SetOnPointerEquals(IRBuilder<> &B, StoreInst *S, Value *&StoreToLower, Value *&StoreToUpper, LocalBounds *LB);
};

#endif
