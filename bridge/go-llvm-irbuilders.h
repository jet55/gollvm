//===-- go-llvm-cabi-irbuilders.h - IR builder helper classes -------------===//
//
// Copyright 2018 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
//
//===----------------------------------------------------------------------===//
//
// Assorted helper classes for IR building.
//
//===----------------------------------------------------------------------===//

#ifndef LLVMGOFRONTEND_GO_LLVM_IRBUILDER_H
#define LLVMGOFRONTEND_GO_LLVM_IRBUILDER_H

#include "go-llvm-bexpression.h"

#include "llvm/IR/IRBuilder.h"

class NameGen;

// Generic "no insert" builder
typedef llvm::IRBuilder<> LIRBuilder;

// Insertion helper for Bexpressions; inserts any instructions
// created by IRBuilder into the specified Bexpression's inst list.

class BexprInserter : public llvm::IRBuilderDefaultInserter {
 public:
  BexprInserter() : expr_(nullptr) { }
  void setDest(Bexpression *expr) { assert(!expr_); expr_ = expr; }

  void InsertHelper(llvm::Instruction *I, const llvm::Twine &Name,
                    llvm::BasicBlock *BB,
                    llvm::BasicBlock::iterator InsertPt) const {
    assert(expr_);
    expr_->appendInstruction(I);
    I->setName(Name);
  }

 private:
    mutable Bexpression *expr_;
};

// Builder that appends to a specified Bexpression

class BexprLIRBuilder :
    public llvm::IRBuilder<llvm::ConstantFolder, BexprInserter> {
  typedef llvm::IRBuilder<llvm::ConstantFolder, BexprInserter> IRBuilderB;
 public:
  BexprLIRBuilder(llvm::LLVMContext &context, Bexpression *expr) :
      IRBuilderB(context, llvm::ConstantFolder(), getInserter(), nullptr, llvm::None) {
    getInserter().setDest(expr);
  }
};

// Similar to the above, but adds to a Binstructions object.

class BinstructionsInserter : public llvm::IRBuilderDefaultInserter {
 public:
  BinstructionsInserter() : insns_(nullptr) { }
  void setDest(Binstructions *insns) { assert(!insns_); insns_ = insns; }

  void InsertHelper(llvm::Instruction *I, const llvm::Twine &Name,
                    llvm::BasicBlock *BB,
                    llvm::BasicBlock::iterator InsertPt) const {
    assert(insns_);
    insns_->appendInstruction(I);
    I->setName(Name);
  }

 private:
    mutable Binstructions *insns_;
};

// Builder that appends to a specified Binstructions object

class BinstructionsLIRBuilder :
    public llvm::IRBuilder<llvm::ConstantFolder, BinstructionsInserter> {
  typedef llvm::IRBuilder<llvm::ConstantFolder,
                          BinstructionsInserter> IRBuilderB;
 public:
  BinstructionsLIRBuilder(llvm::LLVMContext &context, Binstructions *insns) :
      IRBuilderB(context, llvm::ConstantFolder(), getInserter(), nullptr, llvm::None) {
    getInserter().setDest(insns);
  }
};

// Some of the methods in the LLVM IRBuilder class (ex: CreateMemCpy)
// assume that you are appending to an existing basic block (which is
// typically not what we want to do in many cases in the bridge code).
// Furthermore it is required that the block in question be already
// parented within an llvm::Function (the IRBuilder code relies on
// being able to call getParent() to trace back up to the enclosing
// context).
//
// This builder works around this issue by creating a dummy basic
// block to capture any instructions generated, then when the builder
// is destroyed it detaches the instructions from the block so that
// they can be returned in a list. A dummy function has to be passed in
// as well in order to host the dummy BB, however the dummy block will
// be detached in the destructor.

class BlockLIRBuilder : public LIRBuilder {
 public:
  BlockLIRBuilder(llvm::Function *dummyFcn, NameGen *namegen);
  ~BlockLIRBuilder();

  // Return the instructions generated by this builder. Note that
  // this detaches them from the dummy block we emitted them into,
  // hence is not intended to be invoked more than once.
  std::vector<llvm::Instruction*> instructions();

 private:
  std::unique_ptr<llvm::BasicBlock> dummyBlock_;
  NameGen *namegen_;
};

#endif // LLVMGOFRONTEND_GO_LLVM_IRBUILDER_H
