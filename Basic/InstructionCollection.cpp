#include "InstructionCollection.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

template <typename T> void RemoveAFromB(std::set<T> &A, std::set<T> &B){
  std::set<T> I;
  std::set_difference(B.begin(), B.end(), A.begin(), A.end(),
      std::inserter(I, I.end()));
  B = I;
}

InstructionCollection::InstructionCollection(FunctionDuplicater *FD, TypeDuplicater *TD){
  this->FD = FD;
  this->TD = TD;
  CollectInstructions(FD->GetFPFunctions());
}

void InstructionCollection::CollectInstructions(std::set<Function *> Functions){
  for(auto F: Functions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;

      CheckForPointerAlloca(I);
      CheckForPointerStore(I);
      CheckForPointerLoad(I);

      CheckForArrayAlloca(I);

      CheckForStructGep(I);

      CheckForFunctionCall(I);
      if(F->getName() != "main")
        CheckForReturn(I);
    }
  }

  AddArrayGeps();
  AddPointerEquals();

  RemoveAFromB(PointerLoadsForParameters, PointerLoads);
  RemoveAFromB(PointerLoadsForReturn, PointerLoads);
  RemoveAFromB(PointerStoresFromReturn, PointerStores);

  RemoveAFromB(PointerLoadsForPointerEquals, PointerLoads);
  RemoveAFromB(PointerStoresFromPointerEquals, PointerStores);
}

void InstructionCollection::CheckForArrayAlloca(Instruction *I){
  if(AllocaInst *A = dyn_cast<AllocaInst>(I)){
    if(!A->getType()->isPointerTy())
      return;

    if(!A->getType()->getPointerElementType()->isArrayTy())
      return;

    ArrayAllocas.insert(A);
  }
}
void InstructionCollection::CheckForPointerStore(Instruction *I){
  if(StoreInst *S = dyn_cast<StoreInst>(I)){
    if(!S->getPointerOperand()->getType()->getPointerElementType()->isPointerTy())
      return;

    PointerStores.insert(S);
  }
}
void InstructionCollection::CheckForPointerLoad(Instruction *I){
  if(LoadInst *L = dyn_cast<LoadInst>(I)){
    if(!L->getPointerOperand()->getType()->getPointerElementType()->isPointerTy())
    //if(!L->getPointerOperand()->getType()->isPointerTy())
      return;

    errs() << *L << "\n";
    PointerLoads.insert(L);
  }
}
void InstructionCollection::CheckForStructGep(Instruction *I){
  if(auto G = dyn_cast<GetElementPtrInst>(I)){
    Type *T = G->getPointerOperand()->getType()->getPointerElementType();
    if(auto *ST = dyn_cast<StructType>(T))
      if(TD->GetFPTypes().count(ST))
        GepsToRecreate.insert(G);
  }
}

void InstructionCollection::AddPointerEquals(){
  for(auto L: PointerLoads){
    if(auto C = dyn_cast<StoreInst>(L->use_back())){
      if(L->getType() != C->getValueOperand()->getType())
        continue;
      if(PointerStores.count(C)){
        PointerStoresFromPointerEquals.insert(C);
        PointerLoadsForPointerEquals.insert(L);
      }
    }
  }
}

void InstructionCollection::CheckForFunctionCall(Instruction *I){
  if(CallInst *C = dyn_cast<CallInst>(I)){
    // Check if this is a transformable function
    if(!FD->GetRawFunctions().count(C->getCalledFunction())){
      ExternalCalls.insert(C);
      return;
    }

    Calls.insert(C);

    for(int i=0; i<C->getNumArgOperands(); i++){
      if(LoadInst *Inst = dyn_cast<LoadInst>(C->getArgOperand(i))){
        PointerLoadsForParameters.insert(Inst);
      }
    }

    // Remember the next use of the return value
    if(C->getNumUses() == 1){
      if(StoreInst *Store = dyn_cast<StoreInst>(C->use_back()))
        PointerStoresFromReturn.insert(Store);
    }
    assert(C->getNumUses() < 2);
  }
}
void InstructionCollection::CheckForReturn(Instruction *I){
  if(ReturnInst *R = dyn_cast<ReturnInst>(I)){
    if(R->getReturnValue() == NULL)
      return;
    
    Returns.insert(R);

    if(LoadInst *L = dyn_cast<LoadInst>(R->getReturnValue())){
      PointerLoadsForReturn.insert(L);
    }
  }
}

void InstructionCollection::CheckForPointerAlloca(Instruction *I){
  if(AllocaInst *A = dyn_cast<AllocaInst>(I)){
    if(!I->getType()->getPointerElementType()->isPointerTy())
      return;

    PointerAllocas.insert(A);
  }
}
void InstructionCollection::AddArrayGeps(){
  for(auto Arr: ArrayAllocas){
    for(Value::use_iterator i = Arr->use_begin(), e = Arr->use_end(); i!=e; ++i){
      if(auto Gep = dyn_cast<GetElementPtrInst>(*i)){
        ArrayGeps.insert(Gep);
      }
    }
  }
}
