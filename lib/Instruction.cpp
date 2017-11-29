// Copyright (c) 2011-12, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Instruction.h"
#include "InstructionOpcode.h"
#include "InstructionDecode.h"
#include "InstructionDecodeXS1B.h"
#include "InstructionDecodeXS2A.h"
#include "Core.h"
#include "Compiler.h"
#include "ProcessorNode.h"
#include <cassert>

using namespace axe;

void axe::
instructionDecode(const Core &core, uint32_t address,
                  InstructionOpcode &opcode, Operands &operands,
                  bool ignoreBreakpoints)
{
  assert((address & 1) == 0 && core.isValidAddress(address));
  if (!ignoreBreakpoints && core.isBreakpointAddress(address)) {
    opcode = BREAKPOINT;
    return;
  }
  uint16_t low = core.loadShort(address);
  uint16_t high = 0;
  bool highValid;
  if (core.isValidAddress(address + 2)) {
    high = core.loadShort(address + 2);
    highValid = true;
  } else {
    highValid = false;
  }
  //return instructionDecode(low, high, highValid, opcode, operands, core.getParent()->type);
  if (core.getParent()->type == Node::Type::XS2_A) {
    return instructionDecodeXS2A(low, high, highValid, opcode, operands);
  } else {
    return instructionDecodeXS1B(low, high, highValid, opcode, operands);
  }
}

#define PFIX 0x1e /* 0b11110 */
#define EOPR 0x1f /* 0b11111 */

#undef OP
#undef LOP
#undef PC
#undef CHECK_PC
#define OP(n) (operands.ops[n])
#define LOP(n) (operands.lops[n])
#define PC pc
#define CHECK_PC(pc) (decodeCache->isValidPc(pc))

void axe::
instructionTransform(InstructionOpcode &opc, Operands &operands,
                     const Core &core, uint32_t address)
{
  const DecodeCache::State *decodeCache =
    core.getDecodeCacheContaining(address);
  assert(decodeCache);
  uint32_t pc = decodeCache->toPc(address);
  switch (opc) {
  default:
    break;
  case ADD_2rus:
    if (operands.ops[2] == 0) {
      opc = ADD_mov_2rus;
    }
    break;
  case STW_2rus:
  case LDW_2rus:
  case LDAWF_l2rus:
  case LDAWB_l2rus:
    OP(2) = OP(2) << 2;
    break;
  case STWDP_ru6:
  case STWSP_ru6:
  case LDWDP_ru6:
  case LDWSP_ru6:
  case LDAWDP_ru6:
  case LDAWSP_ru6:
  case LDWCP_ru6:
  case STWDP_lru6:
  case STWSP_lru6:
  case LDWDP_lru6:
  case LDWSP_lru6:
  case LDAWDP_lru6:
  case LDAWSP_lru6:
  case LDWCP_lru6:
    OP(1) = OP(1) << 2;
    break;
  case EXTDP_u6:
  case ENTSP_u6:
  case EXTSP_u6:
  case RETSP_u6:
  case KENTSP_u6:
  case KRESTSP_u6:
  case LDAWCP_u6:
  case LDWCPL_u10:
  case EXTDP_lu6:
  case ENTSP_lu6:
  case EXTSP_lu6:
  case RETSP_lu6:
  case KENTSP_lu6:
  case KRESTSP_lu6:
  case LDAWCP_lu6:
  case LDWCPL_lu10:
    OP(0) = OP(0) << 2;
    break;
  case LDAPB_u10:
  case LDAPF_u10:
  case LDAPB_lu10:
  case LDAPF_lu10:
    OP(0) = OP(0) << 1;
    break;
  case SHL_2rus:
    if (OP(2) == 32) {
      opc = SHL_32_2rus;
    }
    break;
  case SHR_2rus:
    if (OP(2) == 32) {
      opc = SHR_32_2rus;
    }
    break;
  case ASHR_l2rus:
    if (OP(2) == 32) {
      opc = ASHR_32_l2rus;
    }
    break;
  // case BRFT_ru6:
  //   OP(1) = PC + 1 + OP(1);
  //   if (!CHECK_PC(OP(1))) {
  //     opc = BRFT_illegal_ru6;
  //   }
  //   break;
  case BRBT_ru6:
    // OP(1) = PC + 1 - OP(1);
    // if (!CHECK_PC(OP(1))) {
    //   opc = BRBT_illegal_ru6;
    // }
    OP(1) = -OP(1);
    break;
  case BRFU_u6:
    OP(0) = PC + 1 + OP(0);
    if (!CHECK_PC(OP(0))) {
      opc = BRFU_illegal_u6;
    }
    break;
  case BRBU_u6:
    OP(0) = PC + 1 - OP(0);
    if (!CHECK_PC(OP(0))) {
      opc = BRBU_illegal_u6;
    }
    break;
  case BRFF_ru6:
    OP(1) = PC + 1 + OP(1);
    if (!CHECK_PC(OP(1))) {
      opc = BRFF_illegal_ru6;
    }
    break;
  case BRBF_ru6:
    OP(1) = PC + 1 - OP(1);
    if (!CHECK_PC(OP(1))) {
      opc = BRBF_illegal_ru6;
    }
    break;
  case BLRB_u10:
    OP(0) = PC + 1 - OP(0);
    if (!CHECK_PC(OP(0))) {
      opc = BLRB_illegal_u10;
    }
    break;
  case BLRF_u10:
    OP(0) = PC + 1 + OP(0);
    if (!CHECK_PC(OP(0))) {
      opc = BLRF_illegal_u10;
    }
    break;
  case BRFT_lru6:
    OP(1) = PC + 2 + OP(1);
    if (!CHECK_PC(OP(1))) {
      opc = BRFT_illegal_lru6;
    }
    break;
  case BRBT_lru6:
    OP(1) = PC + 2 - OP(1);
    if (!CHECK_PC(OP(1))) {
      opc = BRBT_illegal_lru6;
    }
    break;
  case BRFU_lu6:
    OP(0) = PC + 2 + OP(0);
    if (!CHECK_PC(OP(0))) {
      opc = BRFU_illegal_lu6;
    }
    break;
  case BRBU_lu6:
    OP(0) = PC + 2 - OP(0);
    if (!CHECK_PC(OP(0))) {
      opc = BRBU_illegal_lu6;
    }
    break;
  case BRFF_lru6:
    OP(1) = PC + 2 + OP(1);
    if (!CHECK_PC(OP(1))) {
      opc = BRFF_illegal_lru6;
    }
    break;
  case BRBF_lru6:
    OP(1) = PC + 2 - OP(1);
    if (!CHECK_PC(OP(1))) {
      opc = BRBF_illegal_lru6;
    }
    break;
  case BLRB_lu10:
    OP(0) = PC + 2 - OP(0);
    if (!CHECK_PC(OP(0))) {
      opc = BLRB_illegal_lu10;
    }
    break;
  case BLRF_lu10:
    OP(0) = PC + 2 + OP(0);
    if (!CHECK_PC(OP(0))) {
      opc = BLRF_illegal_lu10;
    }
    break;
  case MKMSK_rus:
    OP(1) = makeMask(OP(1));
    break;
  }
}
