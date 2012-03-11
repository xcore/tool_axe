// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _LLVMExtra_h_
#define _LLVMExtra_h_

#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"

#ifdef __cplusplus
extern "C" {
#endif
LLVMMemoryBufferRef
LLVMExtraCreateMemoryBufferWithPtr(const char *ptr, size_t length);

LLVMBool LLVMExtraInlineFunction(LLVMValueRef call);

void LLVMExtraAddDeadCodeEliminationPass(LLVMPassManagerRef PM);

void LLVMExtraRegisterJitDisassembler(LLVMExecutionEngineRef EE,
                                      const char *triple);

#ifdef __cplusplus
} // extern "C"
#endif

#endif //_LLVMExtra_h_
