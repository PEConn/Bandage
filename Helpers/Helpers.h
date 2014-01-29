#ifndef HELPERS
#define HELPERS

#include <set>
#include "llvm/IR/Instructions.h"

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

#endif
