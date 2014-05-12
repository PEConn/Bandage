#include <fstream>

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

cl::opt<std::string> OutputFilename("funcfile", cl::desc("Specify output filename for function list"), cl::value_desc("filename for function list"));

struct ListFuncs: public FunctionPass {
  static char ID;
  ListFuncs() : FunctionPass(ID) {}

  std::ofstream File;

  virtual bool doInitialization(Module &M){
    if(OutputFilename != "")
      File.open(OutputFilename, std::ios::app);
    return false;
  }
  virtual bool doFinalization(Module &M){
    if(OutputFilename != "")
      File.close();
    return false;
  }

  virtual bool runOnFunction(Function &F) {
    File << F.getName().str() << "\n";
    return false;
  }
};
}

char ListFuncs::ID = 0;
static RegisterPass<ListFuncs> X("listfuncs", "List Functions Pass", false, false);
