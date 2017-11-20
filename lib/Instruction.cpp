// Copyright (c) 2011-12, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Instruction.h"
#include "InstructionOpcode.h"
#include "Core.h"
#include "Compiler.h"
#include <cassert>
#include <iostream>

using namespace axe;

static inline uint32_t bitRange(uint32_t value, unsigned high, unsigned low)
{
  return extractBitRange(value, high, low);
}

static inline uint32_t bit(uint32_t value, unsigned shift)
{
  return extractBit(value, shift);
}

// Also tests for 2RUS
static inline bool test3R(uint32_t low)
{
  return bitRange(low, 10, 6) < 27;
}

// Also tests for RUS
static inline bool test2R(uint32_t low)
{
  uint32_t combined = (bitRange(low, 10, 6) - 27) + bit(low, 5) * 5;
  
  return combined < 9;
}

static inline bool test1R(uint32_t low)
{
  return (bitRange(low, 10, 5) == 0x3f) && (bitRange(low, 3, 0) < 12);
}

static inline bool testRU6(uint32_t low)
{
  return bitRange(low, 9, 6) < 12; // 12 = 0b1100
}

static inline bool testRU6_2(UNUSED(uint32_t low))
{
  return true;
}

static inline bool testLRU6(uint32_t high, UNUSED(uint32_t low))
{
  return testRU6(high);
}

static inline bool testLRU6_2(uint32_t high, UNUSED(uint32_t low))
{
  return testRU6_2(high);
}

static inline bool testL2R(uint32_t high, uint32_t low)
{
  uint32_t combined = (bitRange(low, 10, 6) - 27) + bit(low, 5) * 5;
  
  return combined < 9 && (bitRange(high, 10, 4) == 0x7e);
}

// Also tests for L2RUS
static inline bool testL3R(uint32_t high, uint32_t low)
{
  return bitRange(low, 10, 6) < 27 && bitRange(high, 10, 4) == 0x7e;
}

static inline bool testL4R(uint32_t high, uint32_t low)
{
  return bitRange(low, 10, 6) < 27 && bitRange(high, 3, 0) < 12
  && bitRange(high, 10, 5) == 0x3f;
}

static inline bool testL5R(uint32_t high, uint32_t low)
{
  if (bitRange(low, 10, 6) > 26) {
    return false;
  }
  uint32_t combined = (bitRange(high, 10, 6) - 27) + bit(high, 5) * 5;
  
  return combined < 9;
}

static inline bool testL6R(uint32_t high, uint32_t low)
{
  return bitRange(low, 10, 6) < 27 &&
  bitRange(high, 10, 6) < 27;
}

static void decode3ROperands(Operands &operands, uint32_t low)
{
  uint32_t combined = bitRange(low, 10, 6);
  uint32_t op0_high = combined % 3;
  uint32_t op1_high = (combined / 3) % 3;
  uint32_t op2_high = combined / 9;
  operands.ops[0] = bitRange(low, 5, 4) | (op0_high << 2);
  operands.ops[1] = bitRange(low, 3, 2) | (op1_high << 2);
  operands.ops[2] = bitRange(low, 1, 0) | (op2_high << 2);
}

static inline void decodeL3ROperands(Operands &operands, UNUSED(uint32_t high), uint32_t low)
{
  decode3ROperands(operands, low);
}

static inline void decode2RUSOperands(Operands &operands, uint32_t low)
{
  decode3ROperands(operands, low);
}

static inline void decodeL2RUSOperands(Operands &operands, uint32_t high, uint32_t low)
{
  decodeL3ROperands(operands, high, low);
}

static inline void decodeRU6Operands(Operands &operands, uint32_t low)
{
  operands.ops[0] = bitRange(low, 9, 6);
  operands.ops[1] = bitRange(low, 5, 0);
}

static inline void decodeLRU6Operands(Operands &operands, uint32_t high, uint32_t low)
{
  operands.ops[0] = bitRange(high, 9, 6);
  operands.ops[1] = bitRange(high, 5, 0) | (bitRange(low, 9, 0) << 6);
}

static inline void decodeU6Operands(Operands &operands, uint32_t low)
{
  operands.ops[0] = bitRange(low, 5, 0);
}

static inline void decodeLU6Operands(Operands &operands, uint32_t high, uint32_t low)
{
  operands.ops[0] = bitRange(high, 5, 0) | (bitRange(low, 9, 0) << 6);
}

static inline void decodeU10Operands(Operands &operands, uint32_t low)
{
  operands.ops[0] = bitRange(low, 9, 0);
}

static inline void decodeLU10Operands(Operands &operands, uint32_t high, uint32_t low)
{
  operands.ops[0] = bitRange(high, 9, 0) | (bitRange(low, 9, 0) << 10);
}

