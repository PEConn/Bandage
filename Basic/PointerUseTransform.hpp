#ifndef POINTER_USE_TRANSFORM
#define POINTER_USE_TRANSFORM

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include "PointerUseCollection.hpp"

class PointerUseTransform{
public:
  PointerUseTransform(PointerUseCollection *PUC, Module &M);
  void Apply();
  void ApplyTo(PointerAssignment *PA);
  void ApplyTo(PointerReturn *PR);
  void ApplyTo(PointerParameter *PP);

private:
  Value *RecreateValueChain(std::vector<Value *> Chain, IRBuilder<> &B);
  PointerUseCollection *PUC;
  Module *M;
};


#endif
