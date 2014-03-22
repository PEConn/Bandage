#ifndef HELPERS
#define HELPERS

#include <set>
#include <vector>
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

const std::string red = "\033[31m";
const std::string green = "\033[32m";
const std::string yellow = "\033[33m";
const std::string blue = "\033[34m";
const std::string reset = "\033[0m";

void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1);
void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1,
    std::set<Instruction *> H2);
void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1,
    std::set<Instruction *> H2, std::set<Instruction *> H3);
void PrintIrWithHighlight(Module &M, std::set<Instruction *> H1,
    std::set<Instruction *> H2, std::set<Instruction *> H3,
    std::set<Instruction *> H4);

unsigned int GetNumElementsInArray(AllocaInst *);
unsigned int GetArrayElementSizeInBits(AllocaInst *, DataLayout *);

void removeTerminator(BasicBlock *BB);
std::vector<Value *> GetIndices(int val, LLVMContext& C);
Value* Str(IRBuilder<> B, std::string str);

#endif
