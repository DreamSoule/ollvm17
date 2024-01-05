// TypeART library
//
// Copyright (c) 2017-2022 TypeART Authors
// Distributed under the BSD 3-Clause license.
// (See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/BSD-3-Clause)
//
// Project home: https://github.com/tudasc/TypeART
//
// SPDX-License-Identifier: BSD-3-Clause
//

// Compatibility for Clang v10 and higher.
// In Clang 11 the CallSite.h was removed, therefore we copied the Clang v10 version of the header into the TypeART
// project, see https://github.com/llvm/llvm-project

#ifndef COMPAT_LLVM_IR_CALLSITE_H
#define COMPAT_LLVM_IR_CALLSITE_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
//===- CallSite.h - Abstract Call & Invoke instrs ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//#include "llvm/ADT/Optional.h"// Soule.llvm17.update: Legacy alias of llvm::Optional to std::optional
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include <cassert>
#include <cstdint>
#include <iterator>

namespace llvm {

namespace Intrinsic {
typedef unsigned ID;
}

template <typename FunTy = const Function, typename BBTy = const BasicBlock, typename ValTy = const Value,
          typename UserTy = const User, typename UseTy = const Use, typename InstrTy = const Instruction,
          typename CallTy = const CallInst, typename InvokeTy = const InvokeInst, typename CallBrTy = const CallBrInst,
          typename IterTy = User::const_op_iterator>
class CallSiteBase {
 protected:
  PointerIntPair<InstrTy*, 2, int> I;

  CallSiteBase() = default;
  CallSiteBase(CallTy* CI) : I(CI, 1) {
    assert(CI);
  }
  CallSiteBase(InvokeTy* II) : I(II, 0) {
    assert(II);
  }
  CallSiteBase(CallBrTy* CBI) : I(CBI, 2) {
    assert(CBI);
  }
  explicit CallSiteBase(ValTy* II) {
    *this = get(II);
  }

 private:
  /// This static method is like a constructor. It will create an appropriate
  /// call site for a Call, Invoke or CallBr instruction, but it can also create
  /// a null initialized CallSiteBase object for something which is NOT a call
  /// site.
  static CallSiteBase get(ValTy* V) {
    if (InstrTy* II = dyn_cast<InstrTy>(V)) {
      if (II->getOpcode() == Instruction::Call)
        return CallSiteBase(static_cast<CallTy*>(II));
      if (II->getOpcode() == Instruction::Invoke)
        return CallSiteBase(static_cast<InvokeTy*>(II));
      if (II->getOpcode() == Instruction::CallBr)
        return CallSiteBase(static_cast<CallBrTy*>(II));
    }
    return CallSiteBase();
  }

 public:
  /// Return true if a CallInst is enclosed.
  bool isCall() const {
    return I.getInt() == 1;
  }

  /// Return true if a InvokeInst is enclosed. !I.getInt() may also signify a
  /// NULL instruction pointer, so check that.
  bool isInvoke() const {
    return getInstruction() && I.getInt() == 0;
  }

  /// Return true if a CallBrInst is enclosed.
  bool isCallBr() const {
    return I.getInt() == 2;
  }

  InstrTy* getInstruction() const {
    return I.getPointer();
  }
  InstrTy* operator->() const {
    return I.getPointer();
  }
  explicit operator bool() const {
    return I.getPointer();
  }

  /// Get the basic block containing the call site.
  BBTy* getParent() const {
    return getInstruction()->getParent();
  }

  /// Return the pointer to function that is being called.
  ValTy* getCalledValue() const {
    assert(getInstruction() && "Not a call, invoke or callbr instruction!");
    return *getCallee();
  }

  /// Return the function being called if this is a direct call, otherwise
  /// return null (if it's an indirect call).
  FunTy* getCalledFunction() const {
    return dyn_cast<FunTy>(getCalledValue());
  }