static void decode2ROperands(Operands &operands, uint32_t low)
{
  uint32_t combined = (bitRange(low, 10, 6) - 27) + bit(low, 5) * 5;
  uint32_t op0_high = combined%3;
  uint32_t op1_high = combined/3;
  operands.ops[0] = bitRange(low, 3, 2) | (op0_high << 2);
  operands.ops[1] = bitRange(low, 1, 0) | (op1_high << 2);
}

static inline void decodeL2ROperands(Operands &operands, UNUSED(uint32_t high), uint32_t low)
{
  decode2ROperands(operands, low);
}

static void decodeRUSOperands(Operands &operands, uint32_t low)
{
  uint32_t combined = (bitRange(low, 10, 6) - 27) + bit(low, 5) * 5;
  uint32_t op0_high = combined%0x3;
  uint32_t op1_high = combined/3;
  operands.ops[0] = bitRange(low, 3, 2) | (op0_high << 2);
  operands.ops[1] = bitRange(low, 1, 0) | (op1_high << 2);
}

static inline void decode1ROperands(Operands &operands, uint32_t low)
{
  operands.ops[0] = bitRange(low, 3, 0);
}

static void decodeL4ROperands(Operands &operands, uint32_t high, uint32_t low)
{
  uint32_t combined = bitRange(low, 10, 6);
  uint32_t op0_high = combined % 3;
  uint32_t op1_high = (combined / 3) % 3;
  uint32_t op2_high = combined / 9;
  operands.lops[0] = bitRange(low, 5, 4) | (op0_high << 2);
  operands.lops[1] = bitRange(low, 3, 2) | (op1_high << 2);
  operands.lops[2] = bitRange(low, 1, 0) | (op2_high << 2);
  operands.lops[3] = bitRange(high, 3, 0);
}

static void decodeL5ROperands(Operands &operands, uint32_t high, uint32_t low)
{
  uint32_t combined_low = bitRange(low, 10, 6);
  uint32_t op0_high = combined_low % 3;
  uint32_t op1_high = (combined_low / 3) % 3;
  uint32_t op2_high = combined_low / 9;
  
  uint32_t combined_high = (bitRange(high, 10, 6) - 27) + bit(high, 5) * 5;
  uint32_t op3_high = combined_high%3;
  uint32_t op4_high = combined_high/3;
  
  operands.lops[0] = bitRange(low, 5, 4) | (op0_high << 2);
  operands.lops[1] = bitRange(low, 3, 2) | (op1_high << 2);
  operands.lops[2] = bitRange(low, 1, 0) | (op2_high << 2);
  operands.lops[3] = bitRange(high, 3, 2) | (op3_high << 2);
  operands.lops[4] = bitRange(high, 1, 0) | (op4_high << 2);
}

static void decodeL6ROperands(Operands &operands, uint32_t high, uint32_t low)
{
  uint32_t combined_low = bitRange(low, 10, 6);
  uint32_t op0_high = combined_low % 3;
  uint32_t op1_high = (combined_low / 3) % 3;
  uint32_t op2_high = combined_low / 9;
  
  uint32_t combined_high = bitRange(high, 10, 6);
  uint32_t op3_high = combined_high % 3;
  uint32_t op4_high = (combined_high / 3) % 3;
  uint32_t op5_high = combined_high / 9;
  
  operands.lops[0] = bitRange(low, 5, 4) | (op0_high << 2);
  operands.lops[1] = bitRange(low, 3, 2) | (op1_high << 2);
  operands.lops[2] = bitRange(low, 1, 0) | (op2_high << 2);
  operands.lops[3] = bitRange(high, 5, 4) | (op3_high << 2);
  operands.lops[4] = bitRange(high, 3, 2) | (op4_high << 2);
  operands.lops[5] = bitRange(high, 1, 0) | (op5_high << 2);
}

