#ifndef CONSTRAINTS_HPP
#define CONSTRAINTS_HPP

#include <string>
#include "llvm/IR/Instructions.h"

using namespace llvm;

class Constraint{
public:
  virtual void Print() = 0;
protected:
  std::string PrettyString(Value *V, int level);
};

class EqualsConstraint : public Constraint{
public:
  EqualsConstraint(class Value *Pointer, int PointerLevel, class Value *Value, int ValueLevel);
  virtual void Print();
private:
  Value *Pointer;
  Value *Value;
  int PointerLevel;
  int ValueLevel;
};

class ArithmeticConstraint : public Constraint{
public:
  ArithmeticConstraint(class Value *Pointer, int PointerLevel);
  virtual void Print();
private:
  Value *Pointer;
  int PointerLevel;
};

#endif