  /// Return true if the callsite is an indirect call.
  bool isIndirectCall() const {
    const Value* V = getCalledValue();
    if (!V)
      return false;
    if (isa<FunTy>(V) || isa<Constant>(V))
      return false;
    if (const CallBase* CB = dyn_cast<CallBase>(getInstruction()))
      if (CB->isInlineAsm())
        return false;
    return true;
  }

// Soule
  /* /// Set the callee to the specified value.  Unlike the function of the same
  /// name on CallBase, does not modify the type!
  void setCalledFunction(Value* V) {
    const auto elem_type = [&V]() {
#if LLVM_VERSION_MAJOR > 13
      return V->getType()->getPointerElementType();
#else
      return cast<PointerType>(V->getType())->getElementType();
#endif
    };
    assert(getInstruction() && "Not a call, callbr, or invoke instruction!");

    // assert(elem_type() == cast<CallBase>(getInstruction())->getFunctionType() &&
    //        "New callee type does not match FunctionType on call");
    *getCallee() = V;
  }*/

  /// Return the intrinsic ID of the intrinsic called by this CallSite,
  /// or Intrinsic::not_intrinsic if the called function is not an
  /// intrinsic, or if this CallSite is an indirect call.
  Intrinsic::ID getIntrinsicID() const {
    if (auto* F = getCalledFunction())
      return F->getIntrinsicID();
    // Don't use Intrinsic::not_intrinsic, as it will require pulling
    // Intrinsics.h into every header that uses CallSite.
    return static_cast<Intrinsic::ID>(0);
  }

  /// Determine whether the passed iterator points to the callee operand's Use.
  bool isCallee(Value::const_user_iterator UI) const {
    return isCallee(&UI.getUse());
  }

  /// Determine whether this Use is the callee operand's Use.
  bool isCallee(const Use* U) const {
    return getCallee() == U;
  }

  /// Determine whether the passed iterator points to an argument operand.
  bool isArgOperand(Value::const_user_iterator UI) const {
    return isArgOperand(&UI.getUse());
  }

  /// Determine whether the passed use points to an argument operand.
  bool isArgOperand(const Use* U) const {
    assert(getInstruction() == U->getUser());
    return arg_begin() <= U && U < arg_end();
  }

  /// Determine whether the passed iterator points to a bundle operand.
  bool isBundleOperand(Value::const_user_iterator UI) const {
    return isBundleOperand(&UI.getUse());
  }

  /// Determine whether the passed use points to a bundle operand.
  bool isBundleOperand(const Use* U) const {
    assert(getInstruction() == U->getUser());
    if (!hasOperandBundles())
      return false;
    unsigned OperandNo = U - (*this)->op_begin();
    return getBundleOperandsStartIndex() <= OperandNo && OperandNo < getBundleOperandsEndIndex();
  }

  /// Determine whether the passed iterator points to a data operand.
  bool isDataOperand(Value::const_user_iterator UI) const {
    return isDataOperand(&UI.getUse());
  }

  /// Determine whether the passed use points to a data operand.
  bool isDataOperand(const Use* U) const {
    return data_operands_begin() <= U && U < data_operands_end();
  }

  ValTy* getArgument(unsigned ArgNo) const {
    assert(arg_begin() + ArgNo < arg_end() && "Argument # out of range!");
    return *(arg_begin() + ArgNo);
  }

  void setArgument(unsigned ArgNo, Value* newVal) {
    assert(getInstruction() && "Not a call, invoke or callbr instruction!");
    assert(arg_begin() + ArgNo < arg_end() && "Argument # out of range!");
    getInstruction()->setOperand(ArgNo, newVal);
  }

  /// Given a value use iterator, returns the argument that corresponds to it.
  /// Iterator must actually correspond to an argument.
  unsigned getArgumentNo(Value::const_user_iterator I) const {
    return getArgumentNo(&I.getUse());
  }

  /// Given a use for an argument, get the argument number that corresponds to
  /// it.
  unsigned getArgumentNo(const Use* U) const {
    assert(getInstruction() && "Not a call, invoke or callbr instruction!");
    assert(isArgOperand(U) && "Argument # out of range!");
    return U - arg_begin();
  }

