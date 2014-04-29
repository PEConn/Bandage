#ifndef POINTER_ALLOCA_TRANSFORM_HPP
#define POINTER_ALLOCA_TRANSFORM_HPP

#include <set>
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

using namespace llvm;

class PointerAllocaTransform{
public:
  PointerAllocaTransform(std::set<Function *> Functions);
  std::map<AllocaInst *, AllocaInst *> RawToFPMap;
private:
  void CollectAllocas(std::set<Function *> Functions);
  void TransformAllocas();

  std::set<AllocaInst *> Allocas;
};

#endif
