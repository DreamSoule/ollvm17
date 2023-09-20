// LLVM libs
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Analysis/CFG.h"
// User libs
#include "ObfuscationOptions.h"
#include "IPObfuscationContext.h"
#include "compat/CallSite.h"
#include "CryptoUtils.h"
#include "Utils.h"
// System libs
#include <random>

namespace llvm{
    class IndirectCallPass : public PassInfoMixin<IndirectCallPass>{ 
        public:
            bool flag;
            std::vector<CallInst *> CallSites;
            IPObfuscationContext *IPO;
            ObfuscationOptions *Options;
            std::vector<Function *> Callees;
            std::map<Function *, unsigned> CalleeNumbering;
            CryptoUtils RandomEngine;
            IndirectCallPass(bool flag){
                this->flag = flag;
                this->IPO = new IPObfuscationContext;
                this->Options = new ObfuscationOptions;
            } // 携带flag的构造函数
            bool doIndirctCall(Function &F);
            PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
            GlobalVariable *getIndirectCallees(Function &F, ConstantInt *EncKey);
            void NumberCallees(Function &F);
            static bool isRequired() { return true; }
    };
    IndirectCallPass *createIndirectCall(bool flag);
}