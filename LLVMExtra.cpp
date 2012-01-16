// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "LLVMExtra.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm;

LLVMMemoryBufferRef
LLVMExtraCreateMemoryBufferWithPtr(const char *ptr, size_t length)
{
  return wrap(MemoryBuffer::getMemBuffer(StringRef(ptr, length)));
}
