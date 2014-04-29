#include "FatPointers.hpp"
#include "Helpers.hpp"
#include <string>
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

std::map<Type *, StructType *> FatPointers::FatPointerTypes;
std::map<Type *, Function *> FatPointers::BoundsChecks;

Value* FatPointers::CreateFatPointer(Type *PointerType, IRBuilder<> &B, std::string Name){
  StructType *FatPointerType = FatPointers::GetFatPointerType(PointerType);
  Value* FatPointer = B.CreateAlloca(FatPointerType, NULL, Name);
  return FatPointer;
}

ConstantPointerNull* FatPointers::GetFieldNull(Value *FatPointer){
  StructType *ST = cast<StructType>(FatPointer->getType()->getPointerElementType());
  ConstantPointerNull *Null = ConstantPointerNull::get(
      cast<PointerType>(ST->getElementType(0)));
  return Null;
}
StructType* FatPointers::GetFatPointerType(Type *PointerType){
  Type *OriginalPointerType = PointerType;
  if(FatPointers::FatPointerTypes.count(PointerType) == 1)
    return FatPointerTypes[PointerType];
  
  // Deal with nested pointers
  if(PointerType->getPointerElementType()->isPointerTy())
    PointerType = GetFatPointerType(
        PointerType->getPointerElementType())->getPointerTo();

  std::vector<Type *> FatPointerMembers;	
  FatPointerMembers.push_back(PointerType);
  FatPointerMembers.push_back(PointerType);
  FatPointerMembers.push_back(PointerType);

  std::string Name = "FatPointer";
  // Give the struct a nice name - this assumes that structs are named
  // "struct.StructureName"
  if(StructType *ST = dyn_cast<StructType>(PointerType->getPointerElementType())){
    if(ST->hasName())
      Name += ST->getName().str();
  }
  StructType *FatPointerType = StructType::create(FatPointerMembers, Name);

  FatPointers::FatPointerTypes[OriginalPointerType] = FatPointerType;
  return FatPointerType;
}

bool FatPointers::IsFatPointerType(Type *T){
  for(auto Pair: FatPointerTypes)
    if(Pair.second == T)
      return true;
  return false;
}

void FatPointers::CreateBoundsCheck(IRBuilder<> &B, Value *Val, Value *Base, Value *Bound, Function *Print, Module *M){
  if(BoundsChecks.count(Val->getType()) == 0)
    CreateBoundsCheckFunction(Val->getType(), Print, M);

  B.CreateCall3(BoundsChecks[Val->getType()], Val, Base, Bound);
}
void FatPointers::CreateBoundsCheckFunction(Type *PointerType, Function *Print, Module *M){
  std::vector<Type *> ParamTypes;
  ParamTypes.push_back(PointerType);
  ParamTypes.push_back(PointerType);
  ParamTypes.push_back(PointerType);
  FunctionType *FuncType = FunctionType::get(
      Type::getVoidTy(PointerType->getContext()), ParamTypes, false);

  Function *BoundsCheck = Function::Create(FuncType,
      GlobalValue::LinkageTypes::ExternalLinkage, "BoundsCheck", M);
    
  Function::arg_iterator Args = BoundsCheck->arg_begin();
  Value *Val = Args++;
  Val->setName("Value");
  Value *Base = Args++;
  Base->setName("Base");
  Value *Bound = Args++;
  Bound->setName("Bound");

  BasicBlock *UnsetCheck = BasicBlock::Create(PointerType->getContext(), 
      "UnsetCheck", BoundsCheck);
  IRBuilder<> B(UnsetCheck);

  /*
  if(Print){
    B.CreateCall2(Print, Str(B, "Base:  %p"), Base);
    B.CreateCall2(Print, Str(B, "Value: %p"), Val);
    B.CreateCall2(Print, Str(B, "Bound: %p"), Bound);
  }
  */

  Type *IntegerType = IntegerType::getInt64Ty(Val->getContext());

  // Create the check for (Base == NULL)
  Value *BaseAsInt = B.CreatePtrToInt(Base, IntegerType);
  Value *IsBaseNull = B.CreateICmpEQ(ConstantInt::get(IntegerType, 0), BaseAsInt);
  BasicBlock::iterator iter = BasicBlock::iterator(cast<Instruction>(IsBaseNull));
  iter++;

  BasicBlock *InvalidCheck = UnsetCheck->splitBasicBlock(iter, "InvalidCheck");
  removeTerminator(UnsetCheck);

  // Create the check for (Base == Bound)
  B.SetInsertPoint(InvalidCheck);
  Value *BoundAsInt = B.CreatePtrToInt(Bound, IntegerType);
  Value *IsBaseEqBound = B.CreateICmpEQ(BaseAsInt, BoundAsInt);
  iter = BasicBlock::iterator(cast<Instruction>(IsBaseEqBound));
  iter++;

  BasicBlock *InBoundsCheck = InvalidCheck->splitBasicBlock(iter, "InBoundsCheck");
  removeTerminator(InvalidCheck);
  
  // Create the check for (Base <= Value < Bound)
  B.SetInsertPoint(InBoundsCheck);
  Value *ValueAsInt = B.CreatePtrToInt(Val, IntegerType);
  Value *InLowerBound = B.CreateICmpUGE(ValueAsInt, BaseAsInt);
  Value *InHigherBound = B.CreateICmpULT(ValueAsInt, BoundAsInt);
  Instruction *IsInBounds = cast<Instruction>(B.CreateAnd(InLowerBound,InHigherBound));

  iter = BasicBlock::iterator(cast<Instruction>(IsInBounds));
  iter++;

  BasicBlock *AfterChecks = InBoundsCheck->splitBasicBlock(iter, "AfterChecks");
  removeTerminator(InBoundsCheck);

  // Now create the failure basic blocks
  LLVMContext *C = &IsInBounds->getContext();
  BasicBlock *Invalid = BasicBlock::Create(*C, "Invalid", BoundsCheck);
  B.SetInsertPoint(Invalid);
  if(Print)
    B.CreateCall(Print, Str(B, "Invalid"));
  B.CreateBr(AfterChecks);

  BasicBlock *Unset = BasicBlock::Create(*C, "Unset", BoundsCheck);
  B.SetInsertPoint(Unset);
  if(Print)
    B.CreateCall(Print, Str(B, "Unset"));
  B.CreateBr(AfterChecks);

  BasicBlock *OutOfBounds = BasicBlock::Create(*C, "OutOfBounds", BoundsCheck);
  B.SetInsertPoint(OutOfBounds);
  if(Print)
    B.CreateCall(Print, Str(B, "OutOfBounds"));
  B.CreateBr(AfterChecks);

  // Link the Check basic blocks to their failures
  B.SetInsertPoint(UnsetCheck);
  B.CreateCondBr(IsBaseNull, Unset,InvalidCheck);
  B.SetInsertPoint(InvalidCheck);
  B.CreateCondBr(IsBaseEqBound, Invalid, InBoundsCheck);
  B.SetInsertPoint(InBoundsCheck);
  B.CreateCondBr(IsInBounds, AfterChecks, OutOfBounds);

  B.SetInsertPoint(AfterChecks);
  B.CreateRetVoid();

  BoundsChecks[PointerType] = BoundsCheck;
}
