#ifndef LOCAL_BOUNDS_HPP
#define LOCAL_BOUNDS_HPP
#include <map>
#include "llvm/IR/Instructions.h"
#include "FunctionDuplicater.hpp"

using namespace llvm;

class LocalBounds{
public:
  LocalBounds(FunctionDuplicater *FD);
  Value *GetLowerBound(Value *V);
  Value *GetUpperBound(Value *V);

  Value *GetDef(Value *V);
private:
  std::map<Value *, Value *> LowerBounds;
  std::map<Value *, Value *> UpperBounds;
  void CreateBounds(FunctionDuplicater *FD);
  void CreateBound(AllocaInst *A);
  void CreateBound(CallInst *C);
};
#endif
