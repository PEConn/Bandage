#include "BoundsChecks.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/InstIterator.h"

BoundsChecks::BoundsChecks(LocalBounds *LB, Module &M){
  this->LB = LB;
  this->M = &M;
  CreateBoundsChecks();
}

void BoundsChecks::CreateBoundsChecks(){
  for(auto IF = M->begin(), EF = M->end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      Instruction *I = &*II;
      if(auto L = dyn_cast<LoadInst>(I)){
        CreateBoundsCheck(L);
      }
    }
  }
}

void BoundsChecks::CreateBoundsCheck(LoadInst *L){
}
