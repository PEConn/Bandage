#include "LocalBounds.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/InstIterator.h"

LocalBounds::LocalBounds(FunctionDuplicater *FD){
  CreateBounds(FD);
}

void LocalBounds::CreateBounds(FunctionDuplicater *FD){
  for(auto F: FD->FPFunctions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;
      if(auto A = dyn_cast<AllocaInst>(I)){
        if(A->getType()->getPointerElementType()->isPointerTy())
          CreateBound(A);
      }
    }
  }
}

void LocalBounds::CreateBound(AllocaInst *A){
  IRBuilder<> B(A);
  LowerBounds[A] = B.CreateAlloca(A->getType()->getPointerElementType());
  UpperBounds[A] = B.CreateAlloca(A->getType()->getPointerElementType());

  Value *Null = ConstantPointerNull::get(
      cast<PointerType>(A->getType()->getPointerElementType()));
  B.CreateStore(Null, LowerBounds[A]);
  B.CreateStore(Null, UpperBounds[A]);
}
