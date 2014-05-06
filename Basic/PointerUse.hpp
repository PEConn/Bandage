#ifndef POINTER_USE_HPP
#define POINTER_USE_HPP

#include <vector>
#include "llvm/IR/Instructions.h"
using namespace llvm;

class PointerUseTransform;

class PointerUse{
public:
  virtual void Print() = 0;
  virtual bool IsValid() = 0;
  virtual ~PointerUse(){};
  virtual void DispatchTransform(PointerUseTransform *) = 0;
};

class PU : public PointerUse{
public:
  virtual void Print();
  virtual bool IsValid();
  virtual void DispatchTransform(PointerUseTransform *);
  PU(Value *FirstElementOfUseChain, Value *Originator);
  ~PU(){}
  std::vector<Value *> Chain;
  Value *Orig;
};

class PointerParameter : public PointerUse{
public:
  PointerParameter(CallInst *C);
  virtual void Print();
  virtual bool IsValid();
  virtual ~PointerParameter (){}
  virtual void DispatchTransform(PointerUseTransform *);
  std::vector<std::vector<Value *>> ValueChains;
  std::vector<LoadInst *> Loads;
  void FollowChains();
  CallInst *Call;
};


#endif
