#include "FatPointers.hpp"
#include "Helpers.hpp"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

std::map<Type *, Type *> FatPointers::FatPointerTypes;
std::map<Type *, Function *> FatPointers::BoundsChecks;

Type* FatPointers::GetFatPointerType(Type *PointerType){
  if(FatPointers::FatPointerTypes.count(PointerType) == 1)
    return FatPointerTypes[PointerType];
  
  std::vector<Type *> FatPointerMembers;	
  FatPointerMembers.push_back(PointerType);
  FatPointerMembers.push_back(PointerType);
  FatPointerMembers.push_back(PointerType);
  Type *FatPointerType = StructType::create(FatPointerMembers, "FatPointer");

  FatPointers::FatPointerTypes[PointerType] = FatPointerType;
  return FatPointerType;
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

  BasicBlock *Main = BasicBlock::Create(PointerType->getContext(), 
      "main", BoundsCheck);
  IRBuilder<> B(Main);

  /*B.CreateCall2(Print, Str(B, "Base:  %p"), Base);
  B.CreateCall2(Print, Str(B, "Value: %p"), Val);
  B.CreateCall2(Print, Str(B, "Bound: %p"), Bound);*/

  Type *IntegerType = IntegerType::getInt32Ty(Val->getContext());
  Value *InLowerBound = B.CreateICmpUGE(
      B.CreatePtrToInt(Val, IntegerType),
      B.CreatePtrToInt(Base, IntegerType) 
      );
  Value *InHigherBound = B.CreateICmpULT(
      B.CreatePtrToInt(Val, IntegerType),
      B.CreatePtrToInt(Bound, IntegerType)
      );
      
  Instruction *InBounds = cast<Instruction>(B.CreateAnd(InLowerBound, InHigherBound));

  BasicBlock::iterator iter = BasicBlock::iterator(InBounds);
  iter++;

  BasicBlock *BeforeBB = InBounds->getParent();
  BasicBlock *PassedBB = BeforeBB->splitBasicBlock(iter);

  removeTerminator(BeforeBB);
  BasicBlock *FailedBB = BasicBlock::Create(InBounds->getContext(),
      "BoundsCheckFailed", BeforeBB->getParent());

  B.SetInsertPoint(BeforeBB);
  B.CreateCondBr(InBounds, PassedBB, FailedBB);

  B.SetInsertPoint(FailedBB);
  B.CreateCall(Print, Str(B, "OutOfBounds"));
  B.CreateBr(PassedBB);

  B.SetInsertPoint(PassedBB);
  B.CreateRetVoid();

  BoundsChecks[PointerType] = BoundsCheck;
}
