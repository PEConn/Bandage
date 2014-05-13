#include "TypeDuplicater.hpp"
#include <stack>
#include <fstream>
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Metadata.h"
#include "FatPointers.hpp"

TypeDuplicater::TypeDuplicater(Module &M, FindUsedTypes *FUT, std::string FuncFile){

  std::set<std::string> IntDecls;
  if(FuncFile != ""){
    std::ifstream File(FuncFile);
    std::string Line;
    while(std::getline(File, Line)){
      IntDecls.insert(Line);
    }
    File.close();
  }

  for(auto T: FUT->getTypes()){
    if(T->isStructTy()){
      RawStructs.insert(cast<StructType>(T));
    }
  }

  RemoveNonLocalTypes(M, IntDecls);

  CreateSkeletonTypes();
  FillTypeBodies();
  DisplayFPTypes(new DataLayout(&M));
}

void TypeDuplicater::RemoveNonLocalTypes(Module &M, std::set<std::string> IntDecls){
  std::set<Function *> UndefinedFunctions;
  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    Function *F = &*IF;

    if(F->isDeclaration() && !IntDecls.count(F->getName()))
      UndefinedFunctions.insert(F);
  }

  for(auto F: UndefinedFunctions){
    FunctionType *FuncType = cast<FunctionType>(F->getFunctionType());
    for(int i=0; i<FuncType->getNumParams(); i++){
      Type *T =FuncType->getParamType(i);

      while(T->isPointerTy())
        T = T->getPointerElementType();
      if(auto *ST = dyn_cast<StructType>(T))
        RawStructs.erase(ST);
    }

    Type *T =FuncType->getReturnType();
    while(T->isPointerTy())
      T = T->getPointerElementType();
    if(auto *ST = dyn_cast<StructType>(T))
      RawStructs.erase(ST);
  }
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
      int PointerLevels = 0;
      while(Old->isPointerTy()){
        PointerLevels++;
        Old = Old->getPointerElementType();
      }

      Type *New = Old;
      if(auto OldAsST = dyn_cast<StructType>(Old))
        if(RawStructs.count(OldAsST))
          New = RawToFPMap[OldAsST]; 

      while(PointerLevels--)
        New = FatPointers::GetFatPointerType(New->getPointerTo());

      FPStructElements.push_back(New);
    }

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
void TypeDuplicater::DisplayFPTypes(DataLayout *DL){
  errs() << "----------FPTypes----------\n";
  for(auto T: FPStructs){
    //errs() << DL->getTypeAllocSizeInBits(T)/8 << "B: " << *T << "\n";
    errs() << *T << "\n";
  }
  errs() << "---------------------------\n";
}

Type *TypeDuplicater::remapType(Type *srcType){
  // Follow any pointer or array types through to the base element type
  // Wrappers contains 0 for a pointer and the array size for an array
  std::stack<int> Wrappers;
  Type *ElementType = srcType;
  while(ElementType->isPointerTy() || ElementType->isArrayTy()){
    if(ElementType->isPointerTy()){
      Wrappers.push(0);
      ElementType = ElementType->getPointerElementType();
    } else {
      Wrappers.push(ElementType->getArrayNumElements());
      ElementType = ElementType->getArrayElementType();
    }
  }

  // Check to see if we have a fat pointer version of the type
  StructType *ST = dyn_cast<StructType>(ElementType);
  if(!ST)
    return srcType;
  if(!RawStructs.count(ST))
    return srcType;

  Type *Ret = RawToFPMap[ST];

  // Reapply any pointer or array levels
  while(!Wrappers.empty()){
    if(Wrappers.top() == 0)
      Ret = Ret->getPointerTo();
    else
      Ret = ArrayType::get(Ret, Wrappers.top());

    Wrappers.pop();
  }

  //errs() << *srcType << "->" << *Ret << "\n";
  return Ret;
}

std::set<StructType *> TypeDuplicater::GetFPTypes(){
  return FPStructs;
}
std::set<StructType *> TypeDuplicater::GetRawTypes(){
  return RawStructs;
}
