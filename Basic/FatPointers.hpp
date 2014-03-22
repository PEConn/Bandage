#ifndef FAT_POINTERS
#define FAT_POINTERS

#include <vector>
#include <map>
#include "llvm/IR/Instructions.h"

using namespace llvm;

class FatPointers{
public:
  Type* GetFatPointerType(Type *PointerType);
private:
  std::map<Type *, Type *> FatPointerTypes;
};

#endif
