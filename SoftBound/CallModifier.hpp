#ifndef CALL_MODIFIER_HPP
#define CALL_MODIFIER_HPP

#include "FunctionDuplicater.hpp"
#include "LocalBounds.hpp"

class CallModifier{
public:
  CallModifier(FunctionDuplicater *FD, LocalBounds *LB);
  ~CallModifier();
private:
  FunctionDuplicater *FD;
  LocalBounds *LB;
  PointerReturn *PR;

  void ModifyCalls();
  void ModifyCall(CallInst *C);
  void WrapReturn(ReturnInst *R);
};

#endif
