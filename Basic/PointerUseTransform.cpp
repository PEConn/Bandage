#include "PointerUseTransform.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "Helpers.hpp"

PointerUseTransform::PointerUseTransform(PointerUseCollection *PUC, Module &M, std::map<Function *, Function *> RawToFPMap, std::map<AllocaInst *, AllocaInst *> RawToFPAllocaMap){

  this->PUC = PUC;
  this->M = &M;
  this->Print = M.getFunction("printf");
  this->RawToFPMap = RawToFPMap;
  this->RawToFPAllocaMap = RawToFPAllocaMap;
}

void PointerUseTransform::AddPointerAnalysis(std::map<Pointer, CCuredPointerType> Qs, ValueToValueMapTy &VMap){
  for(auto Q: Qs){
    Value *OriginalAlloca = Q.first.id;
    Value *DuplicatedAlloca = VMap[OriginalAlloca];
    Pointer NewKey = Pointer(DuplicatedAlloca, Q.first.level);
    this->Qualifiers[NewKey] = Q.second;
  }
}

void PointerUseTransform::Apply(){
  for(auto U: PUC->PointerUses)
    U->DispatchTransform(this);
  for(auto U: PUC->PointerParams)
    U->DispatchTransform(this);
}

void PointerUseTransform::PointerUseTransform::ApplyTo(PU *P){
  //P->Print();
  RecreateValueChain(P->Chain);
}
void PointerUseTransform::ApplyTo(PointerReturn *PR){}
void PointerUseTransform::ApplyTo(PointerParameter *PP){
  CallInst *Call = PP->Call;
  Function *F = Call->getCalledFunction();

  if(RawToFPMap.count(F)){
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
    IRBuilder<> B(Call);
    for(int i=0; i<Call->getNumArgOperands(); i++){
      Value *Param = Call->getArgOperand(i);

      if(FatPointers::IsFatPointerType(Param->getType())){
        Call->setArgOperand(i, LoadFatPointerValue(Param, B));
      }
    }
  }

}

