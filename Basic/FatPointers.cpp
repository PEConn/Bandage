#include "FatPointers.hpp"

Type* FatPointers::GetFatPointerType(Type *PointerType){
  if(FatPointerTypes.count(PointerType) == 1)
    return FatPointerTypes[PointerType];
  
  std::vector<Type *> FatPointerMembers;	
  FatPointerMembers.push_back(PointerType);
  FatPointerMembers.push_back(PointerType);
  FatPointerMembers.push_back(PointerType);
  Type *FatPointerType = StructType::create(FatPointerMembers, "struct.FatPointer");

  FatPointerTypes[PointerType] = FatPointerType;
  return FatPointerType;
}
