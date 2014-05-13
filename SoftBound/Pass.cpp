#include <set>
#include <map>
#include <algorithm>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/CommandLine.h"

#include "LocalBounds.hpp"
#include "BoundsSetter.hpp"
#include "HeapBounds.hpp"
#include "BoundsChecks.hpp"
#include "FunctionDuplicater.hpp"
#include "CallModifier.hpp"
#include "../Basic/ArrayAccessTransform.hpp"
#include "../Basic/Helpers.hpp"

using namespace llvm;

namespace {

cl::opt<std::string> FuncFile("funcfile", cl::desc("Specify input filename for function list"), cl::value_desc("filename for function list"));

struct SoftBound : public ModulePass{
  static char ID;
  SoftBound() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) {
    auto FD = new FunctionDuplicater(M, FuncFile);
    auto BS = new BoundsSetter(FD->FPFunctions);
    auto LB = new LocalBounds(FD);
    auto HB = new HeapBounds(M);
    auto BC = new BoundsChecks(LB, HB, FD);
    BC->CreateBoundsCheckFunction(M, M.getFunction("printf"));
    BC->CreateBoundsChecks();
    auto CM = new CallModifier(FD, LB);

    BS->SetBounds(LB, HB);

    auto AAT = new ArrayAccessTransform(FD->FPFunctions, CreatePrintFunction(M));

    delete AAT;
    delete BC;
    delete CM;
    delete LB;
    delete FD;
    return true;
  }
  virtual void getAnalysisUsage(AnalysisUsage &AU) const { 
    //AU.addRequired<PointerAnalysis>();
  }
};
}

char SoftBound::ID = 0;
static RegisterPass<SoftBound> Y("softbound", "SoftBound Pass", false, false);