#define DECODE_3R(inst, low) \
do { \
opcode = inst; \
decode3ROperands(operands, low); \
} while(0)
#define DECODE_2RUS(inst, low) \
do { \
opcode = inst; \
decode2RUSOperands(operands, low); \
} while(0)
#define DECODE_RU6(inst, low) \
do { \
opcode = inst; \
decodeRU6Operands(operands, low); \
} while(0)
#define DECODE_LRU6(inst, high, low) \
do { \
opcode = inst; \
decodeLRU6Operands(operands, high, low); \
} while(0)
#define DECODE_2R(inst, low) \
do { \
opcode = inst; \
decode2ROperands(operands, low); \
} while(0)
#define DECODE_RUS(inst, low) \
do { \
opcode = inst; \
decodeRUSOperands(operands, low); \
} while(0)
#define DECODE_1R(inst, low) \
do { \
opcode = inst; \
decode1ROperands(operands, low); \
} while(0)
#define DECODE_U6(inst, low) \
do { \
opcode = inst; \
decodeU6Operands(operands, low); \
} while(0)
#define DECODE_LU6(inst, high, low) \
do { \
opcode = inst; \
decodeLU6Operands(operands, high, low); \
} while(0)
#define DECODE_U10(inst, low) \
do { \
opcode = inst; \
decodeU10Operands(operands, low); \
} while(0)
#define DECODE_LU10(inst, high, low) \
do { \
opcode = inst; \
decodeLU10Operands(operands, high, low); \
} while(0)
#define DECODE_0R(inst) \
do { \
opcode = inst; \
} while(0)
#define DECODE_L2R(inst, high, low) \
do { \
opcode = inst; \
decodeL2ROperands(operands, high, low); \
} while(0)
#define DECODE_L3R(inst, high, low) \
do { \
opcode = inst; \
decodeL3ROperands(operands, high, low); \
} while(0)
#define DECODE_L2RUS(inst, high, low) \
do { \
opcode = inst; \
decodeL2RUSOperands(operands, high, low); \
} while(0)
#define DECODE_L4R(inst, high, low) \
do { \
opcode = inst; \
decodeL4ROperands(operands, high, low); \
} while(0)
#define DECODE_L5R(inst, high, low) \
do { \
opcode = inst; \
decodeL5ROperands(operands, high, low); \
} while(0)
#define DECODE_L6R(inst, high, low) \
do { \
opcode = inst; \
decodeL6ROperands(operands, high, low); \
} while(0)

#define OP(n) (operands.ops[(n)])

static unsigned bitpValue(unsigned Value)
{
  assert(Value < 12 && "Invalid bitp immediate");
  static unsigned bitpValues[12] = {
    32,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    16,
    24,
    32
  };
  return bitpValues[Value];
}

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
  return instructionDecode(low, high, highValid, opcode, operands);
}

#define PFIX 0x1e /* 0b11110 */
#define EOPR 0x1f /* 0b11111 */

