#ifndef LOCAL_BOUNDS_HPP
#define LOCAL_BOUNDS_HPP
#include <map>
#include "llvm/IR/Instructions.h"
#include "FunctionDuplicater.hpp"

using namespace llvm;

class LocalBounds{
public:
  LocalBounds(FunctionDuplicater *FD);
  Value *GetLowerBound(Value *V, bool IgnoreOneLoad = true);
  Value *GetUpperBound(Value *V, bool IgnoreOneLoad = true);
  bool HasBoundsFor(Value *V, bool IgnoreOneLoad = true);

  Value *GetDef(Value *V, bool IgnoreOneLoad = true);
private:
  std::map<Value *, Value *> LowerBounds;
  std::map<Value *, Value *> UpperBounds;
  void CreateBounds(FunctionDuplicater *FD);
  void CreateBound(AllocaInst *A);
  void CreateBound(CallInst *C);
};
#endif
