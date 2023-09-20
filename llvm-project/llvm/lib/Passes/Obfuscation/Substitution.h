//===- SubstitutionIncludes.h - Substitution Obfuscation pass-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains includes and defines for the substitution pass
//
//===----------------------------------------------------------------------===//

#ifndef _SUBSTITUTIONS_H_
#define _SUBSTITUTIONS_H_


// LLVM include
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/PassManager.h" //new Pass
#include "llvm/Transforms/IPO.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "CryptoUtils.h"

// Namespace
using namespace llvm;

#define NUMBER_ADD_SUBST 4
#define NUMBER_SUB_SUBST 3
#define NUMBER_AND_SUBST 2
#define NUMBER_OR_SUBST 2
#define NUMBER_XOR_SUBST 2

namespace llvm {
    class SubstitutionPass : public PassInfoMixin<SubstitutionPass> {
        public:
          bool flag;
          void (SubstitutionPass::*funcAdd[NUMBER_ADD_SUBST])(BinaryOperator *bo);
          void (SubstitutionPass::*funcSub[NUMBER_SUB_SUBST])(BinaryOperator *bo);
          void (SubstitutionPass::*funcAnd[NUMBER_AND_SUBST])(BinaryOperator *bo);
          void (SubstitutionPass::*funcOr[NUMBER_OR_SUBST])(BinaryOperator *bo);
          void (SubstitutionPass::*funcXor[NUMBER_XOR_SUBST])(BinaryOperator *bo);

          SubstitutionPass(bool flag) {
            this->flag = flag;
            funcAdd[0] = &SubstitutionPass::addNeg;
            funcAdd[1] = &SubstitutionPass::addDoubleNeg;
            funcAdd[2] = &SubstitutionPass::addRand;
            funcAdd[3] = &SubstitutionPass::addRand2;

            funcSub[0] = &SubstitutionPass::subNeg;
            funcSub[1] = &SubstitutionPass::subRand;
            funcSub[2] = &SubstitutionPass::subRand2;

            funcAnd[0] = &SubstitutionPass::andSubstitution;
            funcAnd[1] = &SubstitutionPass::andSubstitutionRand;

            funcOr[0] = &SubstitutionPass::orSubstitution;
            funcOr[1] = &SubstitutionPass::orSubstitutionRand;

            funcXor[0] = &SubstitutionPass::xorSubstitution;
            funcXor[1] = &SubstitutionPass::xorSubstitutionRand;
          }

          PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
          bool substitute(Function *f);

          void addNeg(BinaryOperator *bo);
          void addDoubleNeg(BinaryOperator *bo);
          void addRand(BinaryOperator *bo);
          void addRand2(BinaryOperator *bo);

          void subNeg(BinaryOperator *bo);
          void subRand(BinaryOperator *bo);
          void subRand2(BinaryOperator *bo);

          void andSubstitution(BinaryOperator *bo);
          void andSubstitutionRand(BinaryOperator *bo);

          void orSubstitution(BinaryOperator *bo);
          void orSubstitutionRand(BinaryOperator *bo);

          void xorSubstitution(BinaryOperator *bo);
          void xorSubstitutionRand(BinaryOperator *bo);

          static bool isRequired() { return true; } // 直接返回true即可
    };
    SubstitutionPass *createSubstitutionPass(bool flag); // 创建基本块分割
} // namespace llvm

#endif

