#include "llvm/Support/raw_ostream.h"
#include "PointerAllocaTransform.hpp"
#include <string>
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/InstIterator.h"
#include "Helpers.hpp"
#include "FatPointers.hpp"


PointerAllocaTransform::PointerAllocaTransform(std::set<Function *> Functions, Module &M){
  CollectAllocas(Functions, M);
  TransformAllocas();
}
void PointerAllocaTransform::CollectAllocas(std::set<Function *> Functions, Module &M){
  for(auto F: Functions){
    //errs() << "Transforming Allocas in " << F->getName() << "\n";
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
    //errs() << *PointerAlloc << " -> ";
    BasicBlock::iterator iter = PointerAlloc;
    iter++;
    IRBuilder<> B(iter);

    Type *PointerTy = PointerAlloc->getType()->getPointerElementType();
    std::string Name = "FP." + PointerAlloc->getName().str();
    // Construct the type for the fat pointer
    Value *FatPointer = FatPointers::CreateFatPointer(PointerTy, B, Name);
    //errs() << *FatPointer << "\n";
    RawToFPMap[PointerAlloc] = cast<AllocaInst>(FatPointer);
    PointerAlloc->replaceAllUsesWith(FatPointer);

    // Initialise the value, base and bound to null
    Value *Null = FatPointers::GetFieldNull(FatPointer);
    StoreInFatPointerValue(FatPointer, Null, B);
    //StoreInFatPointerBase(FatPointer, Null, B);
    //StoreInFatPointerBound(FatPointer, Null, B);
  }
}
