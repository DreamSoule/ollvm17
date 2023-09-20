/*
 *  LLVM StringEncryption Pass
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
// User libs
#include "Utils.h"
#include "CryptoUtils.h"
#include "SplitBasicBlock.h"
// namespace
using namespace llvm;
using std::vector;

#define DEBUG_TYPE "split" // 调试标识
// Stats
STATISTIC(Split, "Basicblock splitted"); // 宏定义

// 可选的参数，指定一个基本块会被分裂成几个基本块，默认值为 3
static cl::opt<int> SplitNum("split_num", cl::init(3), cl::desc("Split <split_num> time(s) each BB")); 
// 貌似NEW PM暂时不支持这种传递

/**
 * @brief 新的实现方案
 * 
 * @param F 
 * @param AM 
 * @return PreservedAnalyses 
 */
PreservedAnalyses SplitBasicBlockPass::run(Function& F, FunctionAnalysisManager& AM) {
    Function *tmp = &F; // 传入的Function
    if (toObfuscate(flag, tmp, "split")){ // 判断什么函数需要开启混淆
        split(tmp); // 分割流程
        ++Split; // 计次
    }
    return PreservedAnalyses::none();
}

/**
 * @brief 对传入的基本块做分割
 * 
 * @param BB 
 */
void SplitBasicBlockPass::split(Function *f){
    std::vector<BasicBlock *> origBB;
    // 保存所有基本块 防止分割的同时迭代新的基本块
    for (Function::iterator I = f->begin(), IE = f->end(); I != IE; ++I){
        origBB.push_back(&*I);
    }
    // 遍历函数的全部基本块
    for (std::vector<BasicBlock *>::iterator I = origBB.begin(), IE = origBB.end();I != IE; ++I){
        BasicBlock *curr = *I;
        //outs() << "\033[1;32mSplitNum : " << SplitNum << "\033[0m\n";
        //outs() << "\033[1;32mBasicBlock Size : " << curr->size() << "\033[0m\n";
        int splitN = SplitNum;
        // 无需分割只有一条指令的基本块
        // 不可分割含有PHI指令基本块
        if (curr->size() < 2 || containsPHI(curr)){
            //outs() << "\033[0;33mThis BasicBlock is lower then two or had PIH Instruction!\033[0m\n";
            continue;
        }
        // 检查splitN和基本块大小 如果传入的分割块数甚至大于等于基本块自身大小 则修改分割数为基本块大小减一
        if ((size_t)splitN >= curr->size()){
            //outs() << "\033[0;33mSplitNum is bigger then currBasicBlock's size\033[0m\n"; // warning
            //outs() << "\033[0;33mSo SplitNum Now is BasicBlock's size -1 : " << (curr->size() - 1) << "\033[0m\n";
            splitN = curr->size() - 1;
        } else {
            //outs() << "\033[1;32msplitNum Now is " << splitN << "\033[0m\n";
        }
        // Generate splits point
        std::vector<int> test;
        for (unsigned i = 1; i < curr->size(); ++i){
            test.push_back(i);
        }
        // Shuffle
        if (test.size() != 1){
            shuffle(test);
            std::sort(test.begin(), test.begin() + splitN);
        }
        // 分割
        BasicBlock::iterator it = curr->begin();
        BasicBlock *toSplit = curr;
        int last = 0;
        for (int i = 0; i < splitN; ++i){
            if (toSplit->size() < 2){
                continue;
            }
            for (int j = 0; j < test[i] - last; ++j){
                ++it;
            }
            last = test[i];
            toSplit = toSplit->splitBasicBlock(it, toSplit->getName() + ".split");
        }
        ++Split;
    }
}

/**
 * @brief 判断基本块是否包含PHI指令
 * 
 * @param BB 
 * @return true 
 * @return false 
 */
bool SplitBasicBlockPass::containsPHI(BasicBlock *BB){
    for (Instruction &I : *BB){
        if (isa<PHINode>(&I)){
            return true;
        }
    }
    return false;
}

/**
 * @brief 辅助分割流程的函数
 * 
 * @param vec 
 */
void SplitBasicBlockPass::shuffle(std::vector<int> &vec){
    int n = vec.size();
    for (int i = n - 1; i > 0; --i){
        std::swap(vec[i], vec[cryptoutils->get_uint32_t() % (i + 1)]);
    }
}

/**
 * @brief 便于调用基本块分割
 *
 * @param flag
 * @return FunctionPass*
 */
SplitBasicBlockPass *llvm::createSplitBasicBlock(bool flag){
    return new SplitBasicBlockPass(flag);
}