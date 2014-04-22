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
    SolveConstraints();

    return false;
  }
  void CollectPointers(Module &M);
  void SolveConstraints();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {}
private:
  std::set<Pointer> PointerUses;

  std::set<IsDynamic *> IDCons;
  std::set<SetToFunction *> STFCons;
  std::set<SetToPointer *> STPCons;
  std::set<PointerArithmetic *> PACons;
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

      // Follow the Value operand through loads and Geps to the originating alloc
      Value *ValueOperand = S->getValueOperand();
      int ValueLevel = -1;
      while(true){
        if(auto L = dyn_cast<LoadInst>(ValueOperand)){
          ValueLevel++;
          ValueOperand = L->getPointerOperand();
        } else if(auto Gep = dyn_cast<GetElementPtrInst>(ValueOperand)){
          ValueOperand = Gep->getPointerOperand();
        } else if(auto B = dyn_cast<BitCastInst>(ValueOperand)){
          ValueOperand = B->getOperand(0);
        } else {
          break;
        }
      }
      // Follow the Pointer operand through loads to the originating alloc
      Value *PointerOperand = S->getPointerOperand();
      int PointerLevel = 0;
      while(dyn_cast<LoadInst>(PointerOperand)){
        PointerLevel++;
        PointerOperand = (cast<LoadInst>(PointerOperand))->getPointerOperand();
      }

      // Only bother for proper pointers (ones we have seen alloced)
      if(!Pointers.count(PointerOperand))
        continue;

      // Note if the store comes from an assignment from another variable 
      // or a function
      auto P = Pointer(PointerOperand, PointerLevel);
      auto V = Pointer(ValueOperand, ValueLevel);
      PointerUses.insert(P);
      if(dyn_cast<AllocaInst>(ValueOperand)){
        PointerUses.insert(V);
        STPCons.insert(new SetToPointer(P, V));
      } else if (auto C = dyn_cast<CallInst>(ValueOperand)){
        STFCons.insert(new SetToFunction(P, C->getCalledFunction()));
      } else if (auto I = dyn_cast<IntToPtrInst>(ValueOperand)){
        IDCons.insert(new IsDynamic(P));
      } else if (auto C = dyn_cast<Constant>(ValueOperand)){
        IDCons.insert(new IsDynamic(P));
      } else {
        errs() << P.ToString() << " set to:\n" << *ValueOperand << "\n";
      }
    }
  }

  // Collect Pointer Arithmetic
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

      PointerUses.insert(Pointer(PointerOperand, PointerLevel));
      PACons.insert(new PointerArithmetic(Pointer(PointerOperand, PointerLevel)));
    }
  }

  errs() << "Pointers:\n";
  for(auto P: Pointers) errs() << P->getName() << "\n";
  
  errs() << "Pointer Uses:\n";
  for(auto PU: PointerUses) errs() << PU.ToString() << "\n";
  
  errs() << "Constraints:\n";
  for(auto C: IDCons) errs() << C->ToString() << "\n";
  for(auto C: STFCons) errs() << C->ToString() << "\n";
  for(auto C: STPCons) errs() << C->ToString() << "\n";
  for(auto C: PACons) errs() << C->ToString() << "\n";
}

void PointerAnalysis::SolveConstraints(){
  // Create a map, from pointer uses to designation
  std::map<Pointer, CCuredPointerType> Qs;
  for(auto PU: PointerUses) Qs[PU] = UNSET;

  for(auto C: IDCons){
    Qs[C->P] = DYNQ;
  }
  
  for(auto C: STFCons){
    // May expand the qualifiers to function return types
  }

  // If any pointer is set to a value of a different type, set it as DYNQ
  // ...

  // Propegate, until settling the DYNQ values
  bool change = false;
  do{
    change = false;
    // Propegate due to pointer equals to
    for(auto C: STPCons){
      if((Qs[C->Lhs] == DYNQ && Qs[C->Rhs] != DYNQ)
          || (Qs[C->Lhs] != DYNQ && Qs[C->Rhs] == DYNQ)){
        Qs[C->Lhs] = DYNQ;
        Qs[C->Rhs] = DYNQ;
        change = true;
      }
    }
    // If a pointer is dynamic, anything it points to must also be dynamic
    for(auto PU: PointerUses){
      if(Qs[PU] == DYNQ){
        auto PointsTo = Pointer(PU.id, PU.level + 1);
        if(Qs.count(PointsTo) && Qs[PointsTo] != DYNQ){
          Qs[PointsTo] = DYNQ;
          change = true;
        }
      }
    }
  } while(change);

  for(auto C: PACons)
    if(Qs[C->P] != DYNQ)
      Qs[C->P] = SEQ;

  for(auto PU: PointerUses)
    if(Qs[PU] == UNSET)
      Qs[PU] = SAFE;

  errs() << "Results:\n";
  for(auto Pair: Qs) 
    errs() << Pair.first.ToString() << ": " << Pretty(Pair.second) << "\n";
}

char PointerAnalysis::ID = 0;
static RegisterPass<PointerAnalysis> X("pointer-analysis", "Pointer Analysis Pass", false, false);
