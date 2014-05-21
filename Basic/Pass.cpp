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
cl::opt<std::string> FuncFile("funcfile", cl::desc("Specify input filename for function list"), cl::value_desc("filename for function list"));

struct Bandage : public ModulePass{
  static char ID;
  Bandage() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    FatPointers::Inline = !DontInlineChecks;
    FatPointers::Declare = (M.getFunction("main") != NULL);

    errs() << "-------------------------------" << "\n";
    errs() << "Fat Pointer Transformation Pass" << "\n";
    errs() << "-------------------------------" << "\n";
    errs() << "Duplicating Types\n";
    auto *TD = new TypeDuplicater(M, &getAnalysis<FindUsedTypes>(), FuncFile);
    errs() << "Duplicating Functions\n";
    auto *FD = new FunctionDuplicater(M, TD, FuncFile);
    errs() << "Has main: " << FatPointers::Declare << "\n";
    Function *OnError = CreatePrintFunction(M);
    if(FatPointers::Declare){
      // Force creation of the bounds check function
      Type *T = Type::getInt8PtrTy(M.getContext());
      FatPointers::CreateBoundsCheckFunction(T, M.getFunction("printf"), &M);

      // If main takes argv**, put it into a fat pointer
      /*
      Note: This interacts with future transformations
      Function *F = M.getFunction("main");
      auto Arg = F->arg_begin();
      auto ArgEnd = F->arg_end();
      if(Arg != ArgEnd){
        Arg++;
        if(Arg != ArgEnd){
          errs() << "Packing argv\n";
          IRBuilder<> B(&F->front().front());
          Value *ArgV = Arg;
          Value *FP = FatPointers::CreateFatPointer(ArgV->getType(), B);
          Value *IFP = FatPointers::CreateFatPointer(ArgV->getType()->getPointerElementType(), B);

          ArgV->replaceAllUsesWith(FP);

          Value *IFPVal = GetFatPointerValueAddr(IFP, B);
          B.CreateStore(ArgV, IFPVal);
          Value *FPVal = GetFatPointerValueAddr(FP, B);
          B.CreateStore(B.CreateLoad(IFP), FPVal);
        }
      }
      */

    }
    errs() << "Collecting Pointer Uses\n";
    auto *PUC = new PointerUseCollection(FD, M);
    errs() << "Transforming Pointer Allocations\n";
    auto *PAT = new PointerAllocaTransform(FD->GetFPFunctions(), M);
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
    errs() << "    Checked Loads: " << T->NoneSafeLoads << "\n";
    errs() << "-------------------------------" << "\n";

    auto AAT = new ArrayAccessTransform(FD->GetFPFunctions(), OnError);

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
