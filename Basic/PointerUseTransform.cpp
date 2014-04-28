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
/*
void PointerUseTransform::ApplyTo(PointerAssignment *PA){
  // Refollow the chains to contain the updated allocas
  PA->FollowChains(); 
  PA->Print();

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
  PC->Print();

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
// */

void PointerUseTransform::PointerUseTransform::ApplyTo(PU *P){
  RecreateValueChain(P->Chain);
}
void PointerUseTransform::ApplyTo(PointerReturn *PR){}
void PointerUseTransform::ApplyTo(PointerParameter *PP){
  CallInst *Call = PP->Call;
  Function *F = Call->getCalledFunction();

  if(RawToFPMap.count(F)){
    errs() << "No transform: " << F->getName() << "\n";
    // Switch this function call to call the Fat Pointer version
    // -- Collect the parameters
    std::vector<Value *> Args;
    for(int i=0; i<Call->getNumArgOperands(); i++){
      Args.push_back(Call->getArgOperand(i));
    }

    // -- Create the new function call
    CallInst *NewCall = CallInst::Create(RawToFPMap[F], Args, "", Call);

    // -- Replace the old call with the new one
    Call->replaceAllUsesWith(NewCall);
    Call->eraseFromParent();
  } else {
    // Strip the fat pointers
    errs() << "Transform: " << F->getName() << "\n";
    IRBuilder<> B(Call);
    for(int i=0; i<Call->getNumArgOperands(); i++){
      Value *Param = Call->getArgOperand(i);

      if(FatPointers::IsFatPointerType(Param->getType())){
        Call->setArgOperand(i, LoadFatPointerValue(Param, B));
        //Call->setArgOperand(i, LoadFatPointerValue(cast<LoadInst>(Param)->getPointerOperand(), B));
      }
    }
  }

}

void PointerUseTransform::RecreateValueChain(std::vector<Value *> Chain){
  //Value *Rhs = Chain.back();
  Value *PrevBase = NULL;
  Value *PrevBound = NULL;

  // If the chain starts with a malloc instruction, calculate the base and bounds
  // for the future fat pointer
  if(auto C = dyn_cast<CallInst>(Chain.front())){
    if(C->getCalledFunction()->getName() == "malloc"){
      BasicBlock::iterator iter = C;
      iter++;
      IRBuilder<> B(iter);

      PrevBase = C;
      Type *IntegerType = IntegerType::getInt64Ty(PrevBase->getContext());
      PrevBound = B.CreateIntToPtr(B.CreateAdd(
            C->getArgOperand(0), 
            B.CreatePtrToInt(PrevBase, IntegerType)),
          PrevBase->getType());
    }
  }

  // Set equal to a constant string
  if(auto S = dyn_cast<StoreInst>(Chain.back())){
    if(auto G = dyn_cast<GEPOperator>(S->getValueOperand())){
      if(auto C = dyn_cast<Constant>(G->getPointerOperand())){
        IRBuilder<> B(S);

        Value *FP = FatPointers::CreateFatPointer(G->getType(), B);

        // This could be replaced with a constant calculation
        Type *CharArray = G->getPointerOperand()->getType()->getPointerElementType();
        Value *Size = GetSizeValue(CharArray, B);
        StoreInFatPointerValue(FP, G, B);
        SetFatPointerBaseAndBound(FP, G, Size, B);

        S->setOperand(0, B.CreateLoad(FP));
      }
    }
  }

  bool ExpectedFatPointer = false;
  if(auto S = dyn_cast<StoreInst>(Chain.back())){
    if(IsStoreValueOperand(S, Chain[Chain.size() - 2])){
      ExpectedFatPointer = true;
    }
  }

  for(int i=0; i<Chain.size(); i++){
    Value *CurrentLink = Chain[i];

    if(auto L = dyn_cast<LoadInst>(CurrentLink)){
      IRBuilder<> B(L);
      Value *Op = L->getPointerOperand();
      if(FatPointers::IsFatPointerType(Op->getType()->getPointerElementType())){
        if((i == Chain.size() - 2) && ExpectedFatPointer){
          // Load the fat pointer to be stored
          L->replaceAllUsesWith(B.CreateLoad(Op));
          L->eraseFromParent();
        } else {
          // Load the value from the fat pointer
          // Remember the most recent fat pointer so we can carry over bounds on
          // GEP or Cast
          PrevBase = LoadFatPointerBase(Op, B);
          PrevBound  = LoadFatPointerBound(Op, B);

          // If we have a GEP next, delay the bounds checking until after it
          FatPointers::CreateBoundsCheck(B, LoadFatPointerValue(Op, B),
              PrevBase, PrevBound, Print, M);

          L->replaceAllUsesWith(LoadFatPointerValue(Op, B));
          L->eraseFromParent();
        }
      }
    } else if(auto G = dyn_cast<GetElementPtrInst>(CurrentLink)){
      if((i == Chain.size() - 2) && ExpectedFatPointer){
        // We expect a fat pointer, so create one from the GEP
        IRBuilder<> B(G);
        Value *Op = G->getPointerOperand();

        std::vector<Value *> Indices;
        for(auto I=G->idx_begin(), E=G->idx_end(); I != E; ++I)
          Indices.push_back(*I);

        Value *NewGep = B.CreateGEP(Op, Indices);

        Value *FP = FatPointers::CreateFatPointer(NewGep->getType(), B);
        Value *Null = FatPointers::GetFieldNull(FP);
        StoreInFatPointerValue(FP, NewGep, B);
        if(PrevBase){
          StoreInFatPointerBase(FP, PrevBase, B);
          StoreInFatPointerBound(FP, PrevBound, B);
        } else {
          assert(false && "GEP without previous fat pointer");
        }
        G->replaceAllUsesWith(B.CreateLoad(FP));
        G->eraseFromParent();
      }
    } else if(auto C = dyn_cast<CastInst>(CurrentLink)){
      if((i == Chain.size() - 2) && ExpectedFatPointer){

        errs() << "HERE\n";
        IRBuilder<> B(C);

        Type *DestTy;
        if(C->getDestTy()->getPointerElementType()->isPointerTy()){
          // If it is nested (eg int **x) cast it to a fat pointer
          DestTy = FatPointers::GetFatPointerType(
              C->getDestTy()->getPointerElementType())->getPointerTo();
        } else {
          // Otherwise leave it as it is for the moment
          DestTy = C->getDestTy();
        }
        Value *NewCast = B.CreateCast(C->getOpcode(), C->getOperand(0), DestTy);

        // Create a fat pointer object from the cast
        Value *FP = FatPointers::CreateFatPointer(C->getDestTy(), B);
        Value *Null = FatPointers::GetFieldNull(FP);
        StoreInFatPointerValue(FP, NewCast, B);
        if(PrevBase){
          PrevBase = B.CreateCast(C->getOpcode(), PrevBase, DestTy);
          PrevBound = B.CreateCast(C->getOpcode(), PrevBound, DestTy);
          StoreInFatPointerBase(FP, PrevBase, B);
          StoreInFatPointerBound(FP, PrevBound, B);
        } else {
          errs() << "Cast without previous fat pointer.\n";
        }

        C->replaceAllUsesWith(B.CreateLoad(FP));
        C->eraseFromParent();
      }
    }
  }
}
