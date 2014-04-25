#include "PointerUseTransform.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "Helpers.hpp"

PointerUseTransform::PointerUseTransform(PointerUseCollection *PUC, Module &M){
  this->PUC = PUC;
  this->M = &M;
}

void PointerUseTransform::Apply(){
  for(auto U: PUC->PointerUses)
    U->DispatchTransform(this);
}
void PointerUseTransform::ApplyTo(PointerAssignment *PA){
  // Refollow the chains to contain the updated allocas
  PA->FollowChains(); 

  IRBuilder<> B(PA->Store);

  // Transform the loads coming from the pointer chain
  Value *LhsAlloca = PA->PointerChain.back();
  Value *Lhs = LhsAlloca;
  // -1 to get the last index from the size
  // -1 to ignore the allocation
  for(int i=PA->PointerChain.size() - 2; i>= 0; i--){
    // TODO: This should have bounds checks
    // Strip away the fat pointer layers
    Lhs = LoadFatPointerValue(Lhs, B);
    cast<Instruction>(PA->PointerChain[i])->eraseFromParent();
  }

  // Transform the loads coming from the value chain
  Value *RhsAlloca = PA->ValueChain.back();
  if(PA->ValueChain.size() == 1 && !isa<Constant>(RhsAlloca)){
    // We're assigning to the address of
    SetFatPointerToAddress(Lhs, RhsAlloca, B);
  }else{
    // We're assigning to the value of
    Value *Rhs = RecreateValueChain(PA->ValueChain, B);
    B.CreateStore(Rhs, Lhs);
  }
  PA->Store->eraseFromParent();
}
void PointerUseTransform::ApplyTo(PointerReturn *PR){
}
void PointerUseTransform::ApplyTo(PointerParameter *PP){
  PP->FollowChains();

  IRBuilder<> B(PP->Call);
  PP->Print();
  // For each parameter
  for(int i=0; i<PP->ValueChains.size(); i++){
    // TODO: This needs to be updated to deal with arguments
    if(isa<AllocaInst>(PP->ValueChains[i].back())){
      // TODO: This needs to handle fat pointer capable functions
      Value *Param = RecreateValueChain(PP->ValueChains[i], B);
      if(PP->Call->getOperand(i)->getType()->isPointerTy())
      {
        PP->Call->setOperand(i, LoadFatPointerValue(cast<LoadInst>(Param)->getPointerOperand(), B));
      }
      else
        PP->Call->setOperand(i, Param);

    }
  }
}

Value *PointerUseTransform::RecreateValueChain(std::vector<Value *> Chain, IRBuilder<> &B){
  Value *Rhs = Chain.back();

  for(int i=Chain.size() - 2; i>= 0; i--){
    Value *CurrentLink = Chain[i];
    if(GetLinkType(CurrentLink) == LOAD){
      if(i > 0){
        // Rhs may not be of a fat pointer type if it is a different type
        // that will later be cast to a fat pointer type
        if(FatPointers::IsFatPointerType(
              Rhs->getType()->getPointerElementType())){
          Rhs = LoadFatPointerValue(Rhs, B);
        } else {
          Rhs = B.CreateLoad(Rhs);
        }
      } else {
        // The last load needs to be recreated for typing
        Rhs = B.CreateLoad(Rhs);
      }
    } else if(GetLinkType(CurrentLink) == CAST){
      CastInst *Cast = cast<CastInst>(CurrentLink);
      // If the destination type was a pointer, we need to create
      // a fat pointer
      if(Cast->getDestTy()->isPointerTy()){
        Type *DestTy;
        if(Cast->getDestTy()->getPointerElementType()->isPointerTy()){
          // If it is nested (eg int **x) cast it to a fat pointer
          DestTy = FatPointers::GetFatPointerType(
              Cast->getDestTy()->getPointerElementType())->getPointerTo();
        } else {
          // Otherwise leave it as it is for the moment
          DestTy = Cast->getDestTy();
        }
        Rhs = B.CreateCast(Cast->getOpcode(), Rhs, DestTy);

        // Create a fat pointer object from the cast
        Value *FP = FatPointers::CreateFatPointer(Cast->getDestTy(), B);
        Value *Null = FatPointers::GetFieldNull(FP);
        StoreInFatPointerValue(FP, Rhs, B);
        StoreInFatPointerBase(FP, Null, B);
        StoreInFatPointerBound(FP, Null, B);

        Rhs = B.CreateLoad(FP);
      } else {
        Rhs = B.CreateCast(Cast->getOpcode(), Rhs, Cast->getDestTy());
      }
    }

    // Delete the old load
    cast<Instruction>(Chain[i])->eraseFromParent();
  }
  return Rhs;
}
