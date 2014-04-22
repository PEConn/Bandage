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
  ~PointerAnalysis();

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
  std::map<Function *, Pointer> FunctionReturns;
  std::map<std::pair<Function *, int>, Pointer> FunctionParameters;
  std::map<Value *, Value *> HiddenParamToParam;

  std::set<IsDynamic *> IDCons;
  std::set<SetToFunction *> STFCons;
  std::set<SetToPointer *> STPCons;
  std::set<PointerArithmetic *> PACons;
};
}

void PointerAnalysis::CollectPointers(Module &M){
  // Collect Pointer Allocations, Function Returns and Function Parameters
  std::set<Value *> Pointers;
  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      Instruction *I = &*II;

      if(auto A = dyn_cast<AllocaInst>(I)){
        if(A->hasName() && A->getAllocatedType()->isPointerTy()){
          PointerUses.insert(Pointer(A, 0));
          Pointers.insert(A);
        }
      }

      // If a pointer is return, associate the pointer with a function
      if(auto R = dyn_cast<ReturnInst>(I)){
        if(auto V = R->getReturnValue()){
          if(V->getType()->isPointerTy()){
            int Level = -1;
            while(isa<LoadInst>(V)){
              // TODO: Make this deal with Geps
              V = cast<LoadInst>(V)->getPointerOperand();
              Level++;
            }
            if(auto A = dyn_cast<AllocaInst>(V)){
              FunctionReturns[IF] = Pointer(A, Level);
            }
          }
        }
      }
    }
    // Remember the arguments to the function
    int i = 0;
    for(auto IA = IF->arg_begin(), EA = IF->arg_end(); IA != EA; ++IA){
      for(auto IU = IA->use_begin(), EU = IA->use_end(); IU != EU; ++IU){
        if(auto S = dyn_cast<StoreInst>(*IU))
          HiddenParamToParam[S->getPointerOperand()] = S->getValueOperand();
      }
      Pointers.insert(IA);
      FunctionParameters[std::make_pair(IF, i)] = Pointer(IA, 0);
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

      // Follow back through a parameter if needed
      if(HiddenParamToParam.count(ValueOperand))
        ValueOperand = HiddenParamToParam[ValueOperand];

      if(HiddenParamToParam.count(PointerOperand))
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
      } else if (auto Arg = dyn_cast<Argument>(ValueOperand)){
        PointerUses.insert(Pointer(ValueOperand, 0));
        STPCons.insert(new SetToPointer(P, Pointer(ValueOperand, 0)));
      } else {
        errs() << P.ToString() << " set to: " << *ValueOperand << "\n";
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

  errs() << "Pointer Uses:\n";
  for(auto PU: PointerUses) errs() << PU.ToString() << "\n";
  
  errs() << "Function Returns:\n";
  for(auto Pair: FunctionReturns)
    errs() << Pair.first->getName() << " returns " << Pair.second.ToString() << "\n";
  
  errs() << "Function Parameters:\n";
  for(auto Pair: FunctionParameters)
    errs() << Pair.first.first->getName() << ": " << Pair.first.second << " is " << Pair.second.ToString() << "\n";

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
  
  errs() << "Linking Function Returns:\n";
  for(auto C: STFCons){
    if(FunctionReturns.count(C->F)){
      errs() << "Added " << C->P.ToString() 
        << " set to " << FunctionReturns[C->F].ToString() << "\n";
      STPCons.insert(new SetToPointer(C->P, FunctionReturns[C->F]));
    } else {
      errs() << "Could not link " << C->P.ToString() << "\n";
    }
  }

  // If any pointer is set to a value of a different type, set it as DYNQ
  for(auto C: STPCons){
    if(!C->TypesMatch()){
      Qs[C->Lhs] = DYNQ;
      Qs[C->Rhs] = DYNQ;
    }
  }

  // For all pointers with arithmetic constraints, set to SEQ
  for(auto C: PACons)
    if(Qs[C->P] != DYNQ)
      Qs[C->P] = SEQ;

  // Propegate, until settling the DYNQ and SEQ values
  bool change = false;
  do{
    change = false;
    // Propegate due to pointer equals to
    for(auto C: STPCons){
      if(Qs[C->Lhs] == Qs[C->Rhs])
        continue;

      if(Qs[C->Lhs] == DYNQ || Qs[C->Rhs] == DYNQ){
        Qs[C->Lhs] = DYNQ;
        Qs[C->Rhs] = DYNQ;
        change = true;
      }
      
      if(Qs[C->Lhs] == SEQ && Qs[C->Rhs] == UNSET){
        Qs[C->Rhs] = SEQ;
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

  for(auto PU: PointerUses)
    if(Qs[PU] == UNSET)
      Qs[PU] = SAFE;

  errs() << "Results:\n";
  for(auto Pair: Qs) 
    errs() << Pair.first.ToString() << ": " << Pretty(Pair.second) << "\n";
}

PointerAnalysis::~PointerAnalysis(){
  for(auto C: IDCons) delete C;
  for(auto C: STPCons) delete C;
  for(auto C: STFCons) delete C;
  for(auto C: PACons) delete C;
}
char PointerAnalysis::ID = 0;
static RegisterPass<PointerAnalysis> X("pointer-analysis", "Pointer Analysis Pass", false, false);
