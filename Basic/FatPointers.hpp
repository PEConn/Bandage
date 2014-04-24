#ifndef FAT_POINTERS
#define FAT_POINTERS

#include <vector>
#include <map>
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

class FatPointers{
public:
  static Type* GetFatPointerType(Type *PointerType);
  static bool IsFatPointerType(Type *T);
  // The 'Print' function will eventually be changed to the function to call on
  // OutOfBounds
  static void CreateBoundsCheck(IRBuilder<> &B,
      Value *Val, Value *Base, Value *Bound, Function *Print, Module *M);
private:
  static std::map<Type *, Type *> FatPointerTypes;
  static std::map<Type *, Function *> BoundsChecks;
  static void CreateBoundsCheckFunction(Type *PointerType, Function *Print, Module *M);
};

#endif
