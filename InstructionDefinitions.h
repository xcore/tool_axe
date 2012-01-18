// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _InstructionDefinitions_h_
#define _InstructionDefinitions_h_

#include <cstdlib>
#include "Thread.h"
#include "Core.h"
#include "BitManip.h"
#include "CRC.h"

#define THREAD thread
#define CORE THREAD.getParent()
//#define ERROR() internalError(THREAD, __FILE__, __LINE__);
#define ERROR() std::abort();
#define OP(n) (field ## n)
#define LOP(n) OP(n)
#define FROM_PC(addr) CORE.virtualAddress((addr) << 1)
#define EMIT_INSTRUCTION_FUNCTIONS
#include "InstructionGenOutput.inc"
#undef EMIT_INSTRUCTION_FUNCTIONS
#undef THREAD
#undef CORE
#undef ERROR
#undef OP
#undef LOP
#undef FROM_PC

#endif // _InstructionDefinitions_h_
