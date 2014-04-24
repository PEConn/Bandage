#include <set>
#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

#include "Pointer.hpp"
#include "Constraints.hpp"

using namespace llvm;

struct PointerAnalysis : public ModulePass{
  static char ID;
  PointerAnalysis() : ModulePass(ID) {}
  ~PointerAnalysis();

  virtual bool runOnModule(Module &M) {
    CollectPointers(M);
    SolveConstraints();

    return false;
  }
  void CollectPointers(Module &M);
  void SolveConstraints();
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {}

  std::map<Pointer, CCuredPointerType> Qs;
private:
  std::set<Pointer> PointerUses;
  std::map<Function *, Pointer> FunctionReturns;
  std::map<std::pair<Function *, int>, Pointer> FunctionParameters;

  std::set<IsDynamic *> IDCons;
  std::set<SetToFunction *> STFCons;
  std::set<SetToPointer *> STPCons;
  std::set<PointerArithmetic *> PACons;
};
