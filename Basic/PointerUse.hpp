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

class PointerAssignment : public PointerUse{
public:
  PointerAssignment(StoreInst *S);
  virtual void Print();
  virtual bool IsValid();
  virtual ~PointerAssignment(){}
  virtual void DispatchTransform(PointerUseTransform *);
private:
  void FollowChains();
  StoreInst *Store;
  std::vector<Value *> PointerChain;
  std::vector<Value *> ValueChain;
};

class PointerReturn : public PointerUse{
public:
  PointerReturn(ReturnInst *R);
  virtual void Print();
  virtual bool IsValid();
  virtual ~PointerReturn (){}
  virtual void DispatchTransform(PointerUseTransform *);
private:
  void FollowChains();
  ReturnInst *Return;
  std::vector<Value *> ValueChain;
};

class PointerParameter : public PointerUse{
public:
  PointerParameter(CallInst *C);
  virtual void Print();
  virtual bool IsValid();
  virtual ~PointerParameter (){}
  virtual void DispatchTransform(PointerUseTransform *);
  std::vector<std::vector<Value *>> ValueChains;
private:
  void FollowChains();
  CallInst *Call;
};

#endif
