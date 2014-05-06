#ifndef LOCAL_BOUNDS_HPP
#define LOCAL_BOUNDS_HPP
#include <map>
#include "llvm/IR/Instructions.h"

using namespace llvm;

class LocalBounds{
public:
  LocalBounds(Module &M);
  std::map<Value *, Value *> LowerBounds;
  std::map<Value *, Value *> UpperBounds;
private:
  void CreateBounds(Module &M);
  void CreateBound(AllocaInst *A);
};
#endif
