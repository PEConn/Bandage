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
#include "llvm/Support/CommandLine.h"
#include "llvm/Analysis/FindUsedTypes.h"

#include "Helpers.hpp"
#include "FunctionDuplicater.hpp"
#include "TypeDuplicater.hpp"
#include "InstructionCollection.hpp"
#include "Transform.hpp"
#include "FatPointers.hpp"
#include "PointerUse.hpp"
#include "PointerAllocaTransform.hpp"
#include "PointerUseCollection.hpp"
#include "PointerUseTransform.hpp"
#include "ArrayAccessTransform.hpp"

#include "../PointerAnalysis/Pass.hpp"

using namespace llvm;

namespace {

cl::opt<bool> DontUseCCured("bandage-no-ccured", cl::desc("Suppresses the use of CCured analysis for the Bandage fat pointer transformation"));
cl::opt<bool> DontInlineChecks("bandage-no-inline", cl::desc("Doesn't inline the bounds or null checks given by bandage"));

struct Bandage : public ModulePass{
  static char ID;
  Bandage() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    auto FT = FunctionType::get(Type::getVoidTy(M.getContext()), false);
    Function *YellOutOfBounds = Function::Create(FT, 
        GlobalValue::LinkageTypes::ExternalLinkage, "OnError", &M);
    BasicBlock *BB = BasicBlock::Create(M.getContext(), "OnError", YellOutOfBounds);

    IRBuilder<> B(BB);
    Function *Print = M.getFunction("printf");
    if(Print)
      B.CreateCall(Print, Str(B, "OutOfBounds"));
    B.CreateRetVoid();

    FatPointers::Inline = !DontInlineChecks;

    errs() << "-------------------------------" << "\n";
    errs() << "Fat Pointer Transformation Pass" << "\n";
    errs() << "-------------------------------" << "\n";
    errs() << "Duplicating Types\n";
    auto *TD = new TypeDuplicater(M, &getAnalysis<FindUsedTypes>());
    errs() << "Duplicating Functions\n";
    auto *FD = new FunctionDuplicater(M, TD);
    errs() << "Collecting Pointer Uses\n";
    auto *PUC = new PointerUseCollection(FD, M);
    errs() << "Transforming Pointer Allocations\n";
    auto *PAT = new PointerAllocaTransform(FD->GetFPFunctions());
    errs() << "Transform Pointer Uses\n";
    errs() << "  Checks Inlined: " << FatPointers::Inline << "\n";
    auto *T = new PointerUseTransform(PUC, M, FD->RawToFPMap, PAT->RawToFPMap);
    errs() << "  Using CCured Analysis: " << !DontUseCCured << "\n";
    if(!DontUseCCured){
      auto PA = &getAnalysis<PointerAnalysis>();
      T->AddPointerAnalysis(PA->Qs, FD->VMap);
    }
    T->Apply();
    errs() << "  Stats:\n";
    errs() << "    Safe Loads:    " << T->SafeLoads << "\n";
    errs() << "    Checked Loads: " << T->SafeLoads << "\n";
    errs() << "-------------------------------" << "\n";

    auto AAT = new ArrayAccessTransform(FD->GetFPFunctions(), YellOutOfBounds);

    delete AAT;
    delete T;
    delete PAT;
    delete PUC;
    delete FD;
    delete TD;

    return true;
  }
  virtual void getAnalysisUsage(AnalysisUsage &AU) const { 
    AU.addRequired<PointerAnalysis>();
    AU.addRequired<FindUsedTypes>();
  }
};
}


char Bandage::ID = 0;
static RegisterPass<Bandage> Y("bandage", "Bandage Pass", false, false);