  /// The type of iterator to use when looping over actual arguments at this
  /// call site.
  using arg_iterator = IterTy;

  iterator_range<IterTy> args() const {
    return make_range(arg_begin(), arg_end());
  }
  bool arg_empty() const {
    return arg_end() == arg_begin();
  }
  unsigned arg_size() const {
    return unsigned(arg_end() - arg_begin());
  }

  /// Given a value use iterator, return the data operand corresponding to it.
  /// Iterator must actually correspond to a data operand.
  unsigned getDataOperandNo(Value::const_user_iterator UI) const {
    return getDataOperandNo(&UI.getUse());
  }

  /// Given a use for a data operand, get the data operand number that
  /// corresponds to it.
  unsigned getDataOperandNo(const Use* U) const {
    assert(getInstruction() && "Not a call, invoke or callbr instruction!");
    assert(isDataOperand(U) && "Data operand # out of range!");
    return U - data_operands_begin();
  }

  /// Type of iterator to use when looping over data operands at this call site
  /// (see below).
  using data_operand_iterator = IterTy;

  /// data_operands_begin/data_operands_end - Return iterators iterating over
  /// the call / invoke / callbr argument list and bundle operands. For invokes,
  /// this is the set of instruction operands except the invoke target and the
  /// two successor blocks; for calls this is the set of instruction operands
  /// except the call target; for callbrs the number of labels to skip must be
  /// determined first.

  IterTy data_operands_begin() const {
    assert(getInstruction() && "Not a call or invoke instruction!");
    return cast<CallBase>(getInstruction())->data_operands_begin();
  }
  IterTy data_operands_end() const {
    assert(getInstruction() && "Not a call or invoke instruction!");
    return cast<CallBase>(getInstruction())->data_operands_end();
  }
  iterator_range<IterTy> data_ops() const {
    return make_range(data_operands_begin(), data_operands_end());
  }
  bool data_operands_empty() const {
    return data_operands_end() == data_operands_begin();
  }
  unsigned data_operands_size() const {
    return std::distance(data_operands_begin(), data_operands_end());
  }

  /// Return the type of the instruction that generated this call site.
  Type* getType() const {
    return (*this)->getType();
  }

  /// Return the caller function for this call site.
  FunTy* getCaller() const {
    return (*this)->getParent()->getParent();
  }

  /// Tests if this call site must be tail call optimized. Only a CallInst can
  /// be tail call optimized.
  bool isMustTailCall() const {
    return isCall() && cast<CallInst>(getInstruction())->isMustTailCall();
  }

  /// Tests if this call site is marked as a tail call.
  bool isTailCall() const {
    return isCall() && cast<CallInst>(getInstruction())->isTailCall();
  }

#define CALLSITE_DELEGATE_GETTER(METHOD)             \
  InstrTy* II = getInstruction();                    \
  return isCall()     ? cast<CallInst>(II)->METHOD   \
         : isCallBr() ? cast<CallBrInst>(II)->METHOD \
                      : cast<InvokeInst>(II)->METHOD

#define CALLSITE_DELEGATE_SETTER(METHOD) \
  InstrTy* II = getInstruction();        \
  if (isCall())                          \
    cast<CallInst>(II)->METHOD;          \
  else if (isCallBr())                   \
    cast<CallBrInst>(II)->METHOD;        \
  else                                   \
    cast<InvokeInst>(II)->METHOD


  unsigned getNumArgOperands() const {
    #if LLVM_VERSION_MAJOR >= 14
    CALLSITE_DELEGATE_GETTER(arg_size());
    #else
    CALLSITE_DELEGATE_GETTER(getNumArgOperands());
    #endif
  }

  ValTy* getArgOperand(unsigned i) const {
    CALLSITE_DELEGATE_GETTER(getArgOperand(i));
  }

  ValTy* getReturnedArgOperand() const {
    CALLSITE_DELEGATE_GETTER(getReturnedArgOperand());
  }

