#include "PointerUse.hpp"

#include "llvm/Support/raw_ostream.h"
#include "PointerUseTransform.hpp"
#include "FatPointers.hpp"

enum LinkType {LOAD, GEP, CAST, NO_LINK};

LinkType GetLinkType(Value *V){
  if(isa<LoadInst>(V))
    return LOAD;
  else if (isa<GetElementPtrInst>(V))
    return GEP;
  else if (isa<CastInst>(V))
    return CAST;
  else
    return NO_LINK;
}
Value *GetNextLink(Value *Link){
  if(auto L = dyn_cast<LoadInst>(Link))
    return L->getPointerOperand();
  else if(auto G = dyn_cast<GetElementPtrInst>(Link))
    return G->getPointerOperand();
  else if(auto B = dyn_cast<CastInst>(Link))
    return B->getOperand(0);
  assert(false && "GetNextLink has been given invalid link type");
  return NULL;
}

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
  Type *T = PointerChain.back()->getType()->getPointerElementType();
  bool RawPtr = T->isPointerTy();
  bool FatPtr = FatPointers::IsFatPointerType(T);

  return FatPtr;
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
