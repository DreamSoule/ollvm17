#ifndef LLVM_STRING_ENCRYPTION_H
#define LLVM_STRING_ENCRYPTION_H
// LLVM libs
#include "llvm/Transforms/Utils/GlobalStatus.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
//#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/SHA1.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

// User libs
#include "CryptoUtils.h"
#include "Utils.h"
// System libs
#include <map>
#include <set>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

#include "ObfuscationOptions.h"

using namespace std;
namespace llvm {
struct EncryptedGV {
  GlobalVariable *GV;
  uint64_t key;
  uint32_t len;
};

class StringEncryptionPass : public PassInfoMixin<StringEncryptionPass> {
public:
  bool flag;
  struct CSPEntry {
    CSPEntry()
        : ID(0), Offset(0), DecGV(nullptr), DecStatus(nullptr),
          DecFunc(nullptr) {}
    unsigned ID;
    unsigned Offset;
    GlobalVariable *DecGV;
    GlobalVariable *DecStatus; // is decrypted or not
    std::vector<uint8_t> Data;
    std::vector<uint8_t> EncKey;
    Function *DecFunc;
  };

  struct CSUser {
    CSUser(Type *ETy, GlobalVariable *User, GlobalVariable *NewGV)
        : Ty(ETy), GV(User), DecGV(NewGV), DecStatus(nullptr),
          InitFunc(nullptr) {}
    Type *Ty;
    GlobalVariable *GV;
    GlobalVariable *DecGV;
    GlobalVariable *DecStatus; // is decrypted or not
    Function *InitFunc;        // InitFunc will use decryted string to
    // initialize DecGV
  };

  ObfuscationOptions *Options;
  CryptoUtils RandomEngine;
  std::vector<CSPEntry *> ConstantStringPool;
  std::map<GlobalVariable *, CSPEntry *> CSPEntryMap;
  std::map<GlobalVariable *, CSUser *> CSUserMap;
  GlobalVariable *EncryptedStringTable;
  std::set<GlobalVariable *> MaybeDeadGlobalVars;

  map<Function * /*Function*/, GlobalVariable * /*Decryption Status*/>
      encstatus;
  StringEncryptionPass(bool flag) {
    this->flag = flag;
    Options = new ObfuscationOptions;
    //EncryptedStringTable = new GlobalVariable;
  }
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM); // Pass实现函数
  bool do_StrEnc(Module &M, ModuleAnalysisManager &AM);
  void collectConstantStringUser(GlobalVariable *CString,
                                 std::set<GlobalVariable *> &Users);
  bool isValidToEncrypt(GlobalVariable *GV);
  bool processConstantStringUse(Function *F);
  void deleteUnusedGlobalVariable();
  Function *buildDecryptFunction(Module *M, const CSPEntry *Entry);
  Function *buildInitFunction(Module *M, const CSUser *User);
  void getRandomBytes(std::vector<uint8_t> &Bytes, uint32_t MinSize,
                      uint32_t MaxSize);
  void lowerGlobalConstant(Constant *CV, IRBuilder<> &IRB, Value *Ptr,
                           Type *Ty);
  void lowerGlobalConstantStruct(ConstantStruct *CS, IRBuilder<> &IRB,
                                 Value *Ptr, Type *Ty);
  void lowerGlobalConstantArray(ConstantArray *CA, IRBuilder<> &IRB, Value *Ptr,
                                Type *Ty);
  static bool isRequired() { return true; } // 直接返回true即可
};
StringEncryptionPass *createStringEncryption(bool flag); // 创建字符串加密
}
#endif