  bool isInlineAsm() const {
    return cast<CallBase>(getInstruction())->isInlineAsm();
  }

  /// Get the calling convention of the call.
  CallingConv::ID getCallingConv() const {
    CALLSITE_DELEGATE_GETTER(getCallingConv());
  }
  /// Set the calling convention of the call.
  void setCallingConv(CallingConv::ID CC) {
    CALLSITE_DELEGATE_SETTER(setCallingConv(CC));
  }

  FunctionType* getFunctionType() const {
    CALLSITE_DELEGATE_GETTER(getFunctionType());
  }

  void mutateFunctionType(FunctionType* Ty) const {
    CALLSITE_DELEGATE_SETTER(mutateFunctionType(Ty));
  }

  /// Get the parameter attributes of the call.
  AttributeList getAttributes() const {
    CALLSITE_DELEGATE_GETTER(getAttributes());
  }
  /// Set the parameter attributes of the call.
  void setAttributes(AttributeList PAL) {
    CALLSITE_DELEGATE_SETTER(setAttributes(PAL));
  }

  void addAttribute(unsigned i, Attribute::AttrKind Kind) {
    CALLSITE_DELEGATE_SETTER(addAttribute(i, Kind));
  }

  void addAttribute(unsigned i, Attribute Attr) {
    CALLSITE_DELEGATE_SETTER(addAttribute(i, Attr));
  }

  void addParamAttr(unsigned ArgNo, Attribute::AttrKind Kind) {
    CALLSITE_DELEGATE_SETTER(addParamAttr(ArgNo, Kind));
  }

  void removeAttribute(unsigned i, Attribute::AttrKind Kind) {
    CALLSITE_DELEGATE_SETTER(removeAttribute(i, Kind));
  }

  void removeAttribute(unsigned i, StringRef Kind) {
    CALLSITE_DELEGATE_SETTER(removeAttribute(i, Kind));
  }

  void removeParamAttr(unsigned ArgNo, Attribute::AttrKind Kind) {
    CALLSITE_DELEGATE_SETTER(removeParamAttr(ArgNo, Kind));
  }

  /// Return true if this function has the given attribute.
  bool hasFnAttr(Attribute::AttrKind Kind) const {
    CALLSITE_DELEGATE_GETTER(hasFnAttr(Kind));
  }

  /// Return true if this function has the given attribute.
  bool hasFnAttr(StringRef Kind) const {
    CALLSITE_DELEGATE_GETTER(hasFnAttr(Kind));
  }

  /// Return true if this return value has the given attribute.
  bool hasRetAttr(Attribute::AttrKind Kind) const {
    CALLSITE_DELEGATE_GETTER(hasRetAttr(Kind));
  }

  /// Return true if the call or the callee has the given attribute.
  bool paramHasAttr(unsigned ArgNo, Attribute::AttrKind Kind) const {
    CALLSITE_DELEGATE_GETTER(paramHasAttr(ArgNo, Kind));
  }

  Attribute getAttribute(unsigned i, Attribute::AttrKind Kind) const {
    CALLSITE_DELEGATE_GETTER(getAttribute(i, Kind));
  }

  Attribute getAttribute(unsigned i, StringRef Kind) const {
    CALLSITE_DELEGATE_GETTER(getAttribute(i, Kind));
  }

  /// Return true if the data operand at index \p i directly or indirectly has
  /// the attribute \p A.
  ///
  /// Normal call, invoke or callbr arguments have per operand attributes, as
  /// specified in the attribute set attached to this instruction, while operand
  /// bundle operands may have some attributes implied by the type of its
  /// containing operand bundle.
  bool dataOperandHasImpliedAttr(unsigned i, Attribute::AttrKind Kind) const {
    CALLSITE_DELEGATE_GETTER(dataOperandHasImpliedAttr(i, Kind));
  }

  /// Extract the alignment of the return value.
  unsigned getRetAlignment() const {
    CALLSITE_DELEGATE_GETTER(getRetAlignment());
  }

