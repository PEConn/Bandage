#include "Constraints.hpp"
#include "llvm/Support/raw_ostream.h"

PointerArithmetic::PointerArithmetic(Pointer P){
  this->P = P;
}
SetToPointer::SetToPointer(Pointer Lhs, Pointer Rhs){
  this->Lhs = Lhs;
  this->Rhs = Rhs;
}
SetToFunction::SetToFunction(Pointer P, Function *F){
  this->P = P;
  this->F = F;
}
IsDynamic::IsDynamic(Pointer P){
  this->P = P;
}

std::string PointerArithmetic::ToString(){
  return "Pointer Arithmetic on " + P.ToString();
}
std::string SetToPointer::ToString(){
  return Lhs.ToString() + " set to " + Rhs.ToString();
}
std::string SetToFunction::ToString(){
  return P.ToString() + " set to " + (std::string)F->getName();
}
std::string IsDynamic::ToString(){
  return P.ToString() + " is dynamic";
}

bool SetToPointer::TypesMatch(){
  Type *LhsType = Lhs.id->getType();
  Type *RhsType = Rhs.id->getType();

  int LevelDiff = Lhs.level - Rhs.level;
  for(int i=0; i<LevelDiff; i++) LhsType = LhsType->getPointerElementType();
  for(int i=0; i>LevelDiff; i--) RhsType = RhsType->getPointerElementType();

  return LhsType == RhsType;
}
