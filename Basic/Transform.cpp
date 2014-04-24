#include <set>
#include <map>
#include <algorithm>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

#include "Transform.hpp"
#include "Helpers.hpp"

Transform::Transform(InstructionCollection *Instructions, std::map<Function *, Function *> Map, Module &M){
  this->Instructions = Instructions;
  this->RawToFPFunctionMap = Map;
  this->M = &M;
  this->DL = new DataLayout(&M);
  this->Print = M.getFunction("printf");
}

void Transform::AddPointerAnalysis(std::map<Pointer, CCuredPointerType> Qs, ValueToValueMapTy &VMap){
  // Qs map maps (original pointers, level) to CCuredPointerType
  // VMap maps (original pointers) to (duplicated pointer)
  for(auto Q: Qs){
    Pointer Key = Q.first;
    Pointer NewKey = Pointer(VMap[Key.id], Key.level);
    this->Qs[NewKey] = Qs[Key];
  }
}

void Transform::Apply(){
  RecreateGeps();

  TransformExternalFunctionCalls();

  TransformPointerAllocas();
  TransformPointerStores();
  TransformPointerLoads();
  TransformArrayAllocas();
  TransformArrayGeps();

  TransformFunctionCalls();
  TransformReturns();
}

void Transform::TransformPointerAllocas(){
  for(auto PointerAlloc : Instructions->PointerAllocas){
    BasicBlock::iterator iter = PointerAlloc;
    iter++;
    IRBuilder<> B(iter);

    Type *PointerTy = PointerAlloc->getType()->getPointerElementType();

    // Construct the type for the fat pointer
    StructType *FatPointerType = FatPointers::GetFatPointerType(PointerTy);

    Value* FatPointer = B.CreateAlloca(FatPointerType, NULL, "fp" + PointerAlloc->getName());
    PointerAlloc->replaceAllUsesWith(FatPointer);
    FPToRawVariableMap[FatPointer] = PointerAlloc;
    // All code below can use the original PointerAlloc

    // Initialize Value to current value and everything else to zero
    std::vector<Value *> ValueIdx = GetIndices(0, PointerAlloc->getContext());
    std::vector<Value *> BaseIdx = GetIndices(1, PointerAlloc->getContext());
    std::vector<Value *> LengthIdx = GetIndices(2, PointerAlloc->getContext());
    Value *FatPointerValue = B.CreateGEP(FatPointer, ValueIdx); 
    Value *FatPointerBase = B.CreateGEP(FatPointer, BaseIdx); 
    Value *FatPointerBound = B.CreateGEP(FatPointer, LengthIdx); 

    // Set the base, value and bound to NULL
    Value *Null = ConstantPointerNull::get(cast<PointerType>(FatPointerType->getElementType(0)));
    B.CreateStore(Null, FatPointerValue);
    B.CreateStore(Null, FatPointerBase);
    B.CreateStore(Null, FatPointerBound);
  }
}
void TransformBitCast(BitCastInst *BC){
  BasicBlock::iterator iter = BC;
  iter++;
  IRBuilder<> B(iter);

  Type *InnerType = BC->getType();
  while(CountPointerLevels(InnerType) > 1)
    InnerType = InnerType->getPointerElementType();

  // Create the innermost fat pointer
  StructType *FatPointerType = FatPointers::GetFatPointerType(InnerType);
  Value *FatPointer = B.CreateAlloca(FatPointerType);
  Value *Null = ConstantPointerNull::get(cast<PointerType>(InnerType));
  // The innermost pointer is unset
  Value *NewBitCast = B.CreateBitCast(BC->getOperand(0), InnerType);
  StoreInFatPointerValue(FatPointer, NewBitCast, B);
  StoreInFatPointerBase(FatPointer, Null, B);
  StoreInFatPointerBound(FatPointer, Null, B);

  // Create the outer fat pointers
  Value *PrevFatPointer = FatPointer;
  Type *WrapperType = InnerType;
  Type *IntegerType = IntegerType::getInt64Ty(BC->getContext());
  Value *FatPointerSize = ConstantInt::get(IntegerType, 24);
  while(WrapperType != BC->getType()){
    WrapperType = WrapperType->getPointerTo();
    StructType *FatPointerType = FatPointers::GetFatPointerType(WrapperType);
    Value *FatPointer = B.CreateAlloca(FatPointerType);
    // The innermost pointer is unset
    StoreInFatPointerValue(FatPointer, PrevFatPointer, B);
    StoreInFatPointerBase(FatPointer, PrevFatPointer, B);
    Value *Bound = B.CreateAdd(
        FatPointerSize, B.CreatePtrToInt(PrevFatPointer, IntegerType));
    StoreInFatPointerBound(FatPointer, B.CreateIntToPtr(Bound, WrapperType), B);

    PrevFatPointer = FatPointer;
  }

  BC->replaceAllUsesWith(PrevFatPointer);
  errs() << "Need to transform:\n";
  errs() << *BC << "\n";
  errs() << "Pointer Levels: " << CountPointerLevels(BC->getType()) << "\n";
}
void Transform::TransformPointerStores(){
  for(auto PointerStore : Instructions->PointerStores){
    errs() << "PointerStore:\n";
    errs() << GetOriginator(PointerStore->getPointerOperand()).ToString() << "\t";
    errs() << GetOriginator(PointerStore->getValueOperand(), -1).ToString() << "\n";
    IRBuilder<> B(PointerStore);

    Value* FatPointer = PointerStore->getPointerOperand(); 
    Value *Address = PointerStore->getValueOperand();

    // Check if there are any bitcasts that need redoing
    Value *T = Address;
    BitCastInst *Bitcast = dyn_cast<BitCastInst>(T);
    while(GetLinkType(T) != NO_LINK){
      T = GetNextLink(T);
      if(isa<BitCastInst>(T))
        Bitcast = cast<BitCastInst>(T);
    }
    if(Bitcast)
      TransformBitCast(Bitcast);

    // The types may match up - for example in the case of fat pointer parameters
    if(Address->getType() == FatPointer->getType()->getPointerElementType())
      continue;

    Value* RawPointer = 
        B.CreateGEP(FatPointer, GetIndices(0, PointerStore->getContext()));
    Instruction *NewStore = B.CreateStore(Address, RawPointer);
    errs() << *NewStore << "\n";

    B.SetInsertPoint(NewStore);

    SetBoundsOnMalloc(B, PointerStore);
    SetBoundsOnExternalFunctionCall(B, PointerStore);
    SetBoundsOnConstString(B, PointerStore);
    
    PointerStore->eraseFromParent();
  }
}
void Transform::TransformPointerLoads(){
  for(auto PointerLoad : Instructions->PointerLoads){
    PointerDestination PD = GetDestination(PointerLoad);
    assert(PD != RETURN && "Not implemented correct level on function return");

    int startLevel = 0;
    if(PD == CALL) startLevel = -1;
    Pointer FPOrig = GetOriginator(PointerLoad, startLevel);

    if(FPToRawVariableMap.count(FPOrig.id)){
      Pointer Orig = Pointer(FPToRawVariableMap[FPOrig.id], FPOrig.level);
      errs() << "PointerLoad:\n";
      errs() << FPOrig.ToString() << " mapped to " << Orig.ToString();
      errs() << " " << Pretty(Qs[Orig]) << "\n";
    }

    IRBuilder<> B(PointerLoad);

    Value* FatPointer = PointerLoad->getPointerOperand(); 
    Value* RawPointer = 
        B.CreateGEP(FatPointer, GetIndices(0, PointerLoad->getContext()));

    Value* NewLoad = B.CreateLoad(RawPointer);

    bool BoundsChecking = false;
    if(BoundsChecking){
      Value* Base = B.CreateLoad(
          B.CreateGEP(FatPointer, GetIndices(1, PointerLoad->getContext())));
      Value* Bound = B.CreateLoad(
          B.CreateGEP(FatPointer, GetIndices(2, PointerLoad->getContext())));

      Type* VoidPtrType = Type::getInt8Ty(PointerLoad->getContext())->getPointerTo();
      // If the load is going to be used in a Gep, use the address from the
      // Gep in the bounds check
      if(auto Gep = dyn_cast<GetElementPtrInst>(PointerLoad->use_back())){
        BasicBlock::iterator iter = Gep;
        iter++;

        IRBuilder<> B(iter);
        FatPointers::CreateBoundsCheck(B, 
            B.CreatePointerCast(Gep, VoidPtrType), 
            B.CreatePointerCast(Base, VoidPtrType), 
            B.CreatePointerCast(Bound, VoidPtrType), Print, M);
      } else {
        FatPointers::CreateBoundsCheck(B, 
            B.CreatePointerCast(B.CreateLoad(RawPointer), VoidPtrType), 
            B.CreatePointerCast(Base, VoidPtrType),
            B.CreatePointerCast(Bound, VoidPtrType), Print, M);
      }
    }

    SetBoundsOnFree(PointerLoad->use_back(), FatPointer);

    PointerLoad->replaceAllUsesWith(NewLoad);
    PointerLoad->eraseFromParent();
  }

  // This is nessecary to get future typing correct
  for(auto PointerLoad : Instructions->PointerLoadsForParameters){
    IRBuilder<> B(PointerLoad);
    Value* NewLoad = B.CreateLoad(PointerLoad->getPointerOperand());
    PointerLoad->replaceAllUsesWith(NewLoad);
    PointerLoad->eraseFromParent();
  }
  for(auto PointerLoad : Instructions->PointerLoadsForReturn){
    IRBuilder<> B(PointerLoad);
    Value* NewLoad = B.CreateLoad(PointerLoad->getPointerOperand());
    PointerLoad->replaceAllUsesWith(NewLoad);
    PointerLoad->eraseFromParent();
  }
  for(auto PointerLoad : Instructions->PointerLoadsForPointerEquals){
    IRBuilder<> B(PointerLoad);
    Value* NewLoad = B.CreateLoad(PointerLoad->getPointerOperand());
    PointerLoad->replaceAllUsesWith(NewLoad);
    PointerLoad->eraseFromParent();
  }
}
void Transform::TransformReturns(){
  // This will only be called on returns that return a value
  for(auto Return: Instructions->Returns){
    IRBuilder<> B(Return);
    Value *NewReturn = B.CreateRet(Return->getReturnValue());
    Return->eraseFromParent();
  }
}
void Transform::TransformArrayAllocas(){
}
void Transform::TransformArrayGeps(){
  for(auto Gep : Instructions->ArrayGeps){
    LLVMContext *C = &Gep->getContext();
    IRBuilder<> B(Gep);
    BasicBlock::iterator iter(Gep);

    // Create Basic Blocks
    BasicBlock *BeforeCheck = Gep->getParent();
    BasicBlock *AfterCheck = BeforeCheck->splitBasicBlock(iter);
    BasicBlock *FailedCheck = BasicBlock::Create(*C,
        "BoundsCheckFailed", BeforeCheck->getParent());

    removeTerminator(BeforeCheck);

    B.SetInsertPoint(BeforeCheck);

    // Create check
    Value *Passing = NULL;
    Type *IntType = Type::getInt64Ty(*C);
    auto CurrentGep = Gep;
    while(true){
      Type *ArrayType = Gep->getPointerOperand()->getType()->getPointerElementType();
      Constant *Len = ConstantInt::get(IntType, ArrayType->getVectorNumElements());
      
      auto Idx = Gep->idx_begin();
      Idx++;
      Value *Offset = *Idx;

      Value *Test= B.CreateICmpULT(Offset, Len);
      Passing = (Passing == NULL ? Test : B.CreateAnd(Test, Passing));

      if(auto NextGep = dyn_cast<GetElementPtrInst>(Gep->use_back()))
        Gep = NextGep;
      else
        break;
    }

    B.CreateCondBr(Passing, AfterCheck, FailedCheck);

    B.SetInsertPoint(FailedCheck);
    B.CreateCall(Print, Str(B, "OutOfBounds"));
    B.CreateBr(AfterCheck);

    B.SetInsertPoint(AfterCheck);
  }
}
void Transform::SetBoundsOnConstString(IRBuilder<> &B, StoreInst *PointerStore){
  // In this case, the Address will be a GEP instruction
  Value *Address = PointerStore->getValueOperand();
  auto *Gep = dyn_cast<GEPOperator>(Address);
  if(!Gep)
    return;

  if(!Gep->getPointerOperand()->getType()->getPointerElementType()->isArrayTy())
    return;

  Type *IntegerType = IntegerType::getInt64Ty(PointerStore->getContext());
  Value* FatPointer = PointerStore->getPointerOperand(); 
  Value* FatPointerBase = 
      B.CreateGEP(FatPointer, GetIndices(1, PointerStore->getContext()));
  Value* FatPointerBound = 
      B.CreateGEP(FatPointer, GetIndices(2, PointerStore->getContext()));

  B.CreateStore(Address, FatPointerBase);

  int NumElementsInArray = Gep->getPointerOperand()->getType()->getPointerElementType()->getVectorNumElements();
  Constant *ArrayLength = ConstantInt::get(IntegerType, NumElementsInArray);
  Value *Bound = B.CreateAdd(ArrayLength, B.CreatePtrToInt(Address, IntegerType));

  B.CreateStore(B.CreateIntToPtr(Bound, Address->getType()), FatPointerBound);
}
void Transform::SetBoundsOnMalloc(IRBuilder<> &B, StoreInst *PointerStore){
  Value *Prev = PointerStore->getValueOperand();

  // Follow through a cast if there is one
  if(auto BC = dyn_cast<BitCastInst>(Prev))
    Prev = BC->getOperand(0);

  auto Call = dyn_cast<CallInst>(Prev);
  if(!Call)
    return;

  if(Call->getCalledFunction()->getName() != "malloc")
    return;

  Value* FatPointer = PointerStore->getPointerOperand(); 
  Value *Address = PointerStore->getValueOperand();

  Value* FatPointerBase = 
      B.CreateGEP(FatPointer, GetIndices(1, PointerStore->getContext()));
  B.CreateStore(Address, FatPointerBase);

  Type *IntegerType = IntegerType::getInt64Ty(PointerStore->getContext());
  Value *Size = Call->getArgOperand(0);

  Value *Bound = B.CreateAdd(
      B.CreateTruncOrBitCast(Size, IntegerType),
      B.CreatePtrToInt(Address, IntegerType));

  Value* FatPointerBound = 
      B.CreateGEP(FatPointer, GetIndices(2, PointerStore->getContext()));

  B.CreateStore(B.CreateIntToPtr(Bound, Address->getType()), FatPointerBound);
}
void Transform::SetBoundsOnExternalFunctionCall(IRBuilder<> &B, StoreInst *PointerStore){
  Value *Prev = PointerStore->getValueOperand();

  // Follow through a cast if there is one
  if(auto BC = dyn_cast<BitCastInst>(Prev))
    Prev = BC->getOperand(0);

  auto Call = dyn_cast<CallInst>(Prev);
  if(!Call)
    return;

  if(Call->getCalledFunction()->getName() == "malloc")
    return;

  Value* FatPointer = PointerStore->getPointerOperand(); 
  Value *Address = PointerStore->getValueOperand();

  Value* FatPointerBase = 
      B.CreateGEP(FatPointer, GetIndices(1, PointerStore->getContext()));

  Type *IntegerType = IntegerType::getInt64Ty(PointerStore->getContext());
  B.CreateStore(B.CreateIntToPtr(ConstantInt::get(IntegerType, 0), Address->getType()), FatPointerBase);
}
void Transform::SetBoundsOnFree(Instruction *Next, Value *FatPointer){
  // Follow through the cast if there is one
  if(auto BC = dyn_cast<BitCastInst>(Next))
    Next = Next->use_back();

  auto Call = dyn_cast<CallInst>(Next);
  if(!Call)
    return;
  if(Call->getCalledFunction()->getName() != "free")
    return;

  // The fat pointer has been freed, so set the Bound equal to the Base
  BasicBlock::iterator iter = Call;
  iter++;

  IRBuilder<> B(iter); // Insert after the free

  Value* BaseAddr = B.CreateGEP(FatPointer, GetIndices(1, Next->getContext()));
  Value* BoundAddr = B.CreateGEP(FatPointer, GetIndices(2, Next->getContext()));
  B.CreateStore(B.CreateLoad(BaseAddr), BoundAddr);
}
void Transform::TransformFunctionCalls(){
  for(auto Call : Instructions->Calls){
    // Switch this function call to call the Fat Pointer version
    // -- Collect the parameters
    std::vector<Value *> Args;
    for(int i=0; i<Call->getNumArgOperands(); i++){
      Args.push_back(Call->getArgOperand(i));
    }

    // -- Create the new function call
    CallInst *NewCall = CallInst::Create(RawToFPFunctionMap[Call->getCalledFunction()], Args, "", Call);

    // -- Replace the old call with the new one
    Call->replaceAllUsesWith(NewCall);
    Call->eraseFromParent();
  }
}
void Transform::TransformExternalFunctionCalls(){
  for(auto Call : Instructions->ExternalCalls){
    if(!Call->getType()->isPointerTy())
      continue;
    errs() << *Call << "\n";
  }
}

void Transform::RecreateGeps(){
  // This needs to be before TransformPointerLoads
  for(auto Gep: Instructions->GepsToRecreate){
    IRBuilder<> B(Gep);

    std::vector<Value *> Indices;
    for(auto I=Gep->idx_begin(), E=Gep->idx_end(); I != E; ++I)
      Indices.push_back(*I);

    Value *NewGep = B.CreateGEP(Gep->getPointerOperand(), Indices);
    Gep->replaceAllUsesWith(NewGep);
    Gep->eraseFromParent();
  }
}
