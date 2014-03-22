#include "FatPointers.hpp"
#include "llvm/Support/raw_ostream.h"

std::map<Type *, Type *> FatPointers::FatPointerTypes;

Type* FatPointers::GetFatPointerType(Type *PointerType){
  if(FatPointers::FatPointerTypes.count(PointerType) == 1)
    return FatPointerTypes[PointerType];
  
  std::vector<Type *> FatPointerMembers;	
  FatPointerMembers.push_back(PointerType);
  FatPointerMembers.push_back(PointerType);
  FatPointerMembers.push_back(PointerType);
  Type *FatPointerType = StructType::create(FatPointerMembers, "struct.FatPointer");

  FatPointers::FatPointerTypes[PointerType] = FatPointerType;
  return FatPointerType;
}
