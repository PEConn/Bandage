#include "PointerUse.hpp"

#include "llvm/Support/raw_ostream.h"
#include "PointerUseTransform.hpp"
#include "FatPointers.hpp"
#include "../Basic/Helpers.hpp"

PointerParameter::PointerParameter(CallInst *C){
  this->Call = C;
  FollowChains();
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
bool PointerParameter::IsValid(){
  return true;
  //return ValueChains.size() != 0;
}
void PointerParameter::Print(){
  errs() << "----------   Call   ----------\n";
  errs() << *Call << "\n";
  for(auto VC: ValueChains){
    errs() << "Value Chain:\n";
    for(auto V: VC) errs() << *V << "\n";
  }
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
    if(isa<CallInst>(V))
      break;
    // Don't follow if we're the index to a GEP
    if(auto G = dyn_cast<GetElementPtrInst>(V))
      if(G->getPointerOperand() != Chain[Chain.size() - 2])
        break;
  }
}
void PU::DispatchTransform(PointerUseTransform *T){
  T->ApplyTo(this);
}
