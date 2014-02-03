#ifndef FAT_POINTER_H
#define FAT_POINTER_H

using namespace llvm;

struct FatPointer{
  FatPointer(IRBuilder<> *B, LLVMContext &C);

  Value *createFatPointer();
  
  Value *getFatPointerValue(Value *FP);
  Value *getFatPointerBase(Value *FP);
  Value *getFatPointerLength(Value *FP);

  void setFatPointerValue(Value *FP, Value *Val);
  void setFatPointerBase(Value *FP, Value *Val);
  void setFatPointerLength(Value *FP, Value *Val);

  Value *getConstPointer(int Val);
  Value *getConstInteger(int Val);
private:
  IRBuilder<> *B;

  Type* PointerTy;
  Type* IntegerTy;
  Type* FatPointerTy;

  std::vector<Value *> ValueIdx;
  std::vector<Value *> BaseIdx;
  std::vector<Value *> LengthIdx;
};

#endif
