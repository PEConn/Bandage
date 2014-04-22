#ifndef POINTER_HPP
#define POINTER_HPP

#include <set>
#include <string>
#include "llvm/IR/Instructions.h"

using namespace llvm;

enum CCuredPointerType {UNSET, SAFE, SEQ, DYNQ};
std::string Pretty(enum CCuredPointerType PT);

class Pointer{
public:
  Value *id;
  int level;

  Pointer(){}
  Pointer(Value *id, int level);
  std::string ToString() const;

  bool operator<(const Pointer &Other) const{
    
    if(id < Other.id){
      return true;
    } else if(id == Other.id){
      if(level < Other.level){
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
};

#endif
