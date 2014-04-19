#include "Constraints.hpp"
#include "llvm/Support/raw_ostream.h"

EqualsConstraint::EqualsConstraint(class Value *Pointer, int PointerLevel, class Value *Value, int ValueLevel){
  this->Pointer = Pointer;
  this->PointerLevel = PointerLevel;
  this->Value = Value;
  this->ValueLevel = ValueLevel;
}

void EqualsConstraint::Print(){
  int Diff = PointerLevel - ValueLevel;

  //errs() << PrettyString(Pointer, PointerLevel) << " = ";
  //errs() << (ValueLevel == -1 ? ("&" + Value->getName()) : PrettyString(Value, ValueLevel)) << ";\n";

  std::string PointerStr = PrettyString(Pointer, Diff);
  std::string ValueStr = PrettyString(Value, -Diff);

  errs() << "Q(" + ValueStr + ") <= Q(" + PointerStr + ")\n";
  errs() << "Q(" + ValueStr + ") == Q(" + PointerStr + ") = DYNQ";
  errs() << " || T(" + ValueStr + ") ~= T(" + PointerStr + ")\n";
}

ArithmeticConstraint::ArithmeticConstraint(class Value *Pointer, int PointerLevel){
  this->Pointer = Pointer;
  this->PointerLevel = PointerLevel;
}
void ArithmeticConstraint::Print(){
  std::string PointerStr = PrettyString(Pointer, PointerLevel);
  errs() << "Q(" << PointerStr << ") != SAFE\n";
}

std::string Constraint::PrettyString(Value *V, int level){
  std::string str;
  for(int i=0; i<level; i++)
    str += "*";
  str += V->getName();
  return str;
}
