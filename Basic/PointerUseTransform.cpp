#include "PointerUseTransform.hpp"
#include "llvm/Support/raw_ostream.h"
#include "Helpers.hpp"

PointerUseTransform::PointerUseTransform(PointerUseCollection *PUC, Module &M){
  this->PUC = PUC;
  this->M = &M;
}

void PointerUseTransform::Apply(){
  for(auto U: PUC->PointerUses)
    U->DispatchTransform(this);
}
void PointerUseTransform::ApplyTo(PointerAssignment *PA){
}
void PointerUseTransform::ApplyTo(PointerReturn *PR){
}
void PointerUseTransform::ApplyTo(PointerParameter *PP){
  PP->Print();
  // For each parameter
  for(int i=0; i<PP->ValueChains.size(); i++){
    // For each use
    for(int j=PP->ValueChains[i].size() - 1; j>=0; j--){
      Value *Link = PP->ValueChains[i][j];
      if(auto PointerLoad = dyn_cast<LoadInst>(Link)){
        //if(FatPointers::IsFatPointerType(PointerLoad->getPointerOperand()->getType())){
          errs() << "# " << *PointerLoad << "\n";
          IRBuilder<> B(PointerLoad);

          Value* FatPointer = PointerLoad->getPointerOperand(); 
          Value* RawPointer = 
              B.CreateGEP(FatPointer, GetIndices(0, PointerLoad->getContext()));
          Value* NewLoad = B.CreateLoad(RawPointer);

          PointerLoad->replaceAllUsesWith(NewLoad);
          PointerLoad->eraseFromParent();
        //}
      }
    }
  }
}
