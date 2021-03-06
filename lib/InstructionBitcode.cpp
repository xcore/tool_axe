// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "InstructionBitcode.h"

using namespace axe;

const unsigned char axe::instructionBitcode[] = {
#include "InstructionBitcode.inc"
  , 0x00 // Ensure null termination.
};

/// Size of LLVMModule excluding the terminating null byte.
size_t axe::instructionBitcodeSize = sizeof(instructionBitcode) - 1;
