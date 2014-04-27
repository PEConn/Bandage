#include "PointerUseTransform.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
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
  Value *Lhs = RecreateValueChain(PA->PointerChain, B);

  if(PA->Load == NULL 
      && !isa<GetElementPtrInst>(PA->ValueChain.front())){
    if(isa<Constant>(PA->ValueChain.back())){
      // We're assigning to a constant
      if(auto C = dyn_cast<GEPOperator>(PA->ValueChain.back())){
        // Set equal to a constant string
        Value *FP = FatPointers::CreateFatPointer(C->getType(), B);

        // TODO: Set correct bounds
        StoreInFatPointerValue(FP, C, B);
        StoreInFatPointerBase(FP, C, B);
        StoreInFatPointerBound(FP, C, B);

        B.CreateStore(B.CreateLoad(FP), Lhs);
      } else {
        Value *V = RecreateValueChain(PA->ValueChain, B);
        B.CreateStore(V, Lhs);
      }
    } else {
      Value *V = RecreateValueChain(PA->ValueChain, B);
      if(isa<AllocaInst>(PA->ValueChain.front()))
        // We're assigning to the address of
        SetFatPointerToAddress(Lhs, V, B);
      else{
        B.CreateStore(V, Lhs);
      }

    }
  }else{
    // We're assigning to the value of Rhs
    Value *Rhs = RecreateValueChain(PA->ValueChain, B);
    if(Rhs->getType() != Lhs->getType()){
      if(FatPointers::IsFatPointerType(Rhs->getType()->getPointerElementType()))
        Rhs = LoadFatPointerValue(Rhs, B);
      else
        Lhs = LoadFatPointerValue(Lhs, B);

    }

    B.CreateStore(B.CreateLoad(Rhs), Lhs);

    if(PA->Load)
      PA->Load->eraseFromParent();
  }
  PA->Store->eraseFromParent();
}
void PointerUseTransform::ApplyTo(PointerReturn *PR){
}
void PointerUseTransform::ApplyTo(PointerParameter *PP){
  PP->FollowChains();
  //PP->Print();

  IRBuilder<> B(PP->Call);
  // For each parameter
  for(int i=0; i<PP->ValueChains.size(); i++){
    // TODO: This needs to be updated to deal with arguments
    if(isa<AllocaInst>(PP->ValueChains[i].back())){
      // TODO: This needs to handle fat pointer capable functions
      Value *Param = RecreateValueChain(PP->ValueChains[i], B);
      if(PP->Call->getOperand(i)->getType()->isPointerTy()
          && FatPointers::IsFatPointerType(Param->getType()))
      {
        Param = LoadFatPointerValue(cast<LoadInst>(Param)->getPointerOperand(), B);
      }
      PP->Call->setOperand(i, Param);
    }
  }
}
void PointerUseTransform::ApplyTo(PointerCompare *PC){
  PC->FollowChains();

  IRBuilder<> B(PC->Cmp);
  Value *V1 = RecreateValueChain(PC->Chain1, B);
  Value *V2 = RecreateValueChain(PC->Chain2, B);
  Value *NewCmp;
  if(isa<ICmpInst>(PC->Cmp)){
    NewCmp = B.CreateICmp(PC->Cmp->getPredicate(), V1, V2);
  } else {
    NewCmp = B.CreateFCmp(PC->Cmp->getPredicate(), V1, V2);
  }
  PC->Cmp->replaceAllUsesWith(NewCmp);
  PC->Cmp->eraseFromParent();
}

Value *PointerUseTransform::RecreateValueChain(std::vector<Value *> Chain, IRBuilder<> &B){
  Value *Rhs = Chain.back();

  for(int i=Chain.size() - 2; i>= 0; i--){
    Value *CurrentLink = Chain[i];
    if(GetLinkType(CurrentLink) == LOAD){
      // Rhs may not be of a fat pointer type if it is a different type
      // that will later be cast to a fat pointer type
      if(FatPointers::IsFatPointerType(
            Rhs->getType()->getPointerElementType())){
        Rhs = LoadFatPointerValue(Rhs, B);
      } else {
        Rhs = B.CreateLoad(Rhs);
      }
    } else if(GetLinkType(CurrentLink) == GEP){
      auto Gep = cast<GetElementPtrInst>(CurrentLink);
      // TODO: Naturally, bounds check
      std::vector<Value *> Indices;
      for(auto I=Gep->idx_begin(), E=Gep->idx_end(); I != E; ++I)
        Indices.push_back(*I);

      Value *NewGep = B.CreateGEP(Rhs, Indices);

      // TODO: Carry bounds over here
      Value *FP = FatPointers::CreateFatPointer(NewGep->getType(), B);
      Value *Null = FatPointers::GetFieldNull(FP);
      StoreInFatPointerValue(FP, NewGep, B);
      StoreInFatPointerBase(FP, Null, B);
      StoreInFatPointerBound(FP, Null, B);
      //Rhs = B.CreateLoad(FP);
      Rhs = FP;
      errs() << *Rhs << "\n";
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
    // Delete the old link
    cast<Instruction>(Chain[i])->eraseFromParent();
  }
  return Rhs;
}

  /*
  for(int i=PA->PointerChain.size() - 2; i>= 0; i--){
    // TODO: This should have bounds checks
    // Strip away the fat pointer layers
    if(GetLinkType(PA->PointerChain[i]) == LOAD){
      Lhs = LoadFatPointerValue(Lhs, B);
    } else if (GetLinkType(PA->PointerChain[i]) == GEP){
      // TODO: Naturally, bounds check
      auto Gep = cast<GetElementPtrInst>(PA->PointerChain[i]);
      std::vector<Value *> Indices;
      for(auto I=Gep->idx_begin(), E=Gep->idx_end(); I != E; ++I)
        Indices.push_back(*I);

      Lhs = B.CreateGEP(Lhs, Indices);

    } else {
      assert(false && "Something other than LOAD or GEP in PointerChain");
    }

    cast<Instruction>(PA->PointerChain[i])->eraseFromParent();
  }
  */
