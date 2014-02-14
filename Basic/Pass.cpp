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
    std::set<Instruction *> GetElementPtrs = CollectGetElementPtrs(M);

    //PrintIrWithHighlight(M, ArrayAllocs, GetElementPtrs);
    //DisplayArrayInformation(ArrayAllocs);
    //DisplayGepInformation(GetElementPtrs);
    ModifyArrayAllocs(ArrayAllocs);
    ModifyGeps(GetElementPtrs);

    return true;
  }
private:
  DataLayout *DL = NULL;
  Function *Print = NULL;

  std::set<Instruction *> CollectArrayAllocs(Module &M);
  std::set<Instruction *> CollectGetElementPtrs(Module &M);

  void DisplayArrayInformation(std::set<Instruction *> ArrayAllocs);
  void DisplayGepInformation(std::set<Instruction *> GetElementPtrs);

  void ModifyGeps(std::set<Instruction *> GetElementPtrs);
  void ModifyArrayAllocs(std::set<Instruction *> ArrayAllocs);

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

    Type *IntegerType = IntegerType::getInt32Ty(Gep->getContext());
    Value *InLowerBound = B.CreateICmpUGE(
        B.CreatePtrToInt(NewGep, IntegerType),
        B.CreatePtrToInt(Base, IntegerType) 
        );
    Value *InHigherBound = B.CreateICmpULT(
        B.CreatePtrToInt(NewGep, IntegerType),
        B.CreatePtrToInt(Bound, IntegerType)
        );
        
    Instruction *InBounds = cast<Instruction>(B.CreateAnd(InLowerBound, InHigherBound));

    iter = BasicBlock::iterator(InBounds);
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