void case_0x00(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b00000 */
{
  if (test3R(low)) {
    DECODE_2RUS(STW_2rus, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(BYTEREV_2r, low);
        break;
      case 1:
        DECODE_2R(GETST_2r, low);
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(EDU_1r, low);
        break;
      case 1:
        DECODE_1R(EEU_1r, low);
        break;
    }
  } else {
    switch (low) {
      case 0x07ec:
        DECODE_0R(WAITEU_0r);
        break;
      case 0x07ed:
        DECODE_0R(CLRE_0r);
        break;
      case 0x07ee:
        DECODE_0R(SSYNC_0r);
        break;
      case 0x07ef:
        DECODE_0R(FREET_0r);
        break;
      case 0x07fc:
        DECODE_0R(DCALL_0r);
        break;
      case 0x07fd:
        DECODE_0R(KRET_0r);
        break;
      case 0x07fe:
        DECODE_0R(DRET_0r);
        break;
      case 0x07ff:
        DECODE_0R(SETKEP_0r);
        break;
    }
  }
}

void case_0x01(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b00001 */
{
  if (test3R(low)) {
    DECODE_2RUS(LDW_2rus, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(TINITDP_2r, low);
        break;
      case 1:
        DECODE_2R(OUTT_2r, low);
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(WAITET_1r, low);
        break;
      case 1:
        DECODE_1R(WAITEF_1r, low);
        break;
    }
  } else {
    switch (low) {
      case 0x0fec:
        DECODE_0R(LDSPC_0r);
        break;
      case 0x0fed:
        DECODE_0R(STSPC_0r);
        break;
      case 0x0fee:
        DECODE_0R(LDSSR_0r);
        break;
      case 0x0fef:
        DECODE_0R(STSSR_0r);
        break;
      case 0x0ffc:
        DECODE_0R(STSED_0r);
        break;
      case 0x0ffd:
        DECODE_0R(STET_0r);
        break;
      case 0x0ffe:
        DECODE_0R(GETED_0r);
        break;
      case 0x0fff:
        DECODE_0R(GETET_0r);
        break;
    }
  }
}

void case_0x02(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b00010 */
{
  if (test3R(low)) {
    DECODE_3R(ADD_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(TINITSP_2r, low);
        break;
      case 1:
        DECODE_2R(SETD_2r, low);
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(FREER_1r, low);
        break;
      case 1:
        DECODE_1R(MJOIN_1r, low);
        break;
    }
  } else {
    switch (low) {
      case 0x17ec:
        DECODE_0R(DENTSP_0r);
        break;
      case 0x17ed:
        DECODE_0R(DRESTSP_0r);
        break;
      case 0x17ee:
        DECODE_0R(GETID_0r);
        break;
      case 0x17ef:
        DECODE_0R(GETKEP_0r);
        break;
      case 0x17fc:
        DECODE_0R(GETKSP_0r);
        break;
      case 0x17fd:
        DECODE_0R(LDSED_0r);
        break;
      case 0x17fe:
        DECODE_0R(LDET_0r);
        break;
      case 0x17ff:
        DECODE_0R(NOP_0r);
        break;
    }
  }
}

void case_0x03(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b00011 */
{
  if (test3R(low)) {
    DECODE_3R(SUB_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(TINITCP_2r, low);
        break;
      case 1:
        DECODE_2R(TSETMR_2r, low);
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(TSTART_1r, low);
        break;
      case 1:
        DECODE_1R(MSYNC_1r, low);
        break;
    }
  }
}

void case_0x04(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b00100 */
{
  if (test3R(low)) {
    DECODE_3R(SHL_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        break;
      case 1:
        DECODE_2R(EET_2r, low);
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(BLA_1r, low);
        break;
      case 1:
        DECODE_1R(BAU_1r, low);
        break;
    }
  }
}

void case_0x05(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b00101 */
{
  if (test3R(low)) {
    DECODE_3R(SHR_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(ANDNOT_2r, low);
        break;
      case 1:
        DECODE_2R(EEF_2r, low);
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(BRU_1r, low);
        break;
      case 1:
        DECODE_1R(SETSP_1r, low);
        break;
    }
  }
}

void case_0x06(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b00110 */
{
  if (test3R(low)) {
    DECODE_3R(EQ_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(SEXT_2r, low);
        break;
      case 1:
        DECODE_RUS(SEXT_rus, low);
        OP(1) = bitpValue(OP(1));
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(SETDP_1r, low);
        break;
      case 1:
        DECODE_1R(SETCP_1r, low);
        break;
    }
  }
}

void case_0x07(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b00111 */
{
  if (test3R(low)) {
    DECODE_3R(AND_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(GETTS_2r, low);
        break;
      case 1:
        DECODE_2R(SETPT_2r, low);
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(DGETREG_1r, low);
        break;
      case 1:
        DECODE_1R(SETEV_1r, low);
        break;
    }
  }
}

void case_0x08(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b01000 */
{
  if (test3R(low)) {
    DECODE_3R(OR_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(ZEXT_2r, low);
        break;
      case 1:
        DECODE_RUS(ZEXT_rus, low);
        OP(1) = bitpValue(OP(1));
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(KCALL_1r, low);
        break;
      case 1:
        DECODE_1R(SETV_1r, low);
        break;
    }
  }
}

void case_0x09(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b01001 */
{
  if (test3R(low)) {
    DECODE_3R(LDW_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(OUTCT_2r, low);
        break;
      case 1:
        DECODE_RUS(OUTCT_rus, low);
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(ECALLF_1r, low);
        break;
      case 1:
        DECODE_1R(ECALLT_1r, low);
        break;
    }
  }
}

void case_0x0a(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b01010 */
{
  if (testRU6_2(low)) {
    switch (bit(low, 10)) {
      case 0:
        DECODE_RU6(STWDP_ru6, low);
        break;
      case 1:
        DECODE_RU6(STWSP_ru6, low);
        break;
    }
  }
}

void case_0x0b(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b01011 */
{
  if (testRU6_2(low)) {
    switch (bit(low, 10)) {
      case 0:
        DECODE_RU6(LDWDP_ru6, low);
        break;
      case 1:
        DECODE_RU6(LDWSP_ru6, low);
        break;
    }
  }
}

void case_0x0c(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b01100 */
{
  if (testRU6_2(low)) {
    switch (bit(low, 10)) {
      case 0:
        DECODE_RU6(LDAWDP_ru6, low);
        break;
      case 1:
        DECODE_RU6(LDAWSP_ru6, low);
        break;
    }
  }
}

void case_0x0d(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b01101 */
{
  if (testRU6_2(low)) {
    switch (bit(low, 10)) {
      case 0:
        DECODE_RU6(LDC_ru6, low);
        break;
      case 1:
        DECODE_RU6(LDWCP_ru6, low);
        break;
    }
  }
}

void case_0x0e(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b01110 */
{
  if (testRU6(low)) {
    switch (bit(low, 10)) {
      case 0:
        DECODE_RU6(BRFT_ru6, low);
        break;
      case 1:
        DECODE_RU6(BRBT_ru6, low);
        break;
    }
  } else {
    switch (bitRange(low, 10, 6)) {
      case 0x0c:
        DECODE_U6(BRFU_u6, low);
        break;
      case 0x0d:
        DECODE_U6(BLAT_u6, low);
        break;
      case 0x0e:
        DECODE_U6(EXTDP_u6, low);
        break;
      case 0x0f:
        DECODE_U6(KCALL_u6, low);
        break;
      case 0x1c:
        DECODE_U6(BRBU_u6, low);
        break;
      case 0x1d:
        DECODE_U6(ENTSP_u6, low);
        break;
      case 0x1e:
        DECODE_U6(EXTSP_u6, low);
        break;
      case 0x1f:
        DECODE_U6(RETSP_u6, low);
        break;
    }
  }
}

void case_0x0f(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b01111 */
{
  if (testRU6(low)) {
    switch (bit(low, 10)) {
      case 0:
        DECODE_RU6(BRFF_ru6, low);
        break;
      case 1:
        DECODE_RU6(BRBF_ru6, low);
        break;
    }
  } else {
    switch (bitRange(low, 10, 6)) {
      case 0x0c:
        DECODE_U6(CLRSR_u6, low);
        break;
      case 0x0d:
        DECODE_U6(SETSR_u6, low);
        break;
      case 0x0e:
        DECODE_U6(KENTSP_u6, low);
        break;
      case 0x0f:
        DECODE_U6(KRESTSP_u6, low);
        break;
      case 0x1c:
        DECODE_U6(GETSR_u6, low);
        break;
      case 0x1d:
        DECODE_U6(LDAWCP_u6, low);
        break;
      case 0x1e:
        DECODE_U6(DUALENTSP_u6, low);
        return;
    }
  }
}

void case_0x10(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b10000 */
{
  if (test3R(low)) {
    DECODE_3R(LD16S_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_RUS(GETR_rus, low);
        break;
      case 1:
        DECODE_2R(INCT_2r, low);
        break;
    }
  } else if (test1R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_1R(CLRPT_1r, low);
        break;
      case 1:
        DECODE_1R(SYNCR_1r, low);
        break;
    }
  }
}

void case_0x11(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b10001 */
{
  if (test3R(low)) {
    DECODE_3R(LD8U_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(NOT_2r, low);
        break;
      case 1:
        DECODE_2R(INT_2r, low);
        break;
    }
  }
}

void case_0x12(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b10010 */
{
  if (test3R(low)) {
    DECODE_2RUS(ADD_2rus, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(NEG_2r, low);
        break;
      case 1:
        DECODE_2R(ENDIN_2r, low);
        break;
    }
  }
}

void case_0x13(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b10011 */
{
  if (test3R(low)) {
    DECODE_2RUS(SUB_2rus, low);
  }
}

void case_0x14(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b10100 */
{
  if (test3R(low)) {
    DECODE_2RUS(SHL_2rus, low);
    OP(2) = bitpValue(OP(2));
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(MKMSK_2r, low);
        break;
      case 1:
        DECODE_RUS(MKMSK_rus, low);
        OP(1) = bitpValue(OP(1));
        break;
    }
  }
}

void case_0x15(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b10101 */
{
  if (test3R(low)) {
    DECODE_2RUS(SHR_2rus, low);
    OP(2) = bitpValue(OP(2));
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(OUT_2r, low);
        break;
      case 1:
        DECODE_2R(OUTSHR_2r, low);
        break;
    }
  }
}

void case_0x16(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b10110 */
{
  if (test3R(low)) {
    DECODE_2RUS(EQ_2rus, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(IN_2r, low);
        break;
      case 1:
        DECODE_2R(INSHR_2r, low);
        break;
    }
  }
}

void case_0x17(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b10111 */
{
  if (test3R(low)) {
    DECODE_3R(TSETR_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(PEEK_2r, low);
        break;
      case 1:
        DECODE_2R(TESTCT_2r, low);
        break;
    }
  }
}

void case_0x18(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b11000 */
{
  if (test3R(low)) {
    DECODE_3R(LSS_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(SETPSC_2r, low);
        break;
      case 1:
        DECODE_2R(TESTWCT_2r, low);
        break;
    }
  }
}

void case_0x19(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b11001 */
{
  if (test3R(low)) {
    DECODE_3R(LSU_3r, low);
  } else if (test2R(low)) {
    switch (bit(low, 4)) {
      case 0:
        DECODE_2R(CHKCT_2r, low);
        break;
      case 1:
        DECODE_RUS(CHKCT_rus, low);
        break;
    }
  }
}

void case_0x1a(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b11010 */
{
  if (bit(low, 10)) {
    DECODE_U10(BLRB_u10, low);
  } else {
    DECODE_U10(BLRF_u10, low);
  }
}

void case_0x1b(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b11011 */
{
  if (bit(low, 10)) {
    DECODE_U10(LDAPB_u10, low);
  } else {
    DECODE_U10(LDAPF_u10, low);
  }
}

void case_0x1c(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b11100 */
{
  if (bit(low, 10)) {
    DECODE_U10(LDWCPL_u10, low);
  } else {
    DECODE_U10(BLACP_u10, low);
  }
}

void case_0x1d(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) /* 0b11101 */
{
  if (testRU6(low)) {
    if (bit(low, 10) == 0) {
      DECODE_RU6(SETC_ru6, low);
    }
  }
}

void case_PFIX(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands)
{
  if (!highValid) {
    opcode = ILLEGAL_PC;
    return;
  }
  uint32_t high_opc = bitRange(high, 15, 11);
  switch (high_opc) {
    case 0x0a: /* 0b01010 */
      if (testLRU6_2(high, low)) {
        switch (bit(high, 10)) {
          case 0:
            DECODE_LRU6(STWDP_lru6, high, low);
            break;
          case 1:
            DECODE_LRU6(STWSP_lru6, high, low);
            break;
        }
      }
      break;
    case 0x0b: /* 0b01011 */
      if (testLRU6_2(high, low)) {
        switch (bit(high, 10)) {
          case 0:
            DECODE_LRU6(LDWDP_lru6, high, low);
            break;
          case 1:
            DECODE_LRU6(LDWSP_lru6, high, low);
            break;
        }
      }
      break;
    case 0x0c: /* 0b01100 */
      if (testLRU6_2(high, low)) {
        switch (bit(high, 10)) {
          case 0:
            DECODE_LRU6(LDAWDP_lru6, high, low);
            break;
          case 1:
            DECODE_LRU6(LDAWSP_lru6, high, low);
            break;
        }
      }
      break;
    case 0x0d: /* 0b01101 */
      if (testLRU6_2(high, low)) {
        switch (bit(high, 10)) {
          case 0:
            DECODE_LRU6(LDC_lru6, high, low);
            break;
          case 1:
            DECODE_LRU6(LDWCP_lru6, high, low);
            break;
        }
      }
      break;
    case 0x0e: /* 0b01110 */
      if (testLRU6(high, low)) {
        switch (bit(high, 10)) {
          case 0:
            DECODE_LRU6(BRFT_lru6, high, low);
            break;
          case 1:
            DECODE_LRU6(BRBT_lru6, high, low);
            break;
        }
      } else {
        switch (bitRange(high, 10, 6)) {
          case 0x0c:
            DECODE_LU6(BRFU_lu6, high, low);
            break;
          case 0x0d:
            DECODE_LU6(BLAT_lu6, high, low);
            break;
          case 0x0e:
            DECODE_LU6(EXTDP_lu6, high, low);
            break;
          case 0x0f:
            DECODE_LU6(KCALL_lu6, high, low);
            break;
          case 0x1c:
            DECODE_LU6(BRBU_lu6, high, low);
            break;
          case 0x1d:
            DECODE_LU6(ENTSP_lu6, high, low);
            break;
          case 0x1e:
            DECODE_LU6(EXTSP_lu6, high, low);
            break;
          case 0x1f:
            DECODE_LU6(RETSP_lu6, high, low);
            break;
        }
      }
      break;
    case 0x0f: /* 0b01111 */
      if (testLRU6(high, low)) {
        switch (bit(high, 10)) {
          case 0:
            DECODE_LRU6(BRFF_lru6, high, low);
            break;
          case 1:
            DECODE_LRU6(BRBF_lru6, high, low);
            break;
        }
      } else {
        switch (bitRange(high, 10, 6)) {
          case 0x0c:
            DECODE_LU6(CLRSR_lu6, high, low);
            break;
          case 0x0d:
            DECODE_LU6(SETSR_lu6, high, low);
            break;
          case 0x0e:
            DECODE_LU6(KENTSP_lu6, high, low);
            break;
          case 0x0f:
            DECODE_LU6(KRESTSP_lu6, high, low);
            break;
          case 0x1c:
            DECODE_LU6(GETSR_lu6, high, low);
            break;
          case 0x1d:
            DECODE_LU6(LDAWCP_lu6, high, low);
            break;
        }
      }
      break;
    case 0x1a: /* 0b11010 */
      if (bit(high, 10)) {
        DECODE_LU10(BLRB_lu10, high, low);
      } else {
        DECODE_LU10(BLRF_lu10, high, low);
      }
      break;
    case 0x1b: /* 0b11011 */
      if (bit(high, 10)) {
        DECODE_LU10(LDAPB_lu10, high, low);
      } else {
        DECODE_LU10(LDAPF_lu10, high, low);
      }
      break;
    case 0x1c: /* 0b11100 */
      if (bit(high, 10)) {
        DECODE_LU10(LDWCPL_lu10, high, low);
      } else {
        DECODE_LU10(BLACP_lu10, high, low);
      }
      break;
    case 0x1d: /* 0b11101 */
      if (testLRU6(high, low)) {
        if (bit(high, 10) == 0) {
          DECODE_LRU6(SETC_lru6, high, low);
          break;
        }
      }
      break;
  }
}

void case_EOPR(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands)
{
  if (!highValid) {
    opcode = ILLEGAL_PC;
    return;
  }
  uint32_t high_opc = bitRange(high, 15, 11);
  switch (high_opc) {
    case 0x00: /* 0b00000 */
      if (testL6R(high, low)) {
        DECODE_L6R(LMUL_l6r, high, low);
      } else if (testL5R(high, low)) {
        if (bit(high, 4)) {
          DECODE_L5R(LADD_l5r, high, low);
        } else {
          DECODE_L5R(LDIVU_l5r, high, low);
        }
      } else if (testL4R(high, low)) {
        switch (bitRange(high, 10, 4)) {
          case 0x7e:
            DECODE_L4R(CRC8_l4r, high, low);
            break;
          case 0x7f:
            DECODE_L4R(MACCU_l4r, high, low);
            break;
        }
      } else if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(STW_l3r, high, low);
            break;
        }
      } else if (testL2R(high, low)) {
        switch ((bit(low, 4) << 4) | bitRange(high, 3, 0)) {
          case 0x0c:
            DECODE_L2R(BITREV_l2r, high, low);
            break;
          case 0x1c:
            DECODE_L2R(TINITPC_l2r, high, low);
            break;
        }
      }
      break;
    case 0x01: /* 0b00001 */
      if (testL5R(high, low) && !bit(high, 4)) {
        DECODE_L5R(LSUB_l5r, high, low);
      } 
      if (testL4R(high, low)) {
        switch (bitRange(high, 10, 4)) {
          case 0x7e:
            DECODE_L4R(MACCS_l4r, high, low);
            break;
        }
      } else if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(XOR_l3r, high, low);
            break;
        }
      } else if (testL2R(high, low)) {
        switch ((bit(low, 4) << 4) | bitRange(high, 3, 0)) {
          case 0x0c:
            DECODE_L2R(CLZ_l2r, high, low);
            break;
          case 0x1c:
            DECODE_L2R(SETCLK_l2r, high, low);
            break;
        }
      }
      break;
    case 0x02: /* 0b00010 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(ASHR_l3r, high, low);
            break;
        }
      } else if (testL2R(high, low)) {
        switch ((bit(low, 4) << 4) | bitRange(high, 3, 0)) {
          case 0x0c:
            DECODE_L2R(TINITLR_l2r, high, low);
            break;
          case 0x1c:
            DECODE_L2R(GETPS_l2r, high, low);
            break;
        }
      }
      break;
    case 0x03: /* 0b00011 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(LDAWF_l3r, high, low);
            break;
        }
      } else if (testL2R(high, low)) {
        switch ((bit(low, 4) << 4) | bitRange(high, 3, 0)) {
          case 0x0c:
            DECODE_L2R(SETPS_l2r, high, low);
            break;
          case 0x1c:
            DECODE_L2R(GETD_l2r, high, low);
            break;
        }
      }
      break;
    case 0x04: /* 0b00100 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(LDAWB_l3r, high, low);
            break;
        }
      } else if (testL2R(high, low)) {
        switch ((bit(low, 4) << 4) | bitRange(high, 3, 0)) {
          case 0x0c:
            DECODE_L2R(TESTLCL_l2r, high, low);
            break;
          case 0x1c:
            DECODE_L2R(SETTW_l2r, high, low);
            break;
        }
      }
      break;
    case 0x05: /* 0b00101 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(LDA16F_l3r, high, low);
            break;
        }
      } else if (testL2R(high, low)) {
        switch ((bit(low, 4) << 4) | bitRange(high, 3, 0)) {
          case 0x0c:
            DECODE_L2R(SETRDY_l2r, high, low);
            break;
          case 0x1c:
            DECODE_L2R(SETC_l2r, high, low);
            break;
        }
      }
      break;
    case 0x06: /* 0b00110 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(LDA16B_l3r, high, low);
            break;
        }
      } else if (testL2R(high, low)) {
        switch ((bit(low, 4) << 4) | bitRange(high, 3, 0)) {
          case 0x0c:
            DECODE_L2R(SETN_l2r, high, low);
            break;
          case 0x1c:
            DECODE_L2R(GETN_l2r, high, low);
            break;
        }
      }
      break;
    case 0x07: /* 0b00111 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(MUL_l3r, high, low);
            break;
        }
      }
      break;
    case 0x08: /* 0b01000 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(DIVS_l3r, high, low);
            break;
        }
      }
      break;
    case 0x09: /* 0b01001 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(DIVU_l3r, high, low);
            break;
        }
      }
      break;
    case 0x10: /* 0b10000 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(ST16_l3r, high, low);
            break;
        }
      }
      break;
    case 0x11: /* 0b10001 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(ST8_l3r, high, low);
            break;
        }
      }
      break;
    case 0x12: /* 0b10010 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L2RUS(ASHR_l2rus, high, low);
            OP(2) = bitpValue(OP(2));
            break;
          case 0xd:
            DECODE_L2RUS(OUTPW_l2rus, high, low);
            OP(2) = bitpValue(OP(2));
            break;
          case 0xe:
            DECODE_L2RUS(INPW_l2rus, high, low);
            OP(2) = bitpValue(OP(2));
            break;
        }
      }
      break;
    case 0x13: /* 0b10011 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L2RUS(LDAWF_l2rus, high, low);
            break;
        }
      }
      break;
    case 0x14: /* 0b10100 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L2RUS(LDAWB_l2rus, high, low);
            break;
        }
      }
      break;
    case 0x15: /* 0b10101 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(CRC_l3r, high, low);
            break;
        }
      }
      break;
    case 0x18: /* 0b11000 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(REMS_l3r, high, low);
            break;
        }
      }
      break;
    case 0x19: /* 0b11001 */
      if (testL3R(high, low)) {
        switch (bitRange(high, 3, 0)) {
          case 0xc:
            DECODE_L3R(REMU_l3r, high, low);
            break;
        }
      }
      break;
  }
}

void axe::
instructionDecode(uint16_t low, uint16_t high, bool highValid,
                  InstructionOpcode &opcode, Operands &operands) {
  /* bits 15:11 */
  unsigned opc = bitRange(low, 15, 11);
  switch (opc) {
    case 0x00: /* 0b00000 */
      case_0x00(low, high, highValid, opcode, operands);
      break;
    case 0x01: /* 0b00001 */
      case_0x01(low, high, highValid, opcode, operands);
      break;
    case 0x02: /* 0b00010 */
      case_0x02(low, high, highValid, opcode, operands);
      break;
    case 0x03: /* 0b00011 */
      case_0x03(low, high, highValid, opcode, operands);
      break;
    case 0x04: /* 0b00100 */
      case_0x04(low, high, highValid, opcode, operands);
      break;
    case 0x05: /* 0b00101 */
      case_0x05(low, high, highValid, opcode, operands);
      break;
    case 0x06: /* 0b00110 */
      case_0x06(low, high, highValid, opcode, operands);
      break;
    case 0x07: /* 0b00111 */
      case_0x07(low, high, highValid, opcode, operands);
      break;
    case 0x08: /* 0b01000 */
      case_0x08(low, high, highValid, opcode, operands);
      break;
    case 0x09: /* 0b01001 */
      case_0x09(low, high, highValid, opcode, operands);
      break;
    case 0x0a: /* 0b01010 */
      case_0x0a(low, high, highValid, opcode, operands);
      break;
    case 0x0b: /* 0b01011 */
      case_0x0b(low, high, highValid, opcode, operands);
      break;
    case 0x0c: /* 0b01100 */
      case_0x0c(low, high, highValid, opcode, operands);
      break;
    case 0x0d: /* 0b01101 */
      case_0x0d(low, high, highValid, opcode, operands);
      break;
    case 0x0e: /* 0b01110 */
      case_0x0e(low, high, highValid, opcode, operands);
      break;
    case 0x0f: /* 0b01111 */
      case_0x0f(low, high, highValid, opcode, operands);
      break;
    case 0x10: /* 0b10000 */
      case_0x10(low, high, highValid, opcode, operands);
      break;
    case 0x11: /* 0b10001 */
      case_0x11(low, high, highValid, opcode, operands);
      break;
    case 0x12: /* 0b10010 */
      case_0x12(low, high, highValid, opcode, operands);
      break;
    case 0x13: /* 0b10011 */
      case_0x13(low, high, highValid, opcode, operands);
      break;
    case 0x14: /* 0b10100 */
      case_0x14(low, high, highValid, opcode, operands);
      break;
    case 0x15: /* 0b10101 */
      case_0x15(low, high, highValid, opcode, operands);
      break;
    case 0x16: /* 0b10110 */
      case_0x16(low, high, highValid, opcode, operands);
      break;
    case 0x17: /* 0b10111 */
      case_0x17(low, high, highValid, opcode, operands);
      break;
    case 0x18: /* 0b11000 */
      case_0x18(low, high, highValid, opcode, operands);
      break;
    case 0x19: /* 0b11001 */
      case_0x19(low, high, highValid, opcode, operands);
      break;
    case 0x1a: /* 0b11010 */
      case_0x1a(low, high, highValid, opcode, operands);
      break;
    case 0x1b: /* 0b11011 */
      case_0x1b(low, high, highValid, opcode, operands);
      break;
    case 0x1c: /* 0b11100 */
      case_0x1c(low, high, highValid, opcode, operands);
      break;
    case 0x1d: /* 0b11101 */
      case_0x1d(low, high, highValid, opcode, operands);
      break;
    case PFIX:
      if (bit(low, 10) == 0) {
        case_PFIX(low, high, highValid, opcode, operands);
        break;
      }
    case EOPR:
      case_EOPR(low, high, highValid, opcode, operands); 
      break;
  }
}

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
