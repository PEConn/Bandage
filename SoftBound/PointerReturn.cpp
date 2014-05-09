#include "PointerReturn.hpp"
#include "llvm/Support/raw_ostream.h"

std::map<Type *, Type *> PointerReturn::PointerReturns;

std::vector<Value *> GetIndices(int val, LLVMContext& C){
  std::vector<Value *> Idxs;
  Idxs.push_back(ConstantInt::get(IntegerType::getInt32Ty(C), 0));
  Idxs.push_back(ConstantInt::get(IntegerType::getInt32Ty(C), val));
  return Idxs;
}

Type *PointerReturn::GetPointerReturnType(Type *T){
  errs() << "Creating Pointer Return for: " << *T << "\n";
  if(!PointerReturns.count(T)){
    // Create the type
    std::vector<Type *> Members;
    Members.push_back(T);
    Members.push_back(T);
    Members.push_back(T);
    PointerReturns[T] = StructType::create(Members, "PointerReturn");
  }
  return PointerReturns[T];
}

void PointerReturn::WrapInPointerReturn(Value *Val, Value *Base, Value *Bound, Value *FP, IRBuilder<> &B){
  errs() << "Wrap:\n" << "\n";
  errs() << "\t" << *FP << "\n";
  errs() << "\t" << *FP->getType() << "\n";
  errs() << "\t" << *Val << "\n";
  Value *FPValue = B.CreateGEP(FP, GetIndices(0, FP->getContext()));
  Value *FPBase = B.CreateGEP(FP, GetIndices(1, FP->getContext()));
  Value *FPBound = B.CreateGEP(FP, GetIndices(2, FP->getContext()));
  //B.CreateStore(B.CreateLoad(Val), FPValue);
  B.CreateStore(Val, FPValue);
  B.CreateStore(B.CreateLoad(Base), FPBase);
  B.CreateStore(B.CreateLoad(Bound), FPBound);
}

void PointerReturn::ExtractFromPointerReturn(Value *Val, Value *Base, Value *Bound, Value *FP, IRBuilder<> &B){
  errs() << "Extract:\n" << "\n";
  errs() << "\t" << *FP << "\n";
  errs() << "\t" << *FP->getType() << "\n";
  Value *FPValue = B.CreateGEP(FP, GetIndices(0, FP->getContext()));
  Value *FPBase = B.CreateGEP(FP, GetIndices(1, FP->getContext()));
  Value *FPBound = B.CreateGEP(FP, GetIndices(2, FP->getContext()));
  B.CreateStore(B.CreateLoad(FPValue), Val);
  B.CreateStore(B.CreateLoad(FPBase), Base);
  B.CreateStore(B.CreateLoad(FPBound), Bound);
}
