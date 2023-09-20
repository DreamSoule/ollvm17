#ifndef LLVM_UTILS_H
#define LLVM_UTILS_H
// LLVM libs
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
// System libs
#include <map>
#include <set>
#include <vector>
#include <stdio.h>
#include <sstream>
// 常用宏定义
#define INIT_CONTEXT(F) CONTEXT=&F.getContext()
#define TYPE_I32 Type::getInt32Ty(*CONTEXT)
#define CONST_I32(V) ConstantInt::get(TYPE_I32, V, false)
#define CONST(T, V) ConstantInt::get(T, V)
extern llvm::LLVMContext *CONTEXT;
using namespace std;
namespace llvm{
    std::string readAnnotate(Function *f); // 读取llvm.global.annotations中的annotation值
    bool toObfuscate(bool flag, llvm::Function *f, std::string const &attribute); // 判断是否开启混淆
    void fixStack(Function &F); // 修复PHI指令和逃逸变量
    void FixBasicBlockConstantExpr(BasicBlock *BB);
    void FixFunctionConstantExpr(Function *Func);
    string rand_str(int len);
    void LowerConstantExpr(Function &F);
    }
#endif // LLVM_UTILS_H