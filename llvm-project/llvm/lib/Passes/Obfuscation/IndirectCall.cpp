#include "IndirectCall.h"

using namespace llvm;
using std::vector;

/**
 * @brief 
 * 
 * @param F 
 * @param AM 
 * @return PreservedAnalyses 
 */
PreservedAnalyses IndirectCallPass::run(Function &F, FunctionAnalysisManager &AM){
    // 判断是否需要开启间接调用
    if (toObfuscate(flag, &F, "icall")){
        doIndirctCall(F);
    }
    return PreservedAnalyses::none();
}

bool IndirectCallPass::doIndirctCall(Function &Fn){
    if (Options && Options->skipFunction(Fn.getName())) {
        return false;
    }

    LLVMContext &Ctx = Fn.getContext();

    CalleeNumbering.clear();
    Callees.clear();
    CallSites.clear();

    NumberCallees(Fn);

    if (Callees.empty()) {
        return false;
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
    GlobalVariable *Targets = getIndirectCallees(Fn, EncKey1);

    for (auto CI : CallSites) {
        SmallVector<Value *, 8> Args;
        SmallVector<AttributeSet, 8> ArgAttrVec;

        CallBase *CB = CI;

        Function *Callee = CB->getCalledFunction();
        FunctionType *FTy = CB->getFunctionType();
        IRBuilder<> IRB(CB);

        Args.clear();
        ArgAttrVec.clear();

        Value *Idx = ConstantInt::get(intType, CalleeNumbering[CB->getCalledFunction()]);
        Value *GEP = IRB.CreateGEP(Targets->getValueType(), Targets, {Zero, Idx});
        LoadInst *EncDestAddr = IRB.CreateLoad(GEP->getType(), GEP, CI->getName());

        const AttributeList &CallPAL = CB->getAttributes();
        auto I = CB->arg_begin();
        unsigned i = 0;

        for (unsigned e = FTy->getNumParams(); i != e; ++I, ++i) {
          Args.push_back(*I);
          AttributeSet Attrs = CallPAL.getParamAttrs(i);
          ArgAttrVec.push_back(Attrs);
        }

        for (auto E = CB->arg_end(); I != E; ++I, ++i) {
          Args.push_back(*I);
          ArgAttrVec.push_back(CallPAL.getParamAttrs(i));
        }

        Value *Secret = IRB.CreateAdd(EncKey, MySecret);
        Value *DestAddr = IRB.CreateGEP(Type::getInt8Ty(Ctx), EncDestAddr, Secret);

        Value *FnPtr = IRB.CreateBitCast(DestAddr, FTy->getPointerTo());
        FnPtr->setName("Call_" + Callee->getName());
        CB->setCalledOperand(FnPtr);
    }

    return true;
}

/**
 * @brief 
 * 
 * @param F 
 * @param EncKey 
 * @return GlobalVariable* 
 */
GlobalVariable *IndirectCallPass::getIndirectCallees(Function &F, ConstantInt *EncKey){
    std::string GVName(F.getName().str() + "_IndirectCallees");
    GlobalVariable *GV = F.getParent()->getNamedGlobal(GVName);
    if (GV){
        return GV;
    }
    // callee's address
    std::vector<Constant *> Elements;
    for (auto Callee : Callees){
        Constant *CE = ConstantExpr::getBitCast(Callee, Type::getInt8PtrTy(F.getContext()));
        CE = ConstantExpr::getGetElementPtr(Type::getInt8Ty(F.getContext()), CE, EncKey);
        Elements.push_back(CE);
    }
    ArrayType *ATy = ArrayType::get(Type::getInt8PtrTy(F.getContext()), Elements.size());
    Constant *CA = ConstantArray::get(ATy, ArrayRef<Constant *>(Elements));
    GV = new GlobalVariable(*F.getParent(), ATy, false, GlobalValue::LinkageTypes::PrivateLinkage, CA, GVName);
    appendToCompilerUsed(*F.getParent(), {GV});
    return GV;
}

/**
 * @brief 
 * 
 * @param F 
 */
void IndirectCallPass::NumberCallees(Function &F){
    for (auto &BB : F){
        for (auto &I : BB){
            if (dyn_cast<CallInst>(&I)){
                CallSite CS(&I);
                Function *Callee = CS.getCalledFunction();
                if (Callee == nullptr){
                    continue;
                }
                if (Callee->isIntrinsic()){
                    continue;
                }
                CallSites.push_back((CallInst *)&I);
                if (CalleeNumbering.count(Callee) == 0){
                    CalleeNumbering[Callee] = Callees.size();
                    Callees.push_back(Callee);
                }
            }
        }
    }
}

/**
 * @brief 
 * 
 * @param flag 
 * @return IndirectCallPass* 
 */
IndirectCallPass *llvm::createIndirectCall(bool flag){
    return new IndirectCallPass(flag);
}