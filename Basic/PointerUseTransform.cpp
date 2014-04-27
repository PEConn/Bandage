#include "PointerUseTransform.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "Helpers.hpp"

PointerUseTransform::PointerUseTransform(PointerUseCollection *PUC, Module &M, std::map<Function *, Function *> RawToFPMap){

  this->PUC = PUC;
  this->M = &M;
  this->Print = M.getFunction("printf");
  this->RawToFPMap = RawToFPMap;
}

void PointerUseTransform::Apply(){
  for(auto U: PUC->PointerUses)
    U->DispatchTransform(this);
}
void PointerUseTransform::ApplyTo(PointerAssignment *PA){
  // Refollow the chains to contain the updated allocas
  PA->FollowChains(); 

  IRBuilder<> B(PA->Store);

  // 'RecreateValueChain' makes GEPs return a fat pointer, so if the last
  // instruction in a chain is a fat pointer, add a load to get a value from it
  if(isa<GetElementPtrInst>(PA->PointerChain.front()))
    PA->PointerChain.insert(PA->PointerChain.begin(),
        B.CreateLoad(PA->PointerChain.front()));

  Value *Lhs = RecreateValueChain(PA->PointerChain, B);

  if(PA->Load == NULL && isa<AllocaInst>(PA->ValueChain.front())){
    // Assigning to the address of the Rhs
    SetFatPointerToAddress(Lhs, PA->ValueChain.front(), B);
    PA->Store->eraseFromParent();
    return;
  }


  if(isa<Constant>(PA->ValueChain.back())
      && isa<GEPOperator>(PA->ValueChain.back())){
    auto C = dyn_cast<GEPOperator>(PA->ValueChain.back());
    // Set equal to a constant string
    Value *FP = FatPointers::CreateFatPointer(C->getType(), B);

    // Set the bounds from the constant string
    // This could be replaced with a constant calculation
    Type *CharArray = C->getPointerOperand()->getType()->getPointerElementType();
    Value *Size = GetSizeValue(CharArray, B);
    StoreInFatPointerValue(FP, C, B);
    SetFatPointerBaseAndBound(FP, C, Size, B);

    B.CreateStore(B.CreateLoad(FP), Lhs);
    PA->Store->eraseFromParent();
    return;
  }  
  
  if(PA->Load != NULL && isa<GetElementPtrInst>(PA->ValueChain.front()))
    PA->ValueChain.insert(PA->ValueChain.begin(), 
        B.CreateLoad(PA->ValueChain.front()));
  
  Value *Rhs = RecreateValueChain(PA->ValueChain, B);

  if(isa<AllocaInst>(PA->ValueChain.back())){
    // We're assigning to the value of Rhs
    Rhs = B.CreateLoad(Rhs);
  }

  B.CreateStore(Rhs, Lhs);

  if(PA->Load)
    PA->Load->eraseFromParent();
  PA->Store->eraseFromParent();
}
void PointerUseTransform::ApplyTo(PointerReturn *PR){
  PR->FollowChains();
  PR->Print();

  IRBuilder<> B(PR->Return);
  Value *Rhs = RecreateValueChain(PR->ValueChain, B);
  errs() << *PR->Load << "\n";
  if(PR->Load){
  errs() << __LINE__ << "\n";
    PR->Load->eraseFromParent();
    Rhs = B.CreateLoad(Rhs);
  }
  B.CreateRet(Rhs);
  PR->Return->eraseFromParent();
}
void PointerUseTransform::ApplyTo(PointerParameter *PP){
  PP->FollowChains();

  Function *F = PP->Call->getCalledFunction();
  bool StripFatPointers = false;
  if(RawToFPMap.count(F))
    F = RawToFPMap[F];
  else
    StripFatPointers = true;

  IRBuilder<> B(PP->Call);
  std::vector<Value *> Args;

  // For each parameter
  for(int i=0; i<PP->ValueChains.size(); i++){
    if(isa<AllocaInst>(PP->ValueChains[i].back())){
      if(F->getName() == "free"){
        Value *FP = PP->ValueChains[i].back();

        Value *Null = FatPointers::GetFieldNull(FP);
        StoreInFatPointerBase(FP, Null, B);
        StoreInFatPointerBound(FP, Null, B);
      }
      // Remove the last load from the chain
      Value *Param = RecreateValueChain(PP->ValueChains[i], B);

      if(PP->Loads[i]){
        PP->Loads[i]->eraseFromParent();
        Param = B.CreateLoad(Param);
      }
      if(StripFatPointers)
        if(FatPointers::IsFatPointerType(Param->getType()))
          Param = LoadFatPointerValue(cast<LoadInst>(Param)->getPointerOperand(), B);

      Args.push_back(Param);
    } else {
      Args.push_back(PP->Call->getArgOperand(i));
    }
  }
  CallInst *NewCall = CallInst::Create(F, Args, "", PP->Call);

  PP->Call->replaceAllUsesWith(NewCall);
  PP->Call->eraseFromParent();
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
  Value *PrevBase = NULL;
  Value *PrevBound = NULL;

  // If the chain starts with a malloc instruction, calculate the base and bounds
  // for the future fat pointer
  if(auto C = dyn_cast<CallInst>(Chain.back())){
    if(C->getCalledFunction()->getName() == "malloc"){
      PrevBase = C;
      Type *IntegerType = IntegerType::getInt64Ty(PrevBase->getContext());
      PrevBound = B.CreateIntToPtr(B.CreateAdd(
            C->getArgOperand(0), 
            B.CreatePtrToInt(PrevBase, IntegerType)),
          PrevBase->getType());
    }
  }

  for(int i=Chain.size() - 2; i>= 0; i--){
    Value *CurrentLink = Chain[i];
    if(GetLinkType(CurrentLink) == LOAD){
      // Rhs may not be of a fat pointer type if it is a different type
      // that will later be cast to a fat pointer type
      // TODO: Redo bounds checking - needs to be on normal loads
      if(FatPointers::IsFatPointerType(Rhs->getType()->getPointerElementType())){
        // Remember the most recent fat pointer so we can carry over bounds on
        // GEP or Cast
        PrevBase = LoadFatPointerBase(Rhs, B);
        PrevBound  = LoadFatPointerBound(Rhs, B);

        FatPointers::CreateBoundsCheck(B, LoadFatPointerValue(Rhs, B),
            PrevBase, PrevBound, Print, M);
        Rhs = LoadFatPointerValue(Rhs, B);
      } else {
        Rhs = B.CreateLoad(Rhs);
      }
    } else if(GetLinkType(CurrentLink) == GEP){
      auto Gep = cast<GetElementPtrInst>(CurrentLink);
      std::vector<Value *> Indices;
      for(auto I=Gep->idx_begin(), E=Gep->idx_end(); I != E; ++I)
        Indices.push_back(*I);

      Value *NewGep = B.CreateGEP(Rhs, Indices);

      Value *FP = FatPointers::CreateFatPointer(NewGep->getType(), B);
      Value *Null = FatPointers::GetFieldNull(FP);
      StoreInFatPointerValue(FP, NewGep, B);
      if(PrevBase){
        StoreInFatPointerBase(FP, PrevBase, B);
        StoreInFatPointerBound(FP, PrevBound, B);
      } else {
        assert(false && "GEP without previous fat pointer");
      }
      Rhs = FP;
      //Rhs = NewGep;
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
        if(PrevBase){
          PrevBase = B.CreateCast(Cast->getOpcode(), PrevBase, DestTy);
          PrevBound = B.CreateCast(Cast->getOpcode(), PrevBound, DestTy);
          StoreInFatPointerBase(FP, PrevBase, B);
          StoreInFatPointerBound(FP, PrevBound, B);
        } else {
          errs() << "Cast without previous fat pointer.\n";
        }

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
