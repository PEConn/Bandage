#include "PointerUseCollection.hpp"

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

PointerUseCollection::PointerUseCollection(FunctionDuplicater *FD, Module &M){
  CollectInstructions(FD->GetFPFunctions(), M);
}
void PointerUseCollection::CollectInstructions(std::set<Function *> Functions, Module &M){
  std::vector<PointerUse *> PotentialUses;
  /*
  for(auto i = M.global_begin(), e = M.global_end(); i != e; ++i){
    if(auto C = dyn_cast<Constant>(i)){
      for(auto i = C->use_begin(), e = C->use_end(); i != e; ++i){

        auto *P = new PU(*i, C);
        //if(Functions.count(cast<Instruction>(
        //        P->Chain.back())->getParent()->getParent()))
          PotentialUses.push_back(P);
        //else
        //  delete P;
      }
    }
  }
  */

  for(auto F: Functions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;
      // Allocations
      if(auto A = dyn_cast<AllocaInst>(I)){
        for(auto i = A->use_begin(), e = A->use_end(); i != e; ++i){
          PotentialUses.push_back(new PU(*i, A));
        }
      }

      // Calls
      if(auto C = dyn_cast<CallInst>(I)){
        PotentialUses.push_back(new PointerParameter(C));
        for(auto i = C->use_begin(), e = C->use_end(); i != e; ++i){
          PotentialUses.push_back(new PU(*i, C));
        }
      }


      /*
      if(auto S = dyn_cast<StoreInst>(I)){
        PotentialUses.push_back(new PointerAssignment(S));
      } else if(auto R = dyn_cast<ReturnInst>(I)){
        PotentialUses.push_back(new PointerReturn(R));
      } else if(auto C = dyn_cast<CallInst>(I)){
        PotentialUses.push_back(new PointerParameter(C));
      } else if(auto C = dyn_cast<CmpInst>(I)){
        PotentialUses.push_back(new PointerCompare(C));
      }*/
    }
  }

  for(auto PU: PotentialUses){
    if(PU->IsValid())
      PointerUses.insert(PU);
    else
      delete PU;
  }
}
