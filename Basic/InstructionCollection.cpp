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

InstructionCollection::InstructionCollection(std::set<Function *> Functions, std::set<Function *> RawFunctions){
  this->RawFunctions = RawFunctions;
  CollectInstructions(Functions);
}

void InstructionCollection::CollectInstructions(std::set<Function *> Functions){
  for(auto F: Functions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;

      CheckForPointerAlloca(I);
      CheckForPointerStore(I);
      CheckForPointerLoad(I);

      CheckForArrayAlloca(I);

      CheckForFunctionCall(I);
    }
  }

  AddArrayGeps();
  RemoveParametersFromPointerLoads();
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
      return;

    PointerLoads.insert(L);
  }
}
void InstructionCollection::CheckForFunctionCall(Instruction *I){
  if(CallInst *C = dyn_cast<CallInst>(I)){
    // Check if this is a transformable function
    if(!RawFunctions.count(C->getCalledFunction()))
      return;

    Calls.insert(C);

    for(int i=0; i<C->getNumArgOperands(); i++){
      if(LoadInst *Inst = dyn_cast<LoadInst>(C->getArgOperand(i))){
        PointerParameterLoads.insert(Inst);
      }
    }

    // Something to do with the function return value here?
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
      if(auto Gep = dyn_cast<GetElementPtrInst>(*i))
        ArrayGeps.insert(Gep);
    }
  }
}
void InstructionCollection::RemoveParametersFromPointerLoads(){
  std::set<LoadInst *> I;
  std::set_difference(PointerLoads.begin(), PointerLoads.end(),
      PointerParameterLoads.begin(), PointerParameterLoads.end(),
      std::inserter(I, I.end()));

  PointerLoads = I;
}
