#include <set>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

#include "../Helpers/Helpers.h"

using namespace llvm;

namespace {
struct Bandage : public ModulePass{
  static char ID;
  Bandage() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    DL = new DataLayout(&M);
    Print = M.getFunction("printf");

    std::set<Instruction *> ArrayAllocs = CollectArrayAllocs(M);
    //std::set<Instruction *> GetElementPtrs = CollectGetElementPtrs(M);
    std::set<Instruction *> PointerAllocs = CollectPointerAllocs(M);
    std::set<Instruction *> PointerStores = CollectPointerStores(M);
    std::set<Instruction *> PointerLoads = CollectPointerLoads(M);

    std::set<Instruction *> ArrayGeps;
    for(auto Arr: ArrayAllocs){
      for(Value::use_iterator i = Arr->use_begin(), e = Arr->use_end(); i!=e; ++i){
        if(auto Gep = dyn_cast<GetElementPtrInst>(*i)){
          ArrayGeps.insert(Gep);
        }
      }
    }

    //PrintIrWithHighlight(M, PointerAllocs, PointerStores, PointerLoads);
    //DisplayArrayInformation(ArrayAllocs);
    //DisplayGepInformation(GetElementPtrs);
    ModifyArrayAllocs(ArrayAllocs);
    ModifyPointerAllocs(PointerAllocs);
    ModifyPointerStores(PointerStores);
    ModifyPointerLoads(PointerLoads);
    ModifyGeps(ArrayGeps);

    return true;
  }
private:
  DataLayout *DL = NULL;
  Function *Print = NULL;

  std::set<Instruction *> CollectArrayAllocs(Module &M);
  std::set<Instruction *> CollectGetElementPtrs(Module &M);
  std::set<Instruction *> CollectPointerStores(Module &M);
  std::set<Instruction *> CollectPointerLoads(Module &M);
  std::set<Instruction *> CollectPointerAllocs(Module &M);

  void DisplayArrayInformation(std::set<Instruction *> ArrayAllocs);
  void DisplayGepInformation(std::set<Instruction *> GetElementPtrs);

  void ModifyGeps(std::set<Instruction *> GetElementPtrs);
  void ModifyArrayAllocs(std::set<Instruction *> ArrayAllocs);
  void ModifyPointerAllocs(std::set<Instruction *> ArrayAllocs);
  void ModifyPointerStores(std::set<Instruction *> ArrayAllocs);
  void ModifyPointerLoads(std::set<Instruction *> ArrayAllocs);

  void CreateBoundsCheck(IRBuilder<> &B, Value *Val, Value *Base, Value *Bound);

  std::vector<Value *> GetIndices(int val, LLVMContext& C);
  Value* Str(IRBuilder<> B, std::string str);
  void AddPrint(IRBuilder<> B, std::string str, Value *v);
};
}

std::set<Instruction *> Bandage::CollectArrayAllocs(Module &M){
  std::set<Instruction *> ArrayAllocs;

  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      Instruction *I = &*II;
      if(AllocaInst *I = dyn_cast<AllocaInst>(&*II)){
        if(!II->getType()->isPointerTy())
          continue;
        if(!II->getType()->getPointerElementType()->isArrayTy())
          continue;
        ArrayAllocs.insert(I);
      }
    }
  }
  return ArrayAllocs;
}

std::set<Instruction *> Bandage::CollectGetElementPtrs(Module &M){
  std::set<Instruction *> GetElementPtrs;

  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      if(GetElementPtrInst *I = dyn_cast<GetElementPtrInst>(&*II)){
        GetElementPtrs.insert(I);
      }
    }
  }
  return GetElementPtrs;
}

