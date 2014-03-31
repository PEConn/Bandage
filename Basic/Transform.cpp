#include <set>
#include <map>
#include <algorithm>

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

#include "Transform.hpp"
#include "Helpers.hpp"

Transform::Transform(InstructionCollection *Instructions, std::map<Function *, Function *> Map, Module &M){
  this->Instructions = Instructions;
  this->RawToFPFunctionMap = Map;
  this->M = &M;
  this->DL = new DataLayout(&M);
  this->Print = M.getFunction("printf");
}

void Transform::Apply(){
  TransformPointerAllocas();
  TransformPointerStores();
  TransformPointerLoads();
  TransformArrayAllocas();
  TransformArrayGeps();

  TransformFunctionCalls();
  TransformReturns();
}

void Transform::TransformPointerAllocas(){
  for(auto PointerAlloc : Instructions->PointerAllocas){
    BasicBlock::iterator iter = PointerAlloc;
    iter++;
    IRBuilder<> B(iter);

    Type *PointerType = PointerAlloc->getType()->getPointerElementType();

    // Construct the type for the fat pointer
    Type *FatPointerType = FatPointers::GetFatPointerType(PointerType);

    Value* FatPointer = B.CreateAlloca(FatPointerType, NULL, "fp" + PointerAlloc->getName());
    PointerAlloc->replaceAllUsesWith(FatPointer);
    // All code below can use the original PointerAlloc

    // Initialize Value to current value and everything else to zero
    std::vector<Value *> ValueIdx = GetIndices(0, PointerAlloc->getContext());
    std::vector<Value *> BaseIdx = GetIndices(1, PointerAlloc->getContext());
    std::vector<Value *> LengthIdx = GetIndices(2, PointerAlloc->getContext());
    Value *FatPointerValue = B.CreateGEP(FatPointer, ValueIdx); 
    Value *FatPointerBase = B.CreateGEP(FatPointer, BaseIdx); 
    Value *FatPointerBound = B.CreateGEP(FatPointer, LengthIdx); 

    Value *Address = B.CreateLoad(PointerAlloc);
    B.CreateStore(Address, FatPointerValue);
    B.CreateStore(Address, FatPointerBase);
    B.CreateStore(Address, FatPointerBound);
  }
}
void Transform::TransformPointerStores(){
  for(auto PointerStore : Instructions->PointerStores){
    IRBuilder<> B(PointerStore);

    Value* FatPointer = PointerStore->getPointerOperand(); 
    Value *Address = PointerStore->getValueOperand();

    // The types may match up - for example in the case of fat pointer parameters
    if(Address->getType() == FatPointer->getType()->getPointerElementType())
      continue;

    Value* RawPointer = 
        B.CreateGEP(FatPointer, GetIndices(0, PointerStore->getContext()));
    Instruction *NewStore = B.CreateStore(Address, RawPointer);

    B.SetInsertPoint(NewStore);

    SetBoundsForMalloc(B, PointerStore);
    SetBoundsForConstString(B, PointerStore);
    
    PointerStore->eraseFromParent();
  }
}
void Transform::TransformPointerLoads(){
  for(auto PointerLoad : Instructions->PointerLoads){
    IRBuilder<> B(PointerLoad);

    Value* FatPointer = PointerLoad->getPointerOperand(); 
    Value* RawPointer = 
        B.CreateGEP(FatPointer, GetIndices(0, PointerLoad->getContext()));
    Value* Base = B.CreateLoad(
        B.CreateGEP(FatPointer, GetIndices(1, PointerLoad->getContext())));
    Value* Bound = B.CreateLoad(
        B.CreateGEP(FatPointer, GetIndices(2, PointerLoad->getContext())));

    Value* NewLoad = B.CreateLoad(RawPointer);

    // If the load is going to by used in a Gep, use the address from the
    // Gep in the bounds check
    if(auto Gep = dyn_cast<GetElementPtrInst>(PointerLoad->use_back())){
      BasicBlock::iterator iter = Gep;
      iter++;

      IRBuilder<> B(iter);
      FatPointers::CreateBoundsCheck(B, Gep, Base, Bound, Print, M);
    } else {
      FatPointers::CreateBoundsCheck(B, B.CreateLoad(RawPointer), Base, Bound, Print, M);
    }
    PointerLoad->replaceAllUsesWith(NewLoad);
    PointerLoad->eraseFromParent();
  }

  // This is nessecary to get future typing correct
  for(auto PointerLoad : Instructions->PointerLoadsForParameters){
    IRBuilder<> B(PointerLoad);
    Value* NewLoad = B.CreateLoad(PointerLoad->getPointerOperand());
    PointerLoad->replaceAllUsesWith(NewLoad);
    PointerLoad->eraseFromParent();
  }
  for(auto PointerLoad : Instructions->PointerLoadsForReturn){
    IRBuilder<> B(PointerLoad);
    Value* NewLoad = B.CreateLoad(PointerLoad->getPointerOperand());
    PointerLoad->replaceAllUsesWith(NewLoad);
    PointerLoad->eraseFromParent();
  }
  for(auto PointerLoad : Instructions->PointerLoadsForPointerEquals){
    IRBuilder<> B(PointerLoad);
    Value* NewLoad = B.CreateLoad(PointerLoad->getPointerOperand());
    PointerLoad->replaceAllUsesWith(NewLoad);
    PointerLoad->eraseFromParent();
  }
}
void Transform::TransformReturns(){
  // This will only be called on returns that return a value

  for(auto Return: Instructions->Returns){
    IRBuilder<> B(Return);
    Value *NewReturn = B.CreateRet(Return->getReturnValue());
    Return->eraseFromParent();
  }
}
void Transform::TransformArrayAllocas(){
  for(auto ArrayAlloc : Instructions->ArrayAllocas){
    BasicBlock::iterator iter = ArrayAlloc;
    iter++;
    IRBuilder<> B(iter);

    Type *ArrayType = ArrayAlloc->getType();
    Type *IntegerType = IntegerType::getInt32Ty(ArrayAlloc->getContext());

    // Construct the type for the fat pointer
    Type *FatPointerType = FatPointers::GetFatPointerType(ArrayType);
    Value *FatPointer = B.CreateAlloca(FatPointerType, NULL, "FatPointer");
    ArrayAlloc->replaceAllUsesWith(FatPointer);
    // All code below can use the original ArrayAlloc

    // Initialize the values
    std::vector<Value *> ValueIdx = GetIndices(0, ArrayAlloc->getContext());
    std::vector<Value *> BaseIdx = GetIndices(1, ArrayAlloc->getContext());
    std::vector<Value *> LengthIdx = GetIndices(2, ArrayAlloc->getContext());
    Value *FatPointerValue = B.CreateGEP(FatPointer, ValueIdx);
    Value *FatPointerBase = B.CreateGEP(FatPointer, BaseIdx);
    Value *FatPointerBound = B.CreateGEP(FatPointer, LengthIdx);

    Value *Address = ArrayAlloc;
    B.CreateStore(Address, FatPointerValue);
    B.CreateStore(Address, FatPointerBase);

    Constant *ArrayLength = ConstantInt::get(IntegerType,
        GetNumElementsInArray(ArrayAlloc) * GetArrayElementSizeInBits(ArrayAlloc, DL)/8);

    Value *Bound = B.CreateAdd(ArrayLength, B.CreatePtrToInt(Address, IntegerType));

    B.CreateStore(B.CreateIntToPtr(Bound, Address->getType()), FatPointerBound);
  }

}
void Transform::TransformArrayGeps(){
  for(auto Gep : Instructions->ArrayGeps){
    IRBuilder<> B(Gep);

    Value* FatPointer = Gep->getPointerOperand(); 
    Value* RawPointer = B.CreateLoad(
        B.CreateGEP(FatPointer, GetIndices(0, Gep->getContext())));

    std::vector<Value *> IdxList;
    for(auto Idx = Gep->idx_begin(), EIdx = Gep->idx_end(); Idx != EIdx; ++Idx)
      IdxList.push_back(*Idx); 

    Instruction *NewGep = GetElementPtrInst::Create(RawPointer, IdxList);

    BasicBlock::iterator iter(Gep);
    ReplaceInstWithInst(Gep->getParent()->getInstList(), iter, NewGep);

    // Now add checking after the Gep Instruction
    iter = BasicBlock::iterator(NewGep);
    iter++;
    B.SetInsertPoint(iter);

    Value *Base = B.CreateLoad(B.CreateInBoundsGEP(FatPointer, 
          GetIndices(1, Gep->getContext())));
    Value *Bound = B.CreateLoad(B.CreateInBoundsGEP(FatPointer, 
          GetIndices(2, Gep->getContext())));

    FatPointers::CreateBoundsCheck(B, NewGep, Base, Bound, Print, M);
  }
}
void Transform::SetBoundsForConstString(IRBuilder<> &B, StoreInst *PointerStore){
  // In this case, the Address will be a GEP instruction
  Value *Address = PointerStore->getValueOperand();
  auto *Gep = dyn_cast<GEPOperator>(Address);
  if(!Gep)
    return;

  if(!Gep->getPointerOperand()->getType()->getPointerElementType()->isArrayTy())
    return;

  Type *IntegerType = IntegerType::getInt64Ty(PointerStore->getContext());
  Value* FatPointer = PointerStore->getPointerOperand(); 
  Value* FatPointerBase = 
      B.CreateGEP(FatPointer, GetIndices(1, PointerStore->getContext()));
  Value* FatPointerBound = 
      B.CreateGEP(FatPointer, GetIndices(2, PointerStore->getContext()));

  B.CreateStore(Address, FatPointerBase);

  int NumElementsInArray = Gep->getPointerOperand()->getType()->getPointerElementType()->getVectorNumElements();
  Constant *ArrayLength = ConstantInt::get(IntegerType, NumElementsInArray);
  Value *Bound = B.CreateAdd(ArrayLength, B.CreatePtrToInt(Address, IntegerType));

  B.CreateStore(B.CreateIntToPtr(Bound, Address->getType()), FatPointerBound);
}
void Transform::SetBoundsForMalloc(IRBuilder<> &B, StoreInst *PointerStore){
  Value *Prev = PointerStore->getValueOperand();

  // Follow through a cast if there is one
  if(auto BC = dyn_cast<BitCastInst>(Prev))
    Prev = BC->getOperand(0);

  auto Call = dyn_cast<CallInst>(Prev);
  if(!Call)
    return;

  if(Call->getCalledFunction()->getName() != "malloc")
    return;

  Value* FatPointer = PointerStore->getPointerOperand(); 
  Value *Address = PointerStore->getValueOperand();

  Value* FatPointerBase = 
      B.CreateGEP(FatPointer, GetIndices(1, PointerStore->getContext()));
  B.CreateStore(Address, FatPointerBase);

  Type *IntegerType = IntegerType::getInt64Ty(PointerStore->getContext());
  Value *Size = Call->getArgOperand(0);

  Value *Bound = B.CreateAdd(
      B.CreateTruncOrBitCast(Size, IntegerType),
      B.CreatePtrToInt(Address, IntegerType));

  Value* FatPointerBound = 
      B.CreateGEP(FatPointer, GetIndices(2, PointerStore->getContext()));

  B.CreateStore(B.CreateIntToPtr(Bound, Address->getType()), FatPointerBound);
}
void Transform::TransformFunctionCalls(){
  for(auto Call : Instructions->Calls){
    // Switch this function call to call the Fat Pointer version
    // -- Collect the parameters
    std::vector<Value *> Args;
    for(int i=0; i<Call->getNumArgOperands(); i++){
      Args.push_back(Call->getArgOperand(i));
    }

    // -- Create the new function call
    CallInst *NewCall = CallInst::Create(RawToFPFunctionMap[Call->getCalledFunction()], Args, "", Call);

    // -- Replace the old call with the new one
    Call->replaceAllUsesWith(NewCall);
    Call->eraseFromParent();
  }
}
