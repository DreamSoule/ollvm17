#ifndef _BOGUSCONTROLFLOW_H_
#define _BOGUSCONTROLFLOW_H_
// LLVM libs
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/Local.h"
// System libs
#include <list>
#include <memory>
// User libs
#include "CryptoUtils.h"
#include "Utils.h"
using namespace std;
using namespace llvm;
namespace llvm{ // 基本块分割
    class BogusControlFlowPass : public PassInfoMixin<BogusControlFlowPass>{
        public:
            bool flag;
            BogusControlFlowPass(bool flag){
                this->flag = flag;
            } // 携带flag的构造函数
            PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM); // Pass实现函数
            void bogus(Function &F);
            void addBogusFlow(BasicBlock *basicBlock, Function &F);
            bool doF(Module &M, Function &F);
            static bool isRequired() { return true; } // 直接返回true即可
    };
    BogusControlFlowPass *createBogusControlFlow(bool flag); // 创建基本块分割
}

#endif