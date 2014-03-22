#ifndef FAT_POINTERS
#define FAT_POINTERS

#include <vector>
#include <map>
#include "llvm/IR/Instructions.h"

using namespace llvm;

class FatPointers{
public:
  static Type* GetFatPointerType(Type *PointerType);
private:
  static std::map<Type *, Type *> FatPointerTypes;
};

#endif
