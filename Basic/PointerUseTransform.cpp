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
  //this->RawToFPAllocaMap = RawToFPAllocaMap;

  for(auto Pair: RawToFPAllocaMap){
    Replacements[Pair.first] = Pair.second;
  }

  this->SafeLoads = 0;
  this->NoneSafeLoads = 0;
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
void PointerUseTransform::ApplyTo(PointerParameter *PP){
  CallInst *Call = PP->Call;
  Function *F = Call->getCalledFunction();

  if(RawToFPMap.count(F)){
    // Switch this function call to call the Fat Pointer version
    // -- Collect the parameters
    std::vector<Value *> Args;
    for(int i=0; i<Call->getNumArgOperands(); i++){
      // If NULL has been passed as a parameter, transform it
      if(isa<ConstantPointerNull>(Call->getArgOperand(i))){
        IRBuilder<> B(Call);
        Value *FP = FatPointers::CreateFatPointer(
            Call->getArgOperand(i)->getType(), B, "Null");
        Args.push_back(B.CreateLoad(FP));
      } else {
        Args.push_back(Call->getArgOperand(i));
      }
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

  std::set<Value *> CheckedGEPs;

  for(int i=0; i<Chain.size(); i++){
    if(Replacements.count(Chain[i]))
      Chain[i] = Replacements[Chain[i]];
  }
  //errs() << "---\n";

  // If we store a global in a local, the load of the global needs to 
  // be recreated to get the typing correct
  if(auto S = dyn_cast<StoreInst>(Chain.back())){
    if(auto L = dyn_cast<LoadInst>(S->getValueOperand())){
      if(false){
        IRBuilder<> B(L);
        Value *NewLoad = B.CreateLoad(L->getPointerOperand());
        Replacements[L] = NewLoad;
        L->replaceAllUsesWith(NewLoad);
        L->eraseFromParent();
      }
    }
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
        Value *Null = ConstantPointerNull::get(cast<PointerType>(C->getType()));
        Type *IntegerType = IntegerType::getInt64Ty(C->getContext());

        // ToReplace is used so I can use C in the future instructions
        LoadInst *ToReplace = B.CreateLoad(Null);
        C->replaceAllUsesWith(ToReplace);

        if(C->getCalledFunction() && C->getCalledFunction()->getName() == "malloc"){
          // If the next instruction is a cast, delay creating a fat pointer
          PrevBase = C;
          PrevBound = B.CreateIntToPtr(B.CreateAdd(
                C->getArgOperand(0), 
                B.CreatePtrToInt(PrevBase, IntegerType)),
              PrevBase->getType());
        } else {
          PrevBase = Null;
          PrevBound = Null;
        }
        if(!isa<CastInst>(Chain[1])){
          Value *FP = FatPointers::CreateFatPointer(C->getType(), B);
          StoreInFatPointerValue(FP, C, B);
          StoreInFatPointerBase(FP, PrevBase, B);
          StoreInFatPointerBound(FP, PrevBound, B);
          ToReplace->replaceAllUsesWith(B.CreateLoad(FP));
        } else
          ToReplace->replaceAllUsesWith(C);
        ToReplace->eraseFromParent();

      }
    }
  }

  // If the chain ends with a free instruction, set the base to null, signifying
  // unset after the call
  if(auto C = dyn_cast<CallInst>(Chain.back())){
    if(C->getCalledFunction()){
      if(C->getCalledFunction()->getName() == "free"){
        BasicBlock::iterator iter = C;
        iter++;
        IRBuilder<> B(iter);
        Value *FatPointer = Chain.front();
        Value *Null = FatPointers::GetFieldNull(FatPointer);
        StoreInFatPointerValue(FatPointer, Null, B);
      }
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

  // Set equal to null
  if(auto S = dyn_cast<StoreInst>(Chain.back())){
    if(auto C = dyn_cast<ConstantPointerNull>(S->getValueOperand())){
      Type *PointerElementType = S->getPointerOperand()->getType()->getPointerElementType();
      IRBuilder<> B(S);
      if(FatPointers::IsFatPointerType(PointerElementType)){
        // Make a null of the correct type
        Value *Null = FatPointers::GetFieldNull(S->getPointerOperand());
        StoreInFatPointerValue(S->getPointerOperand(), Null, B);
        S->eraseFromParent();
      } else if(PointerElementType->isPointerTy() && Chain.size() == 4
          && isa<LoadInst>(Chain[1]) && isa<GetElementPtrInst>(Chain[2])){

        // Recreate the load
        auto *L = cast<LoadInst>(Chain[1]);
        //auto *NewLoad = B.CreateLoad(L->getPointerOperand());
        auto *NewLoad = LoadFatPointerValue(L->getPointerOperand(), B);
        Replacements[L] = NewLoad;
        L->replaceAllUsesWith(NewLoad);
        L->eraseFromParent();

        // Recreate the GEP
        auto *G = cast<GetElementPtrInst>(Chain[2]);
        std::vector<Value *> Indices;
        for(auto I=G->idx_begin(), E=G->idx_end(); I != E; ++I)
          Indices.push_back(*I);
        auto *NewGep = B.CreateGEP(G->getPointerOperand(), Indices);
        Replacements[G] = NewGep;
        G->replaceAllUsesWith(NewGep);
        G->eraseFromParent();

        Value *Null = FatPointers::GetFieldNull(S->getPointerOperand());
        StoreInFatPointerValue(S->getPointerOperand(), Null, B);
        S->eraseFromParent();

      }

    }
  }

  // Set to the address
  if(Chain.size() == 2){
    if(isa<StoreInst>(Chain[1])
        && isa<AllocaInst>(Chain[0])){
      auto S = cast<StoreInst>(Chain[1]);
      if(IsStoreValueOperand(S, Chain[0])){
        IRBuilder<> B(S);
        Value *FP = FatPointers::CreateFatPointer(Chain[0]->getType(), B);
        SetFatPointerToAddress(FP, Chain[0], B);
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
    if(Replacements.count(V))
      PointerId = Replacements[V];
  int PointerLevel = 0;

  for(int i=0; i<Chain.size(); i++){
    Value *CurrentLink = Chain[i];
    if(auto I = dyn_cast<Instruction>(CurrentLink))
      if(I->getParent() == NULL)
        continue;

    if(auto L = dyn_cast<LoadInst>(CurrentLink)){
      IRBuilder<> B(L);
      Value *Addr = L->getPointerOperand();
      if(FatPointers::IsFatPointerType(Addr->getType()->getPointerElementType())){
        if((i == Chain.size() - 2) && ExpectedFatPointer){
          // Recreate the load for typing
          Replacements[L] = B.CreateLoad(Addr);
          L->replaceAllUsesWith(Replacements[L]);
          L->eraseFromParent();
        } else {

          // Load the value from the fat pointer
          // Remember the most recent fat pointer so we can carry over bounds on
          // GEP or Cast
          PrevBase = LoadFatPointerBase(Addr, B);
          PrevBound  = LoadFatPointerBound(Addr, B);

          auto Next = Chain[i+1];
          if(Replacements.count(Next))
            Next = Replacements[Next];

          // Determine whether this is the last load
          // If we have a GEP next, delay the bounds checking until after it
          if(isa<GetElementPtrInst>(Next)
              && (i != Chain.size() - 3 || !ExpectedFatPointer)){
            CheckedGEPs.insert(Next);
            auto G = dyn_cast<GetElementPtrInst>(Next);
            BasicBlock::iterator iter = G;
            iter++;
            IRBuilder<> AfterGep(iter);

            NoneSafeLoads++;
            FatPointers::CreateBoundsCheck(AfterGep, 
                AfterGep.CreatePointerCast(G, PrevBase->getType()), 
                PrevBase, PrevBound, Print, M);
          } else if(isa<CmpInst>(Next)){
            // If we have a compare next, don't bother bounds checking
            // eg. for the case of "if(t == NULL)"
          } else if(!CheckedGEPs.count(Chain[i-1])){
            if(this->Qualifiers[Pointer(PointerId, PointerLevel)] == SAFE){
              SafeLoads++;
              FatPointers::CreateNullCheck(B, LoadFatPointerValue(Addr, B), Print, M);
            } else {
              NoneSafeLoads++;
              FatPointers::CreateBoundsCheck(B, LoadFatPointerValue(Addr, B),
                  PrevBase, PrevBound, Print, M);
            }
          }

          L->replaceAllUsesWith(LoadFatPointerValue(Addr, B));
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

        auto *NewGep = cast<GetElementPtrInst>(B.CreateGEP(Op, Indices));
        Replacements[G] = NewGep;

        Value *FP = FatPointers::CreateFatPointer(NewGep->getType(), B);
        Value *Null = FatPointers::GetFieldNull(FP);
        StoreInFatPointerValue(FP, NewGep, B);
        if(PrevBase){
          StoreInFatPointerBase(FP, PrevBase, B);
          StoreInFatPointerBound(FP, PrevBound, B);
        } else if(G->getPointerOperand()->getType()->getPointerElementType()
            ->isArrayTy()){
          Type *Array = NewGep->getPointerOperand()->getType()->getPointerElementType();
          Value *Size = GetSizeValue(Array, B);
          SetFatPointerBaseAndBound(FP, NewGep, Size, B);
        } else {
          assert(false && "GEP without previous fat pointer to non-array");
        }
        G->replaceAllUsesWith(B.CreateLoad(FP));
        G->eraseFromParent();
      } else {
        // Recreate the GEP for typing
        std::vector<Value *> Indices;
        for(auto I=G->idx_begin(), E=G->idx_end(); I != E; ++I)
          Indices.push_back(*I);

        Replacements[G] = B.CreateGEP(G->getPointerOperand(), Indices);
        G->replaceAllUsesWith(Replacements[G]);
        G->eraseFromParent();
      }
    } else if(auto C = dyn_cast<CastInst>(CurrentLink)){
      // This should be modified to convert between fat pointer types
      if((i == Chain.size() - 2) && ExpectedFatPointer && C->getDestTy()->isPointerTy()){
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