void PointerUseTransform::RecreateValueChain(std::vector<Value *> Chain){
  //Value *Rhs = Chain.back();
  Value *PrevBase = NULL;
  Value *PrevBound = NULL;

  for(int i=0; i<Chain.size(); i++){
    if(Replacements.count(Chain[i]))
      Chain[i] = Replacements[Chain[i]];
  }

  // If the chain starts with a malloc instruction, calculate the base and bounds
  // for the future fat pointer
  // If the chain starts with a call from a non-fat-pointer function, 
  // Wrap it in a fat pointer
  if(auto C = dyn_cast<CallInst>(Chain.front())){
    if(C->getType()->isPointerTy()){
      if(!RawToFPMap.count(C->getCalledFunction())){
        BasicBlock::iterator iter = C;
        iter++;
        IRBuilder<> B(iter);

        if(C->getCalledFunction()->getName() == "malloc"){
          // If the next instruction is a cast, delay creating a fat pointer until then
          if(isa<CastInst>(Chain[1])){
            PrevBase = C;
            Type *IntegerType = IntegerType::getInt64Ty(PrevBase->getContext());
            PrevBound = B.CreateIntToPtr(B.CreateAdd(
                  C->getArgOperand(0), 
                  B.CreatePtrToInt(PrevBase, IntegerType)),
                PrevBase->getType());
          } else {
            Value *FP = FatPointers::CreateFatPointer(C->getType(), B);
            C->replaceAllUsesWith(B.CreateLoad(FP));
            StoreInFatPointerValue(FP, C, B);
            StoreInFatPointerBase(FP, C, B);
            StoreInFatPointerBound(FP, C, B);
          }
        } else {
          Value *Null = ConstantPointerNull::get(cast<PointerType>(C->getType()));

          if(isa<CastInst>(Chain[1])){
            PrevBase = Null;
            PrevBound = Null;
          } else {
            Value *FP = FatPointers::CreateFatPointer(C->getType(), B);
            C->replaceAllUsesWith(B.CreateLoad(FP));
            StoreInFatPointerValue(FP, C, B);
            StoreInFatPointerBase(FP, Null, B);
            StoreInFatPointerBound(FP, Null, B);
          }
        }
      }

    }
  }

  // If the chain ends with a free instruction, set the base to null, signifying
  // unset after the call
  if(auto C = dyn_cast<CallInst>(Chain.back())){
    if(C->getCalledFunction()->getName() == "free"){
      BasicBlock::iterator iter = C;
      iter++;
      IRBuilder<> B(iter);
      Value *FatPointer = RawToFPAllocaMap[cast<AllocaInst>(Chain.front())];
      Value *Null = FatPointers::GetFieldNull(FatPointer);
      StoreInFatPointerValue(FatPointer, Null, B);
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
  if(auto S = dyn_cast<StoreInst>(Chain.back()))
    if(IsStoreValueOperand(S, Chain[Chain.size() - 2]))
      if(Chain[Chain.size() - 2]->getType()->isPointerTy())
        ExpectedFatPointer = true;
  if(auto C = dyn_cast<CallInst>(Chain.back()))
    if(RawToFPMap.count(C->getCalledFunction()))
      ExpectedFatPointer = true;
  if(auto R = dyn_cast<ReturnInst>(Chain.back()))
    if(R->getReturnValue()->getType()->isPointerTy())
      ExpectedFatPointer = true;
  
  // Store this for later use for CCured optimisation
  Value *PointerId = NULL;
  if(auto V = dyn_cast<AllocaInst>(Chain.front()))
    if(RawToFPAllocaMap.count(V))
      PointerId = RawToFPAllocaMap[V];
  int PointerLevel = 0;

  for(int i=0; i<Chain.size(); i++){
    //errs() << *Chain[i] << "\n";
    Value *CurrentLink = Chain[i];

    if(auto L = dyn_cast<LoadInst>(CurrentLink)){
      IRBuilder<> B(L);
      Value *Op = L->getPointerOperand();
      if(FatPointers::IsFatPointerType(Op->getType()->getPointerElementType())){
        if((i == Chain.size() - 2) && ExpectedFatPointer){
          // Recreate the load for typing
          L->replaceAllUsesWith(B.CreateLoad(Op));
          L->eraseFromParent();
        } else {
          // Load the value from the fat pointer
          // Remember the most recent fat pointer so we can carry over bounds on
          // GEP or Cast
          PrevBase = LoadFatPointerBase(Op, B);
          PrevBound  = LoadFatPointerBound(Op, B);

          // TODO: If we have a GEP next, delay the bounds checking until after it
          if(this->Qualifiers[Pointer(PointerId, PointerLevel)] == SAFE){
            FatPointers::CreateNullCheck(B, LoadFatPointerValue(Op, B), Print, M);
          } else {
            FatPointers::CreateBoundsCheck(B, LoadFatPointerValue(Op, B),
                PrevBase, PrevBound, Print, M);
          }

          L->replaceAllUsesWith(LoadFatPointerValue(Op, B));
          L->eraseFromParent();
        }
      }

      PointerLevel++;
    } else if(auto G = dyn_cast<GetElementPtrInst>(CurrentLink)){
      IRBuilder<> B(G);
      if((i == Chain.size() - 2) && ExpectedFatPointer){
        // We expect a fat pointer, so create one from the GEP
        Value *Op = G->getPointerOperand();

        std::vector<Value *> Indices;
        for(auto I=G->idx_begin(), E=G->idx_end(); I != E; ++I)
          Indices.push_back(*I);

        Value *NewGep = B.CreateGEP(Op, Indices);
        Replacements[G] = NewGep;

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
      } else {
        std::vector<Value *> Indices;
        for(auto I=G->idx_begin(), E=G->idx_end(); I != E; ++I)
          Indices.push_back(*I);

        Replacements[G] = B.CreateGEP(G->getPointerOperand(), Indices);
        G->replaceAllUsesWith(Replacements[G]);
        G->eraseFromParent();
      }
    } else if(auto C = dyn_cast<CastInst>(CurrentLink)){
      // This should be modified to convert between fat pointer types
      /*Value *FP = C->getOperand(0);
      if(FatPointers::IsFatPointerType(FP)){
        IRBuilder<> B(C);
      }*/
      if((i == Chain.size() - 2) && ExpectedFatPointer){

        IRBuilder<> B(C);

        Type *DestTy;
        if(C->getDestTy()->isPointerTy()
            && C->getDestTy()->getPointerElementType()->isPointerTy()){
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
