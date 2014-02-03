#include <vector>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

#include "FatPointer.h"

FatPointer::FatPointer(IRBuilder<> *B, LLVMContext &C){
  this->B = B;
  PointerTy = Type::getInt32PtrTy(C);
  IntegerTy = IntegerType::getInt32Ty(C);

  std::vector<Type *> Elements;
  Elements.push_back(PointerTy);  // Value
  Elements.push_back(PointerTy);  // Base
  Elements.push_back(IntegerTy);  // Offset
  FatPointerTy = StructType::create(Elements, "FatPointer");

  ValueIdx.push_back(ConstantInt::get(IntegerTy, 0));
  ValueIdx.push_back(ConstantInt::get(IntegerTy, 0));
  BaseIdx.push_back(ConstantInt::get(IntegerTy, 0));
  BaseIdx.push_back(ConstantInt::get(IntegerTy, 1));
  LengthIdx.push_back(ConstantInt::get(IntegerTy, 0));
  LengthIdx.push_back(ConstantInt::get(IntegerTy, 2));
}

Value *FatPointer::createFatPointer(){
  return B->CreateAlloca(FatPointerTy, NULL, "FatPointer");
}

Value *FatPointer::getFatPointerValue(Value *FP){
  Value *Ptr = B->CreateGEP(FP, ValueIdx, "FatPointerValue");
  return B->CreateLoad(Ptr, "LoadFatPointerValue");
}
Value *FatPointer::getFatPointerBase(Value *FP){
  Value *Ptr = B->CreateGEP(FP, BaseIdx, "FatPointerValue");
  return B->CreateLoad(Ptr, "LoadFatPointerBase");
}
Value *FatPointer::getFatPointerLength(Value *FP){
  Value *Ptr = B->CreateGEP(FP, LengthIdx, "FatPointerValue");
  return B->CreateLoad(Ptr, "LoadFatPointerLength");
}

Value *FatPointer::getConstPointer(int Val){
  Constant *ConstInt = ConstantInt::get(IntegerTy, Val);
  Value *ConstPtr = ConstantExpr::getIntToPtr(ConstInt, PointerTy);
  return ConstPtr;
}
Value *FatPointer::getConstInteger(int Val){
  Constant *ConstInt = ConstantInt::get(IntegerTy, Val);
  return ConstInt;
}
void   FatPointer::setFatPointerValue(Value *FP, Value *Val){
  Value *Ptr = B->CreateGEP(FP, ValueIdx, "FatPointerValue");
  Value *CastVal = B->CreatePointerCast(Val, PointerTy);

  B->CreateStore(CastVal, Ptr, "SetFatPointerValue");
}
void   FatPointer::setFatPointerBase(Value *FP, Value *Val){
  Value *Ptr = B->CreateGEP(FP, BaseIdx, "FatPointerBase");
  Value *CastVal = B->CreatePointerCast(Val, PointerTy);

  B->CreateStore(CastVal, Ptr, "SetFatPointerValue");
}
void   FatPointer::setFatPointerLength(Value *FP, Value *Val){
  Value *Ptr = B->CreateGEP(FP, LengthIdx, "FatPointerLength");
  B->CreateStore(Val, Ptr, "SetFatPointerLength");
}
