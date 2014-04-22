#ifndef CONSTRAINTS_HPP
#define CONSTRAINTS_HPP

#include <string>
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "Pointer.hpp"

using namespace llvm;

class Constraint{
public:
  virtual std::string ToString() = 0;
};

class PointerArithmetic : public Constraint{
public:
  PointerArithmetic(Pointer P);
  virtual std::string ToString();
  Pointer P;
};

class SetToPointer : public Constraint{
public:
  SetToPointer(Pointer Lhs, Pointer Rhs);
  virtual std::string ToString();
  bool TypesMatch();
  Pointer Lhs;
  Pointer Rhs;
};

class SetToFunction : public Constraint{
public:
  SetToFunction(Pointer P, Function *F);
  virtual std::string ToString();
  Pointer P;
  Function *F;
};

class IsDynamic : public Constraint{
public:
  IsDynamic(Pointer P);
  virtual std::string ToString();
  Pointer P;
};

#endif
