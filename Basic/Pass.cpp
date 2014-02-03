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

#include "FatPointer.h"
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
    //ModifyGeps(GetElementPtrs);
    //ModifyArrayAllocs(ArrayAllocs);
    TurnToFatPointers(ArrayAllocs, GetElementPtrs);

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

  void TurnToFatPointers(std::set<Instruction *> ArrayAllocs,
      std::set<Instruction *> GetElementPtrs);
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
    //errs() << "No of Elements:\t\t" << (cast<ConstantInt>(ArrayAlloc->getArraySize()))->getSExtValue() << "\n";

    Type *ArrayType = ArrayAlloc->getAllocatedType();
    errs() << "No of Elements:\t\t" << ArrayType->getVectorNumElements() << "\n";
    Type *ElementType = ArrayType->getVectorElementType();
    //errs() << "Element Size(p):\t" << ElementType->getPrimitiveSizeInBits() << "\n";
    //errs() << "Element Size(s):\t" << ElementType->getScalarSizeInBits() << "\n";

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
    errs() << "Instruction:\t" << *ArrayAlloc << "\n";

    // Print the array address
    BasicBlock::iterator iter = ArrayAlloc;
    iter++;
    IRBuilder<> B(iter);

    Value *PrintString = B.CreateGlobalStringPtr(StringRef(blue + "Arr: %p\n" + reset), 
        "PrintString");
    B.CreateCall2(Print, PrintString, ArrayAlloc, "DebugPrint"); 

    // Print the array length
    errs() << "Length:\t" << GetNumElementsInArray(ArrayAlloc) << "\n";
    errs() << "Elem Size:\t" << GetArrayElementSizeInBits(ArrayAlloc, DL) << "\n";

  }
}

void Bandage::ModifyGeps(std::set<Instruction *> GetElementPtrs){
  for(auto I: GetElementPtrs){
    auto Gep = cast<GetElementPtrInst>(I);
    errs() << "Instruction:\t" << *Gep << "\n";

    BasicBlock::iterator iter = Gep;
    iter++;
    IRBuilder<> B(iter);

    Value *PrintString = B.CreateGlobalStringPtr(StringRef(blue + "GEP: %p\n" + reset), 
        "PrintString");
    B.CreateCall2(Print, PrintString, Gep, "DebugPrint"); 
  }
}

void Bandage::TurnToFatPointers(std::set<Instruction *> ArrayAllocs,
    std::set<Instruction *> GetElementPtrs){
  for(auto I: ArrayAllocs){
    auto ArrayAlloc = cast<AllocaInst>(I);
    BasicBlock::iterator iter = ArrayAlloc;
    iter++;
    IRBuilder<> B(iter);

    FatPointer *FPF = new FatPointer(&B, ArrayAlloc->getContext());

    Value *FP = FPF->createFatPointer();
    FPF->setFatPointerValue(FP, ArrayAlloc);
    FPF->setFatPointerBase(FP, ArrayAlloc);

    Value *ArrayLength = FPF->getConstInteger(GetArrayElementSizeInBits(ArrayAlloc, DL));
    FPF->setFatPointerLength(FP, ArrayLength);

    // Sanity check
    Value *PrintString = B.CreateGlobalStringPtr(StringRef(blue + "DBG: %p\n" + reset));
    B.CreateCall2(Print, PrintString, FPF->getFatPointerValue(FP)); 
    B.CreateCall2(Print, PrintString, FPF->getFatPointerBase(FP)); 
    B.CreateCall2(Print, PrintString, FPF->getFatPointerLength(FP)); 

	  //ArrayAlloc->replaceAllUsesWith(FP);
  }

  for(auto I: GetElementPtrs){
    auto Gep = cast<GetElementPtrInst>(I);
    IRBuilder<> B(Gep);

    FatPointer *FPF = new FatPointer(&B, Gep->getContext());
    Value *FP = Gep->getPointerOperand();

    Value *Pointer= FPF->getFatPointerValue(FP);

    // Get the access indicies
    std::vector<Value *> IdxList;
    for(auto Idx = Gep->idx_begin(), EIdx = Gep->idx_end(); Idx != EIdx; ++Idx)
      IdxList.push_back(*Idx);
    
    errs() << *Gep << "\n";
    errs() << *Gep->getPointerOperandType() << "\n";
    errs() << *Pointer->getType() << "\n";
    Value *CastPointer = B.CreatePointerCast(Pointer, Gep->getPointerOperandType());
    Value* NewGep = B.CreateGEP(CastPointer, IdxList);
    errs() << *NewGep << "\n";

    BasicBlock::iterator iter(Gep);
    //ReplaceInstWithInst(Gep->getParent()->getInstList(), iter, NewGep);
  }
}
char Bandage::ID = 0;
static RegisterPass<Bandage> X("bandage", "Bandage Pass", false, false);
