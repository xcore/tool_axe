// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "LLVMExtra.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/PassManager.h"

using namespace llvm;

LLVMMemoryBufferRef
LLVMExtraCreateMemoryBufferWithPtr(const char *ptr, size_t length)
{
  return wrap(MemoryBuffer::getMemBuffer(StringRef(ptr, length)));
}

LLVMBool LLVMExtraInlineFunction(LLVMValueRef call)
{
  InlineFunctionInfo IFI;
  return InlineFunction(CallSite(unwrap(call)), IFI);
}

void LLVMExtraAddDeadCodeEliminationPass(LLVMPassManagerRef PM) {
  unwrap(PM)->add(createDeadCodeEliminationPass());
}
