#ifndef FAT_POINTERS
#define FAT_POINTERS

#include <vector>
#include <map>
#include <string>
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

class FatPointers{
public:
  static bool Inline;
  static Value* CreateFatPointer(Type *PointerType, IRBuilder<> &B, std::string Name="");
  static ConstantPointerNull* GetFieldNull(Value *FatPointer);
  static StructType* GetFatPointerType(Type *PointerType);
  static bool IsFatPointerType(Type *T);
  // The 'Print' function will eventually be changed to the function to call on
  // OutOfBounds
  static void CreateBoundsCheck(IRBuilder<> &B,
      Value *Val, Value *Base, Value *Bound, Function *Print, Module *M);
  static void CreateNullCheck(IRBuilder<> &B, Value *V, Function *Print, Module *M);
  static void CreateBoundsCheckFunction(Type *PointerType, Function *Print, Module *M);
  static void CreateNullCheckFunction(Type *PointerType, Function *Print, Module *M);
private:
  static std::map<Type *, StructType *> FatPointerTypes;
  static std::map<Type *, Function *> BoundsChecks;
  static std::map<Type *, Function *> NullChecks;
};

#endif