  /// Extract the alignment for a call or parameter (0=unknown).
  unsigned getParamAlignment(unsigned ArgNo) const {
    CALLSITE_DELEGATE_GETTER(getParamAlignment(ArgNo));
  }

  /// Extract the byval type for a call or parameter (nullptr=unknown).
  Type* getParamByValType(unsigned ArgNo) const {
    CALLSITE_DELEGATE_GETTER(getParamByValType(ArgNo));
  }

  /// Extract the number of dereferenceable bytes for a call or parameter
  /// (0=unknown).
  uint64_t getDereferenceableBytes(unsigned i) const {
    CALLSITE_DELEGATE_GETTER(getDereferenceableBytes(i));
  }

  /// Extract the number of dereferenceable_or_null bytes for a call or
  /// parameter (0=unknown).
  uint64_t getDereferenceableOrNullBytes(unsigned i) const {
    CALLSITE_DELEGATE_GETTER(getDereferenceableOrNullBytes(i));
  }

  /// Determine if the return value is marked with NoAlias attribute.
  bool returnDoesNotAlias() const {
    CALLSITE_DELEGATE_GETTER(returnDoesNotAlias());
  }

  /// Return true if the call should not be treated as a call to a builtin.
  bool isNoBuiltin() const {
    CALLSITE_DELEGATE_GETTER(isNoBuiltin());
  }

  /// Return true if the call requires strict floating point semantics.
  bool isStrictFP() const {
    CALLSITE_DELEGATE_GETTER(isStrictFP());
  }

  /// Return true if the call should not be inlined.
  bool isNoInline() const {
    CALLSITE_DELEGATE_GETTER(isNoInline());
  }
  void setIsNoInline(bool Value = true) {
    CALLSITE_DELEGATE_SETTER(setIsNoInline(Value));
  }

  /// Determine if the call does not access memory.
  bool doesNotAccessMemory() const {
    CALLSITE_DELEGATE_GETTER(doesNotAccessMemory());
  }
  void setDoesNotAccessMemory() {
    CALLSITE_DELEGATE_SETTER(setDoesNotAccessMemory());
  }

  /// Determine if the call does not access or only reads memory.
  bool onlyReadsMemory() const {
    CALLSITE_DELEGATE_GETTER(onlyReadsMemory());
  }
  void setOnlyReadsMemory() {
    CALLSITE_DELEGATE_SETTER(setOnlyReadsMemory());
  }

  /// Determine if the call does not access or only writes memory.
  bool doesNotReadMemory() const {
    CALLSITE_DELEGATE_GETTER(doesNotReadMemory());
  }
  void setDoesNotReadMemory() {
    CALLSITE_DELEGATE_SETTER(setDoesNotReadMemory());
  }

  /// Determine if the call can access memory only using pointers based
  /// on its arguments.
  bool onlyAccessesArgMemory() const {
    CALLSITE_DELEGATE_GETTER(onlyAccessesArgMemory());
  }
  void setOnlyAccessesArgMemory() {
    CALLSITE_DELEGATE_SETTER(setOnlyAccessesArgMemory());
  }

  /// Determine if the function may only access memory that is
  /// inaccessible from the IR.
  bool onlyAccessesInaccessibleMemory() const {
    CALLSITE_DELEGATE_GETTER(onlyAccessesInaccessibleMemory());
  }
  void setOnlyAccessesInaccessibleMemory() {
    CALLSITE_DELEGATE_SETTER(setOnlyAccessesInaccessibleMemory());
  }

  /// Determine if the function may only access memory that is
  /// either inaccessible from the IR or pointed to by its arguments.
  bool onlyAccessesInaccessibleMemOrArgMem() const {
    CALLSITE_DELEGATE_GETTER(onlyAccessesInaccessibleMemOrArgMem());
  }
  void setOnlyAccessesInaccessibleMemOrArgMem() {
    CALLSITE_DELEGATE_SETTER(setOnlyAccessesInaccessibleMemOrArgMem());
  }

