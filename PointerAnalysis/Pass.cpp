#include <set>
#include <vector>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

#include "Pointer.hpp"
#include "Constraints.hpp"

using namespace llvm;

namespace {
struct PointerAnalysis : public ModulePass{
  static char ID;
  PointerAnalysis() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    CollectPointers(M);

    return false;
  }
  void CollectPointers(Module &M);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {}
private:
  std::set<Pointer *> Pointers;
  std::set<Constraint *> Constraints;
};
}

void PointerAnalysis::CollectPointers(Module &M){
  // Collect Pointers here
  std::set<Value *> Pointers;
  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      Instruction *I = &*II;
      AllocaInst *A;
      if(!(A = dyn_cast<AllocaInst>(I)))
        continue;
      if(A->getAllocatedType()->isPointerTy())
        Pointers.insert(A);
    }
  }

  // Collect Pointer Equals
  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      Instruction *I = &*II;
      StoreInst *S;
      if(!(S = dyn_cast<StoreInst>(I)))
        continue;

      Value *ValueOperand = S->getValueOperand();
      int ValueLevel = -1;
      while(dyn_cast<LoadInst>(ValueOperand)
          || dyn_cast<GetElementPtrInst>(ValueOperand)){
        if(dyn_cast<LoadInst>(ValueOperand)){
          ValueLevel++;
          ValueOperand = (cast<LoadInst>(ValueOperand))->getPointerOperand();
        }else{
          ValueOperand = (cast<GetElementPtrInst>(ValueOperand))->getPointerOperand();
        }
      }

      Value *PointerOperand = S->getPointerOperand();
      int PointerLevel = 0;
      while(dyn_cast<LoadInst>(PointerOperand)){
        PointerLevel++;
        PointerOperand = (cast<LoadInst>(PointerOperand))->getPointerOperand();
      }

      if(!Pointers.count(PointerOperand))
        continue;

      if(!dyn_cast<AllocaInst>(ValueOperand))
        continue;

      Constraints.insert(new EqualsConstraint(PointerOperand, PointerLevel,
            ValueOperand, ValueLevel));
    }
  }

  // Collect Pointer Equals
  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      Instruction *I = &*II;
      GetElementPtrInst *G;
      if(!(G = dyn_cast<GetElementPtrInst>(I)))
        continue;

      Value *PointerOperand = G->getPointerOperand();
      int PointerLevel = -1;
      while(dyn_cast<LoadInst>(PointerOperand)){
        PointerLevel++;
        PointerOperand = (cast<LoadInst>(PointerOperand))->getPointerOperand();
      }

      if(!Pointers.count(PointerOperand))
        continue;

      Constraints.insert(new ArithmeticConstraint(PointerOperand, PointerLevel));
    }
  }

  for(auto C: Constraints){
    C->Print();
  }
}

char PointerAnalysis::ID = 0;
static RegisterPass<PointerAnalysis> X("pointer-analysis", "Pointer Analysis Pass", false, false);
