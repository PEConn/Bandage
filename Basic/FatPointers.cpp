#include "FatPointers.hpp"
#include "Helpers.hpp"
#include <string>
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

std::map<Type *, StructType *> FatPointers::FatPointerTypes;
std::map<Type *, Function *> FatPointers::BoundsChecks;
std::map<Type *, Function *> FatPointers::NullChecks;
bool FatPointers::Inline = true;
bool FatPointers::Declare = false;

Value* FatPointers::CreateFatPointer(Type *PointerType, IRBuilder<> &B, std::string Name){
  StructType *FatPointerType = FatPointers::GetFatPointerType(PointerType);
  Value* FatPointer = B.CreateAlloca(FatPointerType, NULL, Name);
  return FatPointer;
}

ConstantPointerNull* FatPointers::GetFieldNull(Value *FatPointer){
  //if(auto G = dyn_cast<GetElementPtrInst>(FatPointer))
  //  errs() << *G->getPointerOperand() << "\n";
  // errs() << *FatPointer << "\n";
  // errs() << *FatPointer->getType() << "\n";
  StructType *ST = cast<StructType>(FatPointer->getType()->getPointerElementType());
  ConstantPointerNull *Null = ConstantPointerNull::get(
      cast<PointerType>(ST->getElementType(0)));
  return Null;
}
StructType* FatPointers::GetFatPointerType(Type *PointerType){
  //errs() << "Looking for " << PointerType << " " << *PointerType << "\n";
  //for(auto T: FatPointerTypes)
  //  errs() << "|    " << T.first << " " << *T.first << "\n";
  Type *OriginalPointerType = PointerType;
  if(FatPointers::FatPointerTypes.count(PointerType) == 1)
    return FatPointerTypes[PointerType];
  
  //errs() << *PointerType << " ->\n\t";

  // Deal with nested pointers
  if(PointerType->getPointerElementType()->isPointerTy())
    PointerType = GetFatPointerType(
        PointerType->getPointerElementType())->getPointerTo();

  if(FatPointers::FatPointerTypes.count(PointerType))
    return FatPointerTypes[PointerType];



  std::vector<Type *> FatPointerMembers;	
  FatPointerMembers.push_back(PointerType);
  FatPointerMembers.push_back(PointerType);
  FatPointerMembers.push_back(PointerType);

  std::string Name = "FatPointer.";
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
  Type *VoidPtr = Type::getInt8PtrTy(M->getContext());
  Val = B.CreatePointerCast(Val, VoidPtr);
  Base = B.CreatePointerCast(Base, VoidPtr);
  Bound = B.CreatePointerCast(Bound, VoidPtr);
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

  Function *BoundsCheckFunc = Function::Create(FuncType,
      GlobalValue::LinkageTypes::ExternalLinkage, "BoundsCheck", M);
  if(Inline)
    BoundsCheckFunc->addFnAttr(Attribute::AlwaysInline);

  if(Declare){

    Function::arg_iterator Args = BoundsCheckFunc->arg_begin();
    Value *Val = Args++;
    Val->setName("Value");
    Value *Base = Args++;
    Base->setName("Base");
    Value *Bound = Args++;
    Bound->setName("Bound");


    Type *IntegerType = IntegerType::getInt64Ty(Val->getContext());

    BasicBlock *NullCheckBB = BasicBlock::Create(PointerType->getContext(), 
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
    LLVMContext *C = &IsInBounds->getContext();

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
  }

  BoundsChecks[PointerType] = BoundsCheckFunc;
}

void FatPointers::CreateNullCheck(IRBuilder<> &B, Value *V, Function *Print, Module *M){
  if(NullChecks.count(V->getType()) == 0)
    CreateNullCheckFunction(V->getType(), Print, M);
  B.CreateCall(NullChecks[V->getType()], V);
}

void FatPointers::CreateNullCheckFunction(Type *PointerType, Function *Print, Module *M){
  std::vector<Type *> ParamTypes;
  ParamTypes.push_back(PointerType);
  FunctionType *FuncType = FunctionType::get(
      Type::getVoidTy(PointerType->getContext()), ParamTypes, false);

  Function *BoundsCheckFunc = Function::Create(FuncType,
      GlobalValue::LinkageTypes::ExternalLinkage, "NullCheck", M);

  if(Inline)
    BoundsCheckFunc->addFnAttr(Attribute::AlwaysInline);
    
  Function::arg_iterator Args = BoundsCheckFunc->arg_begin();
  Value *Val = Args++;
  Val->setName("Value");

  Type *IntegerType = IntegerType::getInt64Ty(Val->getContext());

  BasicBlock *NullCheckBB = BasicBlock::Create(PointerType->getContext(), 
      "NullCheck", BoundsCheckFunc);
  IRBuilder<> B(NullCheckBB);

  // Create NULL check (does Value == NULL)
  Value *ValueAsInt = B.CreatePtrToInt(Val, IntegerType);
  Value *IsValueNull = B.CreateICmpEQ(ConstantInt::get(IntegerType, 0), ValueAsInt);
  BasicBlock::iterator iter = BasicBlock::iterator(cast<Instruction>(IsValueNull));
  iter++;

  BasicBlock *AfterChecksBB = NullCheckBB->splitBasicBlock(iter, "AfterCheck");
  removeTerminator(NullCheckBB);

  // Now create the failure basic block
  LLVMContext *C = &IsValueNull->getContext();
  BasicBlock *NullBB = BasicBlock::Create(*C, "Null", BoundsCheckFunc);
  B.SetInsertPoint(NullBB);
  if(Print)
    B.CreateCall(Print, Str(B, "Null"));
  B.CreateBr(AfterChecksBB);

  // Link the Check basic blocks to their failures
  B.SetInsertPoint(NullCheckBB);
  B.CreateCondBr(IsValueNull, NullBB, AfterChecksBB);

  B.SetInsertPoint(AfterChecksBB);
  B.CreateRetVoid();

  NullChecks[PointerType] = BoundsCheckFunc;
}
