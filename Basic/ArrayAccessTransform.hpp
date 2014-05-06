#ifndef ARRAY_LOAD_TRANSFORM
#define ARRAY_LOAD_TRANSFORM

#include <set>
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"

using namespace llvm;

class ArrayAccessTransform{
public:
  ArrayAccessTransform(std::set<Function *> Functions, Function *OnError);
private:
  Function *OnError;
  void CollectGeps(std::set<Function *> Functions);
  void TransformGeps();
  void AddBoundsCheck(GetElementPtrInst *Gep);
  std::set<GetElementPtrInst *> Geps;
};

#endif