  /// Determine if the call cannot return.
  bool doesNotReturn() const {
    CALLSITE_DELEGATE_GETTER(doesNotReturn());
  }
  void setDoesNotReturn() {
    CALLSITE_DELEGATE_SETTER(setDoesNotReturn());
  }

  /// Determine if the call cannot unwind.
  bool doesNotThrow() const {
    CALLSITE_DELEGATE_GETTER(doesNotThrow());
  }
  void setDoesNotThrow() {
    CALLSITE_DELEGATE_SETTER(setDoesNotThrow());
  }

  /// Determine if the call can be duplicated.
  bool cannotDuplicate() const {
    CALLSITE_DELEGATE_GETTER(cannotDuplicate());
  }
  void setCannotDuplicate() {
    CALLSITE_DELEGATE_SETTER(setCannotDuplicate());
  }

  /// Determine if the call is convergent.
  bool isConvergent() const {
    CALLSITE_DELEGATE_GETTER(isConvergent());
  }
  void setConvergent() {
    CALLSITE_DELEGATE_SETTER(setConvergent());
  }
  void setNotConvergent() {
    CALLSITE_DELEGATE_SETTER(setNotConvergent());
  }

  unsigned getNumOperandBundles() const {
    CALLSITE_DELEGATE_GETTER(getNumOperandBundles());
  }

  bool hasOperandBundles() const {
    CALLSITE_DELEGATE_GETTER(hasOperandBundles());
  }

  unsigned getBundleOperandsStartIndex() const {
    CALLSITE_DELEGATE_GETTER(getBundleOperandsStartIndex());
  }

  unsigned getBundleOperandsEndIndex() const {
    CALLSITE_DELEGATE_GETTER(getBundleOperandsEndIndex());
  }

  unsigned getNumTotalBundleOperands() const {
    CALLSITE_DELEGATE_GETTER(getNumTotalBundleOperands());
  }

  OperandBundleUse getOperandBundleAt(unsigned Index) const {
    CALLSITE_DELEGATE_GETTER(getOperandBundleAt(Index));
  }

  std::optional<OperandBundleUse> getOperandBundle(StringRef Name) const { // Soule.llvm17.update
    CALLSITE_DELEGATE_GETTER(getOperandBundle(Name));
  }

  std::optional<OperandBundleUse> getOperandBundle(uint32_t ID) const { // Soule.llvm17.update
    CALLSITE_DELEGATE_GETTER(getOperandBundle(ID));
  }

  unsigned countOperandBundlesOfType(uint32_t ID) const {
    CALLSITE_DELEGATE_GETTER(countOperandBundlesOfType(ID));
  }

  bool isBundleOperand(unsigned Idx) const {
    CALLSITE_DELEGATE_GETTER(isBundleOperand(Idx));
  }

  IterTy arg_begin() const {
    CALLSITE_DELEGATE_GETTER(arg_begin());
  }

  IterTy arg_end() const {
    CALLSITE_DELEGATE_GETTER(arg_end());
  }

#undef CALLSITE_DELEGATE_GETTER
#undef CALLSITE_DELEGATE_SETTER

  void getOperandBundlesAsDefs(SmallVectorImpl<OperandBundleDef>& Defs) const {
    // Since this is actually a getter that "looks like" a setter, don't use the
    // above macros to avoid confusion.
    cast<CallBase>(getInstruction())->getOperandBundlesAsDefs(Defs);
  }

  /// Determine whether this data operand is not captured.
  bool doesNotCapture(unsigned OpNo) const {
    return dataOperandHasImpliedAttr(OpNo + 1, Attribute::NoCapture);
  }

  /// Determine whether this argument is passed by value.
  bool isByValArgument(unsigned ArgNo) const {
    return paramHasAttr(ArgNo, Attribute::ByVal);
  }

  /// Determine whether this argument is passed in an alloca.
  bool isInAllocaArgument(unsigned ArgNo) const {
    return paramHasAttr(ArgNo, Attribute::InAlloca);
  }

