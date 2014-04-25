#include "PointerUse.hpp"

#include "llvm/Support/raw_ostream.h"
#include "PointerUseTransform.hpp"
#include "FatPointers.hpp"
#include "../Basic/Helpers.hpp"

PointerAssignment::PointerAssignment(StoreInst *S){
  this->Store = S;
  FollowChains();
}
PointerReturn::PointerReturn(ReturnInst *R){
  this->Return = R;
  FollowChains();
}
PointerParameter::PointerParameter(CallInst *C){
  this->Call = C;
  FollowChains();
}

void PointerAssignment::FollowChains(){
  ValueChain.clear();
  PointerChain.clear();

  Value *ValueLink = Store->getValueOperand();
  Value *PointerLink = Store->getPointerOperand();

  while(GetLinkType(ValueLink) != NO_LINK){
    ValueChain.push_back(ValueLink);
    ValueLink = GetNextLink(ValueLink);
  }
  ValueChain.push_back(ValueLink);

  while(GetLinkType(PointerLink) != NO_LINK){
    PointerChain.push_back(PointerLink);
    PointerLink = GetNextLink(PointerLink);
  }
  PointerChain.push_back(PointerLink);
}
void PointerReturn::FollowChains(){
  ValueChain.clear();

  Value *ValueLink = Return->getReturnValue();
  if(!ValueLink)
    return;

  while(GetLinkType(ValueLink) != NO_LINK){
    ValueChain.push_back(ValueLink);
    ValueLink = GetNextLink(ValueLink);
  }
  ValueChain.push_back(ValueLink);
}
void PointerParameter::FollowChains(){
  ValueChains.clear();

  for(int i=0; i<Call->getNumArgOperands(); i++){
    Value *ValueLink = Call->getArgOperand(i);
    std::vector<Value *> ValueChain;

    while(GetLinkType(ValueLink) != NO_LINK){
      ValueChain.push_back(ValueLink);
      ValueLink = GetNextLink(ValueLink);
    }
    ValueChain.push_back(ValueLink);

    if(ValueLink->getType()->isPointerTy())
      ValueChains.push_back(ValueChain);
  }
}

bool PointerAssignment::IsValid(){
  Type *TP = PointerChain.back()->getType()->getPointerElementType();
  Type *TV = ValueChain.back()->getType();
  bool RawPtr = TP->isPointerTy() || TV->isPointerTy();

  return RawPtr;
}
bool PointerReturn::IsValid(){
  if(!Return->getReturnValue())
    return false;

  if(ValueChain.back()->getType()->isPointerTy()
      && ValueChain.back()->getType()->getPointerElementType()->isPointerTy())
    return true;
  return false;
}
bool PointerParameter::IsValid(){
  return ValueChains.size() != 0;
}

void PointerAssignment::Print(){
  errs() << "----------Assignment----------\n";
  errs() << *Store << "\n";
  errs() << "Value Chain:\n";
  for(auto V: ValueChain) errs() << *V << "\n";
  errs() << "Pointer Chain:\n";
  for(auto V: PointerChain) errs() << *V << "\n";
}
void PointerReturn::Print(){
  errs() << "----------  Return  ----------\n";
  errs() << *Return << "\n";
  errs() << "Value Chain:\n";
  for(auto V: ValueChain) errs() << *V << "\n";
}
void PointerParameter::Print(){
  errs() << "----------   Call   ----------\n";
  errs() << *Call << "\n";
  for(auto VC: ValueChains){
    errs() << "Value Chain:\n";
    for(auto V: VC) errs() << *V << "\n";
  }
}

void PointerAssignment::DispatchTransform(PointerUseTransform *T){
  T->ApplyTo(this);
}
void PointerReturn::DispatchTransform(PointerUseTransform *T){
  T->ApplyTo(this);
}
void PointerParameter::DispatchTransform(PointerUseTransform *T){
  T->ApplyTo(this);
}
