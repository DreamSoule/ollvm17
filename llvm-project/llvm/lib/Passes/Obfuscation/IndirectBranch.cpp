/*
LLVM Indirect Branching Pass
Copyright (C) 2017 Zhang(https://github.com/Naville/)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "IndirectBranch.h"
#include <random>

PreservedAnalyses IndirectBranchPass::run(Module &M, ModuleAnalysisManager &AM) {
  for (Function &Fn : M) {
    if (toObfuscate(flag, &Fn, "ibr")) {

      if (Options && Options->skipFunction(Fn.getName())) {
        continue;
      }

      if (Fn.empty() || Fn.hasLinkOnceLinkage() ||
          Fn.getSection() == ".text.startup") {
        continue;
      }

      LLVMContext &Ctx = Fn.getContext();

      // Init member fields
      BBNumbering.clear();
      BBTargets.clear();

      // llvm cannot split critical edge from IndirectBrInst
      SplitAllCriticalEdges(Fn, CriticalEdgeSplittingOptions(nullptr, nullptr));
      NumberBasicBlock(Fn);

      if (BBNumbering.empty()) {
        continue;
      }

      uint64_t V = RandomEngine.get_uint64_t();
      IntegerType *intType = Type::getInt32Ty(Ctx);
      unsigned pointerSize =
          Fn.getEntryBlock().getModule()->getDataLayout().getTypeAllocSize(
              PointerType::getUnqual(Fn.getContext())); // Soule
      if (pointerSize == 8) {
        intType = Type::getInt64Ty(Ctx);
      }
      ConstantInt *EncKey = ConstantInt::get(intType, V, false);
      ConstantInt *EncKey1 = ConstantInt::get(intType, -V, false);

      Value *MySecret = ConstantInt::get(intType, 0, true);

      ConstantInt *Zero = ConstantInt::get(intType, 0);
      GlobalVariable *DestBBs = getIndirectTargets(Fn, EncKey1);

      for (auto &BB : Fn) {
        auto *BI = dyn_cast<BranchInst>(BB.getTerminator());
        if (BI && BI->isConditional()) {
          IRBuilder<> IRB(BI);

          Value *Cond = BI->getCondition();
          Value *Idx;
          Value *TIdx, *FIdx;

          TIdx = ConstantInt::get(intType, BBNumbering[BI->getSuccessor(0)]);
          FIdx = ConstantInt::get(intType, BBNumbering[BI->getSuccessor(1)]);
          Idx = IRB.CreateSelect(Cond, TIdx, FIdx);

          Value *GEP =
              IRB.CreateGEP(DestBBs->getValueType(), DestBBs, {Zero, Idx});
          Value *EncDestAddr =
              IRB.CreateLoad(GEP->getType(), GEP, "EncDestAddr");
          // -EncKey = X - FuncSecret
          Value *DecKey = IRB.CreateAdd(EncKey, MySecret);
          Value *DestAddr =
              IRB.CreateGEP(Type::getInt8Ty(Ctx), EncDestAddr, DecKey);

          IndirectBrInst *IBI = IndirectBrInst::Create(DestAddr, 2);
          IBI->addDestination(BI->getSuccessor(0));
          IBI->addDestination(BI->getSuccessor(1));
          ReplaceInstWithInst(BI, IBI);
        }
      }
    }
  }
  return PreservedAnalyses::none();
}

void IndirectBranchPass::NumberBasicBlock(Function &F) {
  for (auto &BB : F) {
    if (auto *BI = dyn_cast<BranchInst>(BB.getTerminator())) {
      if (BI->isConditional()) {
        unsigned N = BI->getNumSuccessors();
        for (unsigned I = 0; I < N; I++) {
          BasicBlock *Succ = BI->getSuccessor(I);
          if (BBNumbering.count(Succ) == 0) {
            BBTargets.push_back(Succ);
            BBNumbering[Succ] = 0;
          }
        }
      }
    }
  }

  long seed = RandomEngine.get_uint32_t();
  std::default_random_engine e(seed);
  std::shuffle(BBTargets.begin(), BBTargets.end(), e);

  unsigned N = 0;
  for (auto BB : BBTargets) {
    BBNumbering[BB] = N++;
  }
}

GlobalVariable *IndirectBranchPass::getIndirectTargets(Function &F, ConstantInt *EncKey) {
  std::string GVName(F.getName().str() + "_IndirectBrTargets");
  GlobalVariable *GV = F.getParent()->getNamedGlobal(GVName);
  if (GV)
    return GV;

  // encrypt branch targets
  std::vector<Constant *> Elements;
  for (const auto BB : BBTargets) {
    Constant *CE = ConstantExpr::getBitCast(BlockAddress::get(BB),
                                            Type::getInt8PtrTy(F.getContext()));
    CE = ConstantExpr::getGetElementPtr(Type::getInt8Ty(F.getContext()), CE,
                                        EncKey);
    Elements.push_back(CE);
  }

  ArrayType *ATy =
      ArrayType::get(Type::getInt8PtrTy(F.getContext()), Elements.size());
  Constant *CA = ConstantArray::get(ATy, ArrayRef<Constant *>(Elements));
  GV =
      new GlobalVariable(*F.getParent(), ATy, false,
                         GlobalValue::LinkageTypes::PrivateLinkage, CA, GVName);
  appendToCompilerUsed(*F.getParent(), {GV});
  return GV;
}

IndirectBranchPass *llvm::createIndirectBranch(bool flag) {
  return new IndirectBranchPass(flag);
}