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
  bool SetOnMalloc(StoreInst *S, Value *Lower, Value *Upper);
  bool SetOnConstString(StoreInst *S, Value *Lower, Value *Upper);
  bool SetOnPointerEquals(StoreInst *S, Value *Lower, Value *Upper, LocalBounds *LB);
  //bool SetOnAddress(Value *ValueOperand, Value *UpperBound);
  //bool SetOnConstString(Value *ValueOperand, Value *UpperBound);
  //bool SetOnNull(Value *ValueOperand, Value *UpperBound);
};

#endif
