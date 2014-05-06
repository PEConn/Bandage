#include "BoundsChecks.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

BoundsChecks::BoundsChecks(LocalBounds *LB, FunctionDuplicater *FD){
  this->LB = LB;
  this->FD = FD;
  CreateBoundsChecks();
}

void BoundsChecks::CreateBoundsChecks(){
  for(auto F: FD->FPFunctions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;
      if(auto L = dyn_cast<LoadInst>(I)){
        CreateBoundsCheck(L);
      }
    }
  }
}

void BoundsChecks::CreateBoundsCheck(LoadInst *L){
  errs() << "TODO: Create bounds check for " << *L << "\n";
}
