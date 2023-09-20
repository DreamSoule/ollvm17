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
#include "IndirectGlobalVariable.h"
#include <random>

PreservedAnalyses IndirectGlobalVariablePass::run(Module &M, ModuleAnalysisManager &AM) {

  for (Function &Fn : M) {
    if (!toObfuscate(flag, &Fn, "igv")) {
      continue;
    }

    if (Options && Options->skipFunction(Fn.getName())) {
      continue;
    }

    LLVMContext &Ctx = Fn.getContext();

    GVNumbering.clear();
    GlobalVariables.clear();

    LowerConstantExpr(Fn);
    NumberGlobalVariable(Fn);

    if (GlobalVariables.empty()) {
      continue;
    }

    uint64_t V = RandomEngine.get_uint64_t();
    IntegerType *intType = Type::getInt32Ty(Ctx);
    unsigned pointerSize = Fn.getEntryBlock().getModule()->getDataLayout().getTypeAllocSize(PointerType::getUnqual(Fn.getContext())); // Soule
    if (pointerSize == 8) {
      intType = Type::getInt64Ty(Ctx);
    }

    ConstantInt *EncKey = ConstantInt::get(intType, V, false);
    ConstantInt *EncKey1 = ConstantInt::get(intType, -V, false);

    Value *MySecret = ConstantInt::get(intType, 0, true);

    ConstantInt *Zero = ConstantInt::get(intType, 0);
    GlobalVariable *GVars = getIndirectGlobalVariables(Fn, EncKey1);

    for (inst_iterator I = inst_begin(Fn), E = inst_end(Fn); I != E; ++I) {
      Instruction *Inst = &*I;
      if (isa<LandingPadInst>(Inst) || isa<CleanupPadInst>(Inst) ||
          isa<CatchPadInst>(Inst) || isa<CatchReturnInst>(Inst) ||
          isa<CatchSwitchInst>(Inst) || isa<ResumeInst>(Inst) ||
          isa<CallInst>(Inst)) {
        continue;
      }
      if (PHINode *PHI = dyn_cast<PHINode>(Inst)) {
        for (unsigned int i = 0; i < PHI->getNumIncomingValues(); ++i) {
          Value *val = PHI->getIncomingValue(i);
          if (GlobalVariable *GV = dyn_cast<GlobalVariable>(val)) {
            if (GVNumbering.count(GV) == 0) {
              continue;
            }

            Instruction *IP = PHI->getIncomingBlock(i)->getTerminator();
            IRBuilder<> IRB(IP);

            Value *Idx = ConstantInt::get(intType, GVNumbering[GV]);
            Value *GEP =
                IRB.CreateGEP(GVars->getValueType(), GVars, {Zero, Idx});
            LoadInst *EncGVAddr =
                IRB.CreateLoad(GEP->getType(), GEP, GV->getName());

            Value *Secret = IRB.CreateAdd(EncKey, MySecret);
            Value *GVAddr =
                IRB.CreateGEP(Type::getInt8Ty(Ctx), EncGVAddr, Secret);
            GVAddr = IRB.CreateBitCast(GVAddr, GV->getType());
            GVAddr->setName("IndGV0_");
            PHI->setIncomingValue(i, GVAddr);
          }
        }
      } else {
        for (User::op_iterator op = Inst->op_begin(); op != Inst->op_end();
             ++op) {
          if (GlobalVariable *GV = dyn_cast<GlobalVariable>(*op)) {
            if (GVNumbering.count(GV) == 0) {
              continue;
            }

            IRBuilder<> IRB(Inst);
            Value *Idx = ConstantInt::get(intType, GVNumbering[GV]);
            Value *GEP =
                IRB.CreateGEP(GVars->getValueType(), GVars, {Zero, Idx});
            LoadInst *EncGVAddr =
                IRB.CreateLoad(GEP->getType(), GEP, GV->getName());

            Value *Secret = IRB.CreateAdd(EncKey, MySecret);
            Value *GVAddr =
                IRB.CreateGEP(Type::getInt8Ty(Ctx), EncGVAddr, Secret);
            GVAddr = IRB.CreateBitCast(GVAddr, GV->getType());
            GVAddr->setName("IndGV1_");
            Inst->replaceUsesOfWith(GV, GVAddr);
          }
        }
      }
    }

  }
  return PreservedAnalyses::none();
}

void IndirectGlobalVariablePass::NumberGlobalVariable(Function &F) {
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    for (User::op_iterator op = (*I).op_begin(); op != (*I).op_end(); ++op) {
      Value *val = *op;
      if (GlobalVariable *GV = dyn_cast<GlobalVariable>(val)) {
        if (!GV->isThreadLocal() && GVNumbering.count(GV) == 0 &&
            !GV->isDLLImportDependent()) {
          GVNumbering[GV] = GlobalVariables.size();
          GlobalVariables.push_back((GlobalVariable *)val);
        }
      }
    }
  }
}

GlobalVariable* IndirectGlobalVariablePass::getIndirectGlobalVariables(Function &F, ConstantInt *EncKey) {
  std::string GVName(F.getName().str() + "_IndirectGVars");
  GlobalVariable *GV = F.getParent()->getNamedGlobal(GVName);
  if (GV)
    return GV;

  std::vector<Constant *> Elements;
  for (auto GVar : GlobalVariables) {
    Constant *CE =
        ConstantExpr::getBitCast(GVar, Type::getInt8PtrTy(F.getContext()));
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


IndirectGlobalVariablePass *llvm::createIndirectGlobalVariable(bool flag) {
  return new IndirectGlobalVariablePass(flag);
}