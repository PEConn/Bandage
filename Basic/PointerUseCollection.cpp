#include "PointerUseCollection.hpp"

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

PointerUseCollection::PointerUseCollection(FunctionDuplicater *FD){
  CollectInstructions(FD->GetFPFunctions());
}
void PointerUseCollection::CollectInstructions(std::set<Function *> Functions){
  std::vector<PointerUse *> PotentialUses;
  for(auto F: Functions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;
      if(auto S = dyn_cast<StoreInst>(I)){
        PotentialUses.push_back(new PointerAssignment(S));
      } else if(auto R = dyn_cast<ReturnInst>(I)){
        PotentialUses.push_back(new PointerReturn(R));
      } else if(auto C = dyn_cast<CallInst>(I)){
        PotentialUses.push_back(new PointerParameter(C));
      } else if(auto C = dyn_cast<CmpInst>(I)){
        PotentialUses.push_back(new PointerCompare(C));
      }
    }
  }

  for(auto PU: PotentialUses){
    if(PU->IsValid())
      PointerUses.insert(PU);
    else
      delete PU;
  }
}