std::set<Instruction *> Bandage::CollectPointerStores(Module &M){
  std::set<Instruction *> PointerStores;

  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      if(StoreInst *I = dyn_cast<StoreInst>(&*II)){
        if(!I->getPointerOperand()->getType()->getPointerElementType()->isPointerTy())
          continue;
        PointerStores.insert(I);
      }
    }
  }
  return PointerStores;
}
std::set<Instruction *> Bandage::CollectPointerLoads(Module &M){
  std::set<Instruction *> PointerLoads;

  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      if(LoadInst *I = dyn_cast<LoadInst>(&*II)){
        if(!I->getPointerOperand()->getType()->getPointerElementType()->isPointerTy())
          continue;
        PointerLoads.insert(I);
      }
    }
  }
  return PointerLoads;
}
std::set<Instruction *> Bandage::CollectPointerAllocs(Module &M){
  std::set<Instruction *> PointerAllocs;

  for(auto IF = M.begin(), EF = M.end(); IF != EF; ++IF){
    for(auto II = inst_begin(IF), EI = inst_end(IF); II != EI; ++II){
      Instruction *I = &*II;
      if(AllocaInst *I = dyn_cast<AllocaInst>(&*II)){
        if(!I->getType()->getPointerElementType()->isPointerTy())
          continue;
        PointerAllocs.insert(I);
      }
    }
  }
  return PointerAllocs;
}
void Bandage::DisplayArrayInformation(std::set<Instruction *> ArrayAllocs){
  for(auto I: ArrayAllocs){
    auto ArrayAlloc = cast<AllocaInst>(I);

    errs() << "Instruction:\t" << *ArrayAlloc << "\n";
    errs() << "Array Name:\t\t" << ArrayAlloc->getName() << "\n";

    Type *ArrayType = ArrayAlloc->getAllocatedType();
    errs() << "No of Elements:\t\t" << ArrayType->getVectorNumElements() << "\n";
    Type *ElementType = ArrayType->getVectorElementType();

    errs() << "Element Size(d):\t" << DL->getTypeAllocSize(ElementType) << "\n";
  }
}

void Bandage::DisplayGepInformation(std::set<Instruction *> GetElementPtrs){
  for(auto I: GetElementPtrs){
    auto Gep = cast<GetElementPtrInst>(I);
    errs() << "Instruction:\t" << *Gep << "\n";
  }
}

void Bandage::ModifyArrayAllocs(std::set<Instruction *> ArrayAllocs){
  for(auto I: ArrayAllocs){
    auto ArrayAlloc = cast<AllocaInst>(I);
    BasicBlock::iterator iter = ArrayAlloc;
    iter++;
    IRBuilder<> B(iter);

    Type *ArrayType = ArrayAlloc->getType();
    Type *IntegerType = IntegerType::getInt32Ty(ArrayAlloc->getContext());

    // Construct the type for the fat pointer
    std::vector<Type *> FatPointerMembers;	
    FatPointerMembers.push_back(ArrayType);
    FatPointerMembers.push_back(ArrayType);
    FatPointerMembers.push_back(ArrayType);
    Type *FatPointerType = StructType::create(FatPointerMembers, "struct.FatPointer");
    Value* FatPointer = B.CreateAlloca(FatPointerType, NULL, "FatPointer");
    ArrayAlloc->replaceAllUsesWith(FatPointer);
    // All code below can use the original ArrayAlloc

    // Initialize the values
    std::vector<Value *> ValueIdx = GetIndices(0, ArrayAlloc->getContext());
    std::vector<Value *> BaseIdx = GetIndices(1, ArrayAlloc->getContext());
    std::vector<Value *> LengthIdx = GetIndices(2, ArrayAlloc->getContext());
    Value *FatPointerValue = B.CreateGEP(FatPointer, ValueIdx); 
    Value *FatPointerBase = B.CreateGEP(FatPointer, BaseIdx); 
    Value *FatPointerBound = B.CreateGEP(FatPointer, LengthIdx); 

    Value *Address = ArrayAlloc;
    B.CreateStore(Address, FatPointerValue);
    B.CreateStore(Address, FatPointerBase);

    Constant *ArrayLength = ConstantInt::get(IntegerType, 
        GetNumElementsInArray(ArrayAlloc) * GetArrayElementSizeInBits(ArrayAlloc, DL)/8); 
  
    Value *Bound = B.CreateAdd(ArrayLength, B.CreatePtrToInt(Address, IntegerType));

    B.CreateStore(B.CreateIntToPtr(Bound, Address->getType()), FatPointerBound);
  }
}

