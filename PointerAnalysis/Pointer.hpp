#ifndef POINTER_HPP
#define POINTER_HPP

#include <set>
#include "llvm/IR/Instructions.h"

using namespace llvm;

enum CCuredPointerType {SAFE, SEQ, DYN};

class Pointer{
public:
  Pointer(AllocaInst *Declaration);
  void CollectUses();
  void DeterminePointerType();
  void Print();
private:
  void CheckStores();
  Value *FollowStore(StoreInst *S);
  AllocaInst *Declaration;
  CCuredPointerType PointerType = DYN;

  std::set<Value *> Uses;
};

#endif
