#ifndef CALL_MODIFIER_HPP
#define CALL_MODIFIER_HPP

#include "FunctionDuplicater.hpp"
#include "LocalBounds.hpp"
#include "HeapBounds.hpp"

class CallModifier{
public:
  CallModifier(FunctionDuplicater *FD, LocalBounds *LB, HeapBounds *HB);
  ~CallModifier();
private:
  FunctionDuplicater *FD;
  LocalBounds *LB;
  HeapBounds *HB;
  PointerReturn *PR;

  void ModifyCalls();
  void ModifyCall(CallInst *C);
  void WrapReturn(ReturnInst *R);
};

#endif
