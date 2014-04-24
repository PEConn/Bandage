#include "PointerAllocaTransform.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/InstIterator.h"
#include "Helpers.hpp"
#include "FatPointers.hpp"


PointerAllocaTransform::PointerAllocaTransform(std::set<Function *> Functions){
  CollectAllocas(Functions);
  TransformAllocas();
}
void PointerAllocaTransform::CollectAllocas(std::set<Function *> Functions){
  for(auto F: Functions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;
      if(AllocaInst *A = dyn_cast<AllocaInst>(I)){
        if(!I->getType()->getPointerElementType()->isPointerTy())
          continue;

        Allocas.insert(A);
      }
    }
  }
}
void PointerAllocaTransform::TransformAllocas(){
  for(auto PointerAlloc : Allocas){
    BasicBlock::iterator iter = PointerAlloc;
    iter++;
    IRBuilder<> B(iter);

    Type *PointerType = PointerAlloc->getType()->getPointerElementType();

    // Construct the type for the fat pointer
    Type *FatPointerType = FatPointers::GetFatPointerType(PointerType);

    Value* FatPointer = B.CreateAlloca(FatPointerType, NULL, "fp" + PointerAlloc->getName());
    PointerAlloc->replaceAllUsesWith(FatPointer);
    // All code below can use the original PointerAlloc

    // Initialize Value to current value and everything else to zero
    /*std::vector<Value *> ValueIdx = GetIndices(0, PointerAlloc->getContext());
    std::vector<Value *> BaseIdx = GetIndices(1, PointerAlloc->getContext());
    std::vector<Value *> LengthIdx = GetIndices(2, PointerAlloc->getContext());
    Value *FatPointerValue = B.CreateGEP(FatPointer, ValueIdx); 
    Value *FatPointerBase = B.CreateGEP(FatPointer, BaseIdx); 
    Value *FatPointerBound = B.CreateGEP(FatPointer, LengthIdx); 

    Value *Address = B.CreateLoad(PointerAlloc);
    B.CreateStore(Address, FatPointerValue);
    B.CreateStore(Address, FatPointerBase);
    B.CreateStore(Address, FatPointerBound);
    */
  }
}
