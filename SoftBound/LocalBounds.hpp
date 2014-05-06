#ifndef LOCAL_BOUNDS_HPP
#define LOCAL_BOUNDS_HPP
#include <map>
#include "llvm/IR/Instructions.h"
#include "FunctionDuplicater.hpp"

using namespace llvm;

class LocalBounds{
public:
  LocalBounds(FunctionDuplicater *FD);
  std::map<Value *, Value *> LowerBounds;
  std::map<Value *, Value *> UpperBounds;
private:
  void CreateBounds(FunctionDuplicater *FD);
  void CreateBound(AllocaInst *A);
};
#endif