  /// Determine whether this argument is passed by value or in an alloca.
  bool isByValOrInAllocaArgument(unsigned ArgNo) const {
    return paramHasAttr(ArgNo, Attribute::ByVal) || paramHasAttr(ArgNo, Attribute::InAlloca);
  }

  /// Determine if there are is an inalloca argument. Only the last argument can
  /// have the inalloca attribute.
  bool hasInAllocaArgument() const {
    return !arg_empty() && paramHasAttr(arg_size() - 1, Attribute::InAlloca);
  }

  bool doesNotAccessMemory(unsigned OpNo) const {
    return dataOperandHasImpliedAttr(OpNo + 1, Attribute::ReadNone);
  }

  bool onlyReadsMemory(unsigned OpNo) const {
    return dataOperandHasImpliedAttr(OpNo + 1, Attribute::ReadOnly) ||
           dataOperandHasImpliedAttr(OpNo + 1, Attribute::ReadNone);
  }

  bool doesNotReadMemory(unsigned OpNo) const {
    return dataOperandHasImpliedAttr(OpNo + 1, Attribute::WriteOnly) ||
           dataOperandHasImpliedAttr(OpNo + 1, Attribute::ReadNone);
  }

  /// Return true if the return value is known to be not null.
  /// This may be because it has the nonnull attribute, or because at least
  /// one byte is dereferenceable and the pointer is in addrspace(0).
  bool isReturnNonNull() const {
    if (hasRetAttr(Attribute::NonNull))
      return true;
    else if (getDereferenceableBytes(AttributeList::ReturnIndex) > 0 &&
             !NullPointerIsDefined(getCaller(), getType()->getPointerAddressSpace()))
      return true;

    return false;
  }

  /// Returns true if this CallSite passes the given Value* as an argument to
  /// the called function.
  bool hasArgument(const Value* Arg) const {
    for (arg_iterator AI = this->arg_begin(), E = this->arg_end(); AI != E; ++AI)
      if (AI->get() == Arg)
        return true;
    return false;
  }

 private:
  IterTy getCallee() const {
    return cast<CallBase>(getInstruction())->op_end() - 1;
  }
};

class CallSite : public CallSiteBase<Function, BasicBlock, Value, User, Use, Instruction, CallInst, InvokeInst,
                                     CallBrInst, User::op_iterator> {
 public:
  CallSite() = default;
  CallSite(CallSiteBase B) : CallSiteBase(B) {
  }
  CallSite(CallInst* CI) : CallSiteBase(CI) {
  }
  CallSite(InvokeInst* II) : CallSiteBase(II) {
  }
  CallSite(CallBrInst* CBI) : CallSiteBase(CBI) {
  }
  explicit CallSite(Instruction* II) : CallSiteBase(II) {
  }
  explicit CallSite(Value* V) : CallSiteBase(V) {
  }

  bool operator==(const CallSite& CS) const {
    return I == CS.I;
  }
  bool operator!=(const CallSite& CS) const {
    return I != CS.I;
  }
  bool operator<(const CallSite& CS) const {
    return getInstruction() < CS.getInstruction();
  }

 private:
  friend struct DenseMapInfo<CallSite>;

  User::op_iterator getCallee() const;
};

/// Establish a view to a call site for examination.
class ImmutableCallSite : public CallSiteBase<> {
 public:
  ImmutableCallSite() = default;
  ImmutableCallSite(const CallInst* CI) : CallSiteBase(CI) {
  }
  ImmutableCallSite(const InvokeInst* II) : CallSiteBase(II) {
  }
  ImmutableCallSite(const CallBrInst* CBI) : CallSiteBase(CBI) {
  }
  explicit ImmutableCallSite(const Instruction* II) : CallSiteBase(II) {
  }
  explicit ImmutableCallSite(const Value* V) : CallSiteBase(V) {
  }
  ImmutableCallSite(CallSite CS) : CallSiteBase(CS.getInstruction()) {
  }
};

}  // namespace llvm
#pragma GCC diagnostic pop

#endif  // LLVM_IR_CALLSITE_H