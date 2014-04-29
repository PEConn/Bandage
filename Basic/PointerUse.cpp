#include "PointerUse.hpp"

#include "llvm/Support/raw_ostream.h"
#include "PointerUseTransform.hpp"
#include "FatPointers.hpp"
#include "../Basic/Helpers.hpp"

PointerAssignment::PointerAssignment(StoreInst *S){
  this->Load = NULL;
  this->Store = S;
  FollowChains();
}
PointerCompare::PointerCompare(CmpInst *C){
  this->Cmp = C;
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
  // Store the last value load seperately from the rest of the chain
  if(auto L = dyn_cast<LoadInst>(ValueLink)){
    this->Load = L;
    ValueLink = GetNextLink(ValueLink);
  }
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
void PointerCompare::FollowChains(){
  Chain1.clear();
  Chain2.clear();

  Value *Link1 = Cmp->getOperand(0);
  Value *Link2 = Cmp->getOperand(1);

  while(GetLinkType(Link1) != NO_LINK){
    Chain1.push_back(Link1);
    Link1 = GetNextLink(Link1);
  }
  Chain1.push_back(Link1);

  while(GetLinkType(Link2) != NO_LINK){
    Chain2.push_back(Link2);
    Link2 = GetNextLink(Link2);
  }
  Chain2.push_back(Link2);
}
void PointerReturn::FollowChains(){
  ValueChain.clear();

  Value *ValueLink = Return->getReturnValue();
  if(!ValueLink)
    return;
  
  auto L = dyn_cast<LoadInst>(ValueLink);
  Load = L;
  if(L)
    ValueLink = GetNextLink(ValueLink);

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

    auto L = dyn_cast<LoadInst>(ValueLink);
    Loads.push_back(L);
    if(L)
      ValueLink = GetNextLink(ValueLink);

    while(GetLinkType(ValueLink) != NO_LINK){
      ValueChain.push_back(ValueLink);
      ValueLink = GetNextLink(ValueLink);
    }
    ValueChain.push_back(ValueLink);

    ValueChains.push_back(ValueChain);
  }
}

bool PointerAssignment::IsValid(){
  Type *TP = PointerChain.back()->getType()->getPointerElementType();
  Type *TV = ValueChain.back()->getType();
  bool RawPtr = TP->isPointerTy() || TV->isPointerTy();

  return RawPtr;
}
bool PointerCompare::IsValid(){
  Type *T1 = Chain1.back()->getType();
  Type *T2 = Chain2.back()->getType();
  return T1->isPointerTy() || T2->isPointerTy();
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
  return true;
  //return ValueChains.size() != 0;
}

void PointerAssignment::Print(){
  errs() << "----------Assignment----------\n";
  errs() << *Store << "\n";
  errs() << "Value Chain:\n";
  for(auto V: ValueChain) errs() << *V << "\n";
  errs() << "Pointer Chain:\n";
  for(auto V: PointerChain) errs() << *V << "\n";
}
void PointerCompare::Print(){
  errs() << "----------   Cmp    ----------\n";
  errs() << *Cmp << "\n";
  errs() << "Chain1:\n";
  for(auto V: Chain1) errs() << *V << "\n";
  errs() << "Chain2:\n";
  for(auto V: Chain2) errs() << *V << "\n";
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
void PointerCompare::DispatchTransform(PointerUseTransform *T){
  T->ApplyTo(this);
}
void PointerReturn::DispatchTransform(PointerUseTransform *T){
  T->ApplyTo(this);
}
void PointerParameter::DispatchTransform(PointerUseTransform *T){
  T->ApplyTo(this);
}

void PU::Print(){
  errs() << "Use chain for " << *Orig << "\n";
  for(auto L: Chain){
    if(L)
      errs() << "| " << *L << "\n";
    else
      errs() << "| " << "<NULL>" << "\n";
  }
  errs() << "\\--------------\n";
}
bool PU::IsValid(){
  return true;
}
PU::PU(Value *V, Value *O){
  this->Orig = O;
  
  Chain.push_back(O);
  Chain.push_back(V);
  while(V->use_begin() != V->use_end()){
    V = *V->use_begin();
    Chain.push_back(V);
  }
}
void PU::DispatchTransform(PointerUseTransform *T){
  T->ApplyTo(this);
}