void Bandage::ModifyPointerAllocs(std::set<Instruction *> PointerAllocs){
  for(auto I: PointerAllocs){
    auto PointerAlloc = cast<AllocaInst>(I);
    BasicBlock::iterator iter = PointerAlloc;
    iter++;
    IRBuilder<> B(iter);

    Type *PointerType = PointerAlloc->getType()->getPointerElementType();

    // Construct the type for the fat pointer
    std::vector<Type *> FatPointerMembers;	
    FatPointerMembers.push_back(PointerType);
    FatPointerMembers.push_back(PointerType);
    FatPointerMembers.push_back(PointerType);
    Type *FatPointerType = StructType::create(FatPointerMembers, "struct.FatPointer");

    Value* FatPointer = B.CreateAlloca(FatPointerType, NULL, "fp" + PointerAlloc->getName());
    PointerAlloc->replaceAllUsesWith(FatPointer);
    // All code below can use the original PointerAlloc

    // Initialize Value to current value and everything else to zero
    std::vector<Value *> ValueIdx = GetIndices(0, PointerAlloc->getContext());
    std::vector<Value *> BaseIdx = GetIndices(1, PointerAlloc->getContext());
    std::vector<Value *> LengthIdx = GetIndices(2, PointerAlloc->getContext());
    Value *FatPointerValue = B.CreateGEP(FatPointer, ValueIdx); 
    Value *FatPointerBase = B.CreateGEP(FatPointer, BaseIdx); 
    Value *FatPointerBound = B.CreateGEP(FatPointer, LengthIdx); 

    Value *Address = B.CreateLoad(PointerAlloc);
    B.CreateStore(Address, FatPointerValue);
    B.CreateStore(Address, FatPointerBase);
    B.CreateStore(Address, FatPointerBound);
  }
}
void Bandage::ModifyPointerStores(std::set<Instruction *> PointerStores){
  for(auto I: PointerStores){
    auto PointerStore = cast<StoreInst>(I);
    IRBuilder<> B(PointerStore);

    Value* FatPointer = PointerStore->getPointerOperand(); 
    Value* RawPointer = 
        B.CreateGEP(FatPointer, GetIndices(0, PointerStore->getContext()));

    Value *Address = PointerStore->getValueOperand();
    Instruction *NewStore = B.CreateStore(Address, RawPointer);

    B.SetInsertPoint(NewStore);

    // Check if this comes from a malloc like instruction
    Value *Prev = PointerStore->getValueOperand();

    // Follow through a cast if there is one
    if(auto BC = dyn_cast<BitCastInst>(Prev))
      Prev = BC->getOperand(0);

    if(auto Call = dyn_cast<CallInst>(Prev)){
      if(Call->getCalledFunction()->getName() == "malloc"){
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
    }
    
    PointerStore->eraseFromParent();
  }
}
void Bandage::ModifyPointerLoads(std::set<Instruction *> PointerLoads){
  for(auto I: PointerLoads){
    auto PointerLoad = cast<LoadInst>(I);
    IRBuilder<> B(PointerLoad);

    Value* FatPointer = PointerLoad->getPointerOperand(); 
    Value* RawPointer = 
        B.CreateGEP(FatPointer, GetIndices(0, PointerLoad->getContext()));
    Value* Base = B.CreateLoad(
        B.CreateGEP(FatPointer, GetIndices(1, PointerLoad->getContext())));
    Value* Bound = B.CreateLoad(
        B.CreateGEP(FatPointer, GetIndices(2, PointerLoad->getContext())));

    Value* NewLoad = B.CreateLoad(RawPointer);
    PointerLoad->replaceAllUsesWith(NewLoad);
    CreateBoundsCheck(B, B.CreateLoad(RawPointer), Base, Bound);
    PointerLoad->eraseFromParent();
  }
}
void Bandage::ModifyGeps(std::set<Instruction *> GetElementPtrs){
  for(auto I: GetElementPtrs){
    auto Gep = cast<GetElementPtrInst>(I);
    IRBuilder<> B(Gep);

    Value* FatPointer = Gep->getPointerOperand(); 
    Value* RawPointer = B.CreateLoad(
        B.CreateGEP(FatPointer, GetIndices(0, Gep->getContext())));

    std::vector<Value *> IdxList;
    for(auto Idx = Gep->idx_begin(), EIdx = Gep->idx_end(); Idx != EIdx; ++Idx)
      IdxList.push_back(*Idx); 

    Instruction *NewGep = GetElementPtrInst::Create(RawPointer, IdxList);

    BasicBlock::iterator iter(Gep);
    ReplaceInstWithInst(Gep->getParent()->getInstList(), iter, NewGep);

    // Now add checking after the Gep Instruction
    iter = BasicBlock::iterator(NewGep);
    iter++;
    B.SetInsertPoint(iter);

    Value *Base = B.CreateLoad(B.CreateInBoundsGEP(FatPointer, 
          GetIndices(1, Gep->getContext())));
    Value *Bound = B.CreateLoad(B.CreateInBoundsGEP(FatPointer, 
          GetIndices(2, Gep->getContext())));

    CreateBoundsCheck(B, NewGep, Base, Bound);
  }
}

void Bandage::CreateBoundsCheck(IRBuilder<> &B, Value *Val, Value *Base, Value *Bound){
  //AddPrint(B, "Value: %p", Val);
  //AddPrint(B, "Base:  %p", Base);
  //AddPrint(B, "Bound: %p", Bound);
  Type *IntegerType = IntegerType::getInt32Ty(Val->getContext());
  Value *InLowerBound = B.CreateICmpUGE(
      B.CreatePtrToInt(Val, IntegerType),
      B.CreatePtrToInt(Base, IntegerType) 
      );
  Value *InHigherBound = B.CreateICmpULT(
      B.CreatePtrToInt(Val, IntegerType),
      B.CreatePtrToInt(Bound, IntegerType)
      );
      
  Instruction *InBounds = cast<Instruction>(B.CreateAnd(InLowerBound, InHigherBound));

  BasicBlock::iterator iter = BasicBlock::iterator(InBounds);
  iter++;

  BasicBlock *BeforeBB = InBounds->getParent();
  BasicBlock *PassedBB = BeforeBB->splitBasicBlock(iter);

  removeTerminator(BeforeBB);
  BasicBlock *FailedBB = BasicBlock::Create(InBounds->getContext(),
      "BoundsCheckFailed", BeforeBB->getParent());

  B.SetInsertPoint(BeforeBB);
  B.CreateCondBr(InBounds, PassedBB, FailedBB);

  B.SetInsertPoint(FailedBB);
  B.CreateCall(Print, Str(B, "OutOfBounds"));
  B.CreateBr(PassedBB);
}

std::vector<Value *> Bandage::GetIndices(int val, LLVMContext& C){
  std::vector<Value *> Idxs;
  Idxs.push_back(ConstantInt::get(IntegerType::getInt32Ty(C), 0));
  Idxs.push_back(ConstantInt::get(IntegerType::getInt32Ty(C), val));
  return Idxs;
}

Value* Bandage::Str(IRBuilder<> B, std::string str){
  return B.CreateGlobalStringPtr(StringRef(blue + str + "\n" + reset));
}

void Bandage::AddPrint(IRBuilder<> B, std::string str, Value *v){
  B.CreateCall2(Print, Str(B, str), v);
}

char Bandage::ID = 0;
static RegisterPass<Bandage> X("bandage", "Bandage Pass", false, false);
