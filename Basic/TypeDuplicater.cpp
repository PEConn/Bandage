#include "TypeDuplicater.hpp"
#include "llvm/Support/raw_ostream.h"
#include "FatPointers.hpp"

TypeDuplicater::TypeDuplicater(Module &M, FindUsedTypes *FUT){
  for(auto T: FUT->getTypes()){
    if(T->isStructTy())
      RawStructs.insert(cast<StructType>(T));
  }
  CreateSkeletonTypes();
  FillTypeBodies();
}

void TypeDuplicater::CreateSkeletonTypes(){
  for(auto ST: RawStructs){
    std::string Name = ST->getName().str() + ".FP";
    RawToFPMap[ST] = StructType::create(ST->getContext(), Name);
  }
}
void TypeDuplicater::FillTypeBodies(){
  for(auto ST: RawStructs){
    std::vector<Type *> FPStructElements;

    for(int i=0; i< ST->getNumElements(); i++){
      // Replace all pointers with fat pointers
      // Replace all types with their FP equivalent
      Type *Old = ST->getElementType(i);
      bool WasPointer;
      if(Old->isPointerTy()){
        WasPointer = true;
        Old = Old->getPointerElementType();
      }

      Type *New = Old;
      if(auto OldAsST = dyn_cast<StructType>(Old))
        if(RawStructs.count(OldAsST))
          New = RawToFPMap[OldAsST]; 
      
      if(WasPointer)
        New = FatPointers::GetFatPointerType(New->getPointerTo());

      FPStructElements.push_back(New);
    }

    //ST->setBody(FPStructElements);
    RawToFPMap[ST]->setBody(FPStructElements);
    FPStructs.insert(RawToFPMap[ST]);
  }
}
bool TypeDuplicater::NeedsFPType(StructType *ST){
  for(int i=0; i<ST->getNumElements(); i++)
    if(ST->getElementType(i)->isPointerTy())
      return true;
  return false;
}
void TypeDuplicater::DisplayFPTypes(){
  for(auto T: FPStructs){
    errs() << *T << "\n";
  }
}

Type *TypeDuplicater::remapType(Type *srcType){
  // Follow any pointer types through to the base element type
  int PointerLevels = 0;
  Type *ElementType = srcType;
  while(ElementType->isPointerTy()){
    ElementType = ElementType->getPointerElementType();
    PointerLevels++;
  }

  // Check to see if we have a fat pointer version of the type
  StructType *ST = dyn_cast<StructType>(ElementType);
  if(!ST)
    return srcType;
  if(!RawStructs.count(ST))
    return srcType;

  Type *Ret = RawToFPMap[ST];

  // Reapply any pointer levels
  while(PointerLevels--)
    Ret = Ret->getPointerTo();

  return Ret;
}


std::set<StructType *> TypeDuplicater::GetFPTypes(){
  return FPStructs;
}
std::set<StructType *> TypeDuplicater::GetRawTypes(){
  return RawStructs;
}
