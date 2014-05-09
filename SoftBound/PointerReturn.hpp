#ifndef POINTER_RETURN
#define POINTER_RETURN

#include <map>
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

class PointerReturn{
public:
  Type *GetPointerReturnType(Type *T);
  void WrapInPointerReturn(Value *Val, Value *Base, Value *Bound, Value *FP, IRBuilder<> &B);
  void ExtractFromPointerReturn(Value *Val, Value *Base, Value *Bound, Value *FP, IRBuilder<> &B);
private:
  static std::map<Type *, Type *> PointerReturns;
};

#endif
