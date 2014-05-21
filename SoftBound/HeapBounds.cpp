#include "HeapBounds.hpp"
#include "llvm/Support/raw_ostream.h"

HeapBounds::HeapBounds(Module &M){
  /*
  std::vector<Type *> ParamTypes;
  ParamTypes.push_back(PointerType);
  ParamTypes.push_back(PointerType);
  ParamTypes.push_back(PointerType);
  FunctionType *FuncType = FunctionType::get(
      Type::getVoidTy(PointerType->getContext()), ParamTypes, false);

  Function *BoundsCheckFunc = Function::Create(FuncType,
      GlobalValue::LinkageTypes::ExternalLinkage, "BoundsCheck", M);
  if(Inline)
    BoundsCheckFunc->addFnAttr(Attribute::AlwaysInline);

    */
  Type *VoidTy = Type::getVoidTy(M.getContext());
  Type *PtrTy = Type::getInt8PtrTy(M.getContext());
  Type *PtrPtrTy = PtrTy->getPointerTo();

  auto Linkage = GlobalValue::LinkageTypes::ExternalLinkage;

  FunctionType *Type1 = FunctionType::get(VoidTy, false);
  Teardown = Function::Create(Type1, Linkage, "TableTeardown", &M);
  Setup = Function::Create(Type1, Linkage, "TableSetup", &M);

  std::vector<Type *> LookupParams;
  LookupParams.push_back(PtrTy);
  LookupParams.push_back(PtrPtrTy);
  LookupParams.push_back(PtrPtrTy);
  FunctionType *LookupType = FunctionType::get(VoidTy, LookupParams, false);
  Lookup = Function::Create(LookupType, Linkage, "TableLookup", &M);

  std::vector<Type *> AssignParams;
  AssignParams.push_back(PtrTy);
  AssignParams.push_back(PtrTy);
  AssignParams.push_back(PtrTy);
  FunctionType *AssignType = FunctionType::get(VoidTy, AssignParams, false);
  Assign = Function::Create(AssignType, Linkage, "TableAssign", &M);

  if(!IsValid())
    errs() << "Could not create heap bounds headers.\n";
}
bool HeapBounds::IsValid(){
  return (Teardown != NULL) && (Setup != NULL) 
    && (Lookup != NULL) && (Assign != NULL);
}
void HeapBounds::InsertTableLookup(IRBuilder<> &B, Value *Key, Value *Base, Value *Bound){
  // Assumes that key, base and value are all i8*
  if(!IsValid())
    return;

  B.CreateCall3(Lookup, Key, Base, Bound);
}
void HeapBounds::InsertTableAssign(IRBuilder<> &B, Value *Key, Value *Base, Value *Bound){
  // Assumes that key, base and value are all i8*
  if(!IsValid())
    return;

  B.CreateCall3(Assign, Key, Base, Bound);
}
