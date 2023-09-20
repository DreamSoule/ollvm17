# 前言
项目大致内容与我之前发布的[OLLVM16](https://github.com/DreamSoule/ollvm16)区别不大, 仅对LLVM17做了兼容性修复<br>
感兴趣的可以自己下载之前项目对比查看修改部分
# 混淆功能列表
> 命令行添加位置: 项目->属性->C/C++->Command Line
```bash
- bcf # 虚假控制流
-   bcf_prob # 虚假控制流混淆概率 1~100, 默认70
-   bcf_loop # 虚假控制流重复次数, 无限制, 默认2
- fla # 控制流平坦化
- sub # 指令替换(add/and/sub/or/xor)
-   sub_loop # 指令替换次数, 无限制, 默认1
- sobf # 字符串混淆(仅窄字符)
- split # 基本块分割
-   split_num # 将原基本块分割数量, 无限制, 默认3
- ibr # 间接分支
- icall # 间接调用 (call 寄存器)
- igv # 间接全局变量
```
# 功能全开
> 命令行添加位置: 项目->属性->C/C++->命令行
```bash
-mllvm -fla -mllvm -bcf -mllvm -bcf_prob=80 -mllvm -bcf_loop=3 -mllvm -sobf -mllvm -icall -mllvm -ibr -mllvm -igv -mllvm -sub -mllvm -sub_loop=3 -mllvm -split -mllvm -split_num=5
```
# 官方LLVM修补教程
1.下载LLVM官方源码 [LLVM 17.0.1](https://github.com/llvm/llvm-project/releases/tag/llvmorg-17.0.1) 并解压<br>
2.下载此项目, 将项目内文件替换至官方源码内<br>
3.使用cmake创建自己需要的编译工具生成文件, 以 VisualStudio 2022 为例
```bash
cd llvm-project
mkdir build_vs2022
cd build_vs2022
cmake -G "Visual Studio 17 2022" -DCMAKE_C_FLAGS=/utf-8 -DCMAKE_CXX_FLAGS=/utf-8 -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_EH=OFF -DLLVM_ENABLE_RTTI=OFF -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_PROJECTS="clang;lld" -A x64 ../llvm
```
4.等cmake生成出解决方案后打开build_vs2022目录内的LLVM.sln点击生成解决方案即可<br>
>~~(如果编译过程中有提示部分函数是private的无法调用的话把private注释即可 LLVM 17.0.1注释后正常无报错)~~<br>
>要取消private属性的文件及目标行(LLVM17.0.1): ...\llvm-project\llvm\include\llvm\IR\Function.h:722<br>
>(以下函数的private属性: removeFromParent | eraseFromParent | getBasicBlockList)
# 修补细节
> ...\llvm-project\llvm\lib\Passes\PassBuilder.cpp
```cpp
// 引用 Obfuscation 相关文件
#include "Obfuscation/BogusControlFlow.h" // 虚假控制流
#include "Obfuscation/Flattening.h"  // 控制流平坦化
#include "Obfuscation/SplitBasicBlock.h" // 基本块分割
#include "Obfuscation/Substitution.h" // 指令替换
#include "Obfuscation/StringEncryption.h" // 字符串加密
#include "Obfuscation/IndirectGlobalVariable.h" // 间接全局变量
#include "Obfuscation/IndirectBranch.h" // 间接跳转
#include "Obfuscation/IndirectCall.h" // 间接调用

// 添加命令行支持
static cl::opt<bool> s_obf_split("split", cl::init(false), cl::desc("SplitBasicBlock: split_num=3(init)"));
static cl::opt<bool> s_obf_sobf("sobf", cl::init(false), cl::desc("String Obfuscation"));
static cl::opt<bool> s_obf_fla("fla", cl::init(false), cl::desc("Flattening"));
static cl::opt<bool> s_obf_sub("sub", cl::init(false), cl::desc("Substitution: sub_loop"));
static cl::opt<bool> s_obf_bcf("bcf", cl::init(false), cl::desc("BogusControlFlow: application number -bcf_loop=x must be x > 0"));
static cl::opt<bool> s_obf_ibr("ibr", cl::init(false), cl::desc("Indirect Branch"));
static cl::opt<bool> s_obf_igv("igv", cl::init(false), cl::desc("Indirect Global Variable"));
static cl::opt<bool> s_obf_icall("icall", cl::init(false), cl::desc("Indirect Call"));

// 在此函数内直接注册Pipeline回调
PassBuilder::PassBuilder(...) {
...
  this->registerPipelineStartEPCallback(
      [](llvm::ModulePassManager &MPM,
         llvm::OptimizationLevel Level) {
        outs() << "[Soule] run.PipelineStartEPCallback\n";
        MPM.addPass(StringEncryptionPass(s_obf_sobf));
        llvm::FunctionPassManager FPM;
        FPM.addPass(IndirectCallPass(s_obf_icall));
        FPM.addPass(SplitBasicBlockPass(s_obf_split));
        FPM.addPass(FlatteningPass(s_obf_fla));
        FPM.addPass(SubstitutionPass(s_obf_sub));
        FPM.addPass(BogusControlFlowPass(s_obf_bcf));
        MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
        MPM.addPass(IndirectBranchPass(s_obf_ibr));
        MPM.addPass(IndirectGlobalVariablePass(s_obf_igv));
      }
  );
}
```
> ...\llvm-project\llvm\lib\Passes\CMakeLists.txt
``` bash
# 添加 Obfuscation 相关源码
add_llvm_component_library(LLVMPasses
...
Obfuscation/Utils.cpp
Obfuscation/CryptoUtils.cpp
Obfuscation/ObfuscationOptions.cpp
Obfuscation/BogusControlFlow.cpp
Obfuscation/IPObfuscationContext.cpp
Obfuscation/Flattening.cpp
Obfuscation/StringEncryption.cpp
Obfuscation/SplitBasicBlock.cpp
Obfuscation/Substitution.cpp
Obfuscation/IndirectBranch.cpp
Obfuscation/IndirectCall.cpp
Obfuscation/IndirectGlobalVariable.cpp
...
)
```
# Credits
[LLVM](https://github.com/llvm/llvm-project)

[SsagePass](https://github.com/SsageParuders/SsagePass)

[wwh1004-ollvm16](https://github.com/wwh1004/ollvm-16)
