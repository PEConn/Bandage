#include "BoundsChecks.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

#include "../Basic/Helpers.hpp"

BoundsChecks::BoundsChecks(LocalBounds *LB, FunctionDuplicater *FD){
  this->LB = LB;
  this->FD = FD;
  this->BoundsCheck = NULL;
}

void BoundsChecks::CreateBoundsChecks(){
  if(this->BoundsCheck == NULL){
    errs() << "Cannot create bounds checks with null print function\n";
    return;
  }
  for(auto F: FD->FPFunctions){
    for(auto II = inst_begin(F), EI = inst_end(F); II != EI; ++II){
      Instruction *I = &*II;
      if(auto L = dyn_cast<LoadInst>(I)){
        if(L->getPointerOperand()->getType()->getPointerElementType()->isPointerTy())
          CreateBoundsCheck(L);
      }
    }
  }
}

void BoundsChecks::CreateBoundsCheck(LoadInst *L){
  IRBuilder<> B(L);
  Type *PtrTy = Type::getInt8PtrTy(L->getContext());
  B.CreateCall3(BoundsCheck, 
      B.CreatePointerCast(B.CreateLoad(L->getPointerOperand()), PtrTy), 
      B.CreatePointerCast(B.CreateLoad(LB->GetLowerBound(L)), PtrTy),
      B.CreatePointerCast(B.CreateLoad(LB->GetUpperBound(L)), PtrTy));
  //errs() << "TODO: Create bounds check for " << *L << "\n";
}

void BoundsChecks::CreateBoundsCheckFunction(Module &M, Function *Print){
  // Get i8* type
  Type *PtrTy = Type::getInt8PtrTy(M.getContext());
  Type *VoidTy = Type::getVoidTy(M.getContext());

  std::vector<Type *> ParamTypes;
  ParamTypes.push_back(PtrTy);
  ParamTypes.push_back(PtrTy);
  ParamTypes.push_back(PtrTy);

  FunctionType *FuncType = FunctionType::get(VoidTy, ParamTypes, false);

  Function *BoundsCheckFunc = Function::Create(FuncType,
      GlobalValue::LinkageTypes::ExternalLinkage, "BoundsCheck", &M);

  // BoundsCheckFunc->addFnAttr(Attribute::AlwaysInline);
    
  Function::arg_iterator Args = BoundsCheckFunc->arg_begin();
  Value *Val = Args++;
  Val->setName("Value");
  Value *Base = Args++;
  Base->setName("Base");
  Value *Bound = Args++;
  Bound->setName("Bound");

  Type *IntegerType = IntegerType::getInt64Ty(M.getContext());

  BasicBlock *NullCheckBB = BasicBlock::Create(M.getContext(), 
      "NullCheck", BoundsCheckFunc);
  IRBuilder<> B(NullCheckBB);

  if(false && Print){
    B.CreateCall2(Print, Str(B, "Base:  %p"), Base);
    B.CreateCall2(Print, Str(B, "Value: %p"), Val);
    B.CreateCall2(Print, Str(B, "Bound: %p"), Bound);
  }

  // Create NULL check (does Value == NULL)
  Value *ValueAsInt = B.CreatePtrToInt(Val, IntegerType);
  Value *IsValueNull = B.CreateICmpEQ(ConstantInt::get(IntegerType, 0), ValueAsInt);
  BasicBlock::iterator iter = BasicBlock::iterator(cast<Instruction>(IsValueNull));
  iter++;

  BasicBlock *NoBoundsCheckBB = NullCheckBB->splitBasicBlock(iter, "NoBoundsCheck");
  removeTerminator(NullCheckBB);

  // Create NoBounds check (does Base == NULL)
  B.SetInsertPoint(NoBoundsCheckBB);
  Value *BaseAsInt = B.CreatePtrToInt(Base, IntegerType);
  Value *IsBaseNull = B.CreateICmpEQ(ConstantInt::get(IntegerType, 0), BaseAsInt);
  iter = BasicBlock::iterator(cast<Instruction>(IsBaseNull));
  iter++;
  
  BasicBlock *BoundsCheckBB = NoBoundsCheckBB->splitBasicBlock(iter, "BoundsCheck");
  removeTerminator(NoBoundsCheckBB);

  // Create BoundsCheck (Base <= Value < Bound)
  B.SetInsertPoint(BoundsCheckBB);
  Value *BoundAsInt = B.CreatePtrToInt(Bound, IntegerType);
  Value *InLowerBound = B.CreateICmpUGE(ValueAsInt, BaseAsInt);
  Value *InHigherBound = B.CreateICmpULT(ValueAsInt, BoundAsInt);
  Instruction *IsInBounds = cast<Instruction>(B.CreateAnd(InLowerBound,InHigherBound));
  iter = BasicBlock::iterator(cast<Instruction>(IsInBounds));
  iter++;

  BasicBlock *AfterChecksBB = BoundsCheckBB->splitBasicBlock(iter, "AfterChecks");
  removeTerminator(BoundsCheckBB);

  // Now create the failure basic blocks
  LLVMContext *C = &M.getContext();

  BasicBlock *NullBB = BasicBlock::Create(*C, "Null", BoundsCheckFunc);
  B.SetInsertPoint(NullBB);
  if(Print)
    B.CreateCall(Print, Str(B, "Null"));
  B.CreateBr(AfterChecksBB);

  BasicBlock *NoBoundsBB= BasicBlock::Create(*C, "NoBounds", BoundsCheckFunc);
  B.SetInsertPoint(NoBoundsBB);
  if(Print)
    B.CreateCall(Print, Str(B, "NoBounds"));
  B.CreateBr(AfterChecksBB);

  BasicBlock *OutOfBoundsBB = BasicBlock::Create(*C, "OutOfBounds", BoundsCheckFunc);
  B.SetInsertPoint(OutOfBoundsBB);
  if(Print)
    B.CreateCall(Print, Str(B, "OutOfBounds"));
  B.CreateBr(AfterChecksBB);

  // Link the Check basic blocks to their failures
  B.SetInsertPoint(NullCheckBB);
  B.CreateCondBr(IsValueNull, NullBB, NoBoundsCheckBB);
  B.SetInsertPoint(NoBoundsCheckBB);
  B.CreateCondBr(IsBaseNull, NoBoundsBB, BoundsCheckBB);
  B.SetInsertPoint(BoundsCheckBB);
  B.CreateCondBr(IsInBounds, AfterChecksBB, OutOfBoundsBB);

  B.SetInsertPoint(AfterChecksBB);
  B.CreateRetVoid();

  BoundsCheck = BoundsCheckFunc;
}
