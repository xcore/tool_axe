// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef AXE_C_axe_h_
#define AXE_C_axe_h_

#ifdef __cplusplus
extern "C" {
#endif

enum AXERegister {
  AXE_REG_R0,
  AXE_REG_R1,
  AXE_REG_R2,
  AXE_REG_R3,
  AXE_REG_R4,
  AXE_REG_R5,
  AXE_REG_R6,
  AXE_REG_R7,
  AXE_REG_R8,
  AXE_REG_R9,
  AXE_REG_R10,
  AXE_REG_R11,
  AXE_REG_CP,
  AXE_REG_DP,
  AXE_REG_SP,
  AXE_REG_LR,
  AXE_REG_PC,
  AXE_REG_SR,
  AXE_REG_SPC,
  AXE_REG_SSR,
  AXE_REG_ET,
  AXE_REG_ED,
  AXE_REG_SED,
  AXE_REG_KEP,
  AXE_REG_KSP
};

enum AXEStopReason {
  AXE_STOP_BREAKPOINT,
  AXE_STOP_TIMEOUT,
  AXE_STOP_EXIT,
  AXE_STOP_NO_RUNNABLE_THREADS
};

enum AXEWatchpointType {
  AXE_WATCHPOINT_READ,
  AXE_WATCHPOINT_WRITE
};

enum AXENodeType {
  AXE_NODE_TYPE_XS1_L   = 0,
  AXE_NODE_TYPE_XS1_G   = 1,
  AXE_NODE_TYPE_UNKNOWN = 100
};

typedef struct AXEOpaqueSystem *AXESystemRef;
typedef struct AXEOpaqueCore *AXECoreRef;
typedef struct AXEOpaqueThread *AXEThreadRef;

void axeRemoveThreadFromRunQueue(AXEThreadRef thread);
void axeAddThreadToRunQueue(AXEThreadRef thread);
void axeClearRunQueue(AXESystemRef system);
int axeIsThreadInRunQueue(AXEThreadRef thread);

AXESystemRef axeCreateInstance(const char *xeFileName, int enabledTracing);
void axeDeleteInstance(AXESystemRef system);
AXECoreRef axeLookupCore(AXESystemRef system, unsigned jtagIndex, unsigned core);

AXENodeType axeGetNodeType(AXESystemRef system, int jtagIndex);
int axeGetNumNodes(AXESystemRef system);
int axeGetNumTiles(AXESystemRef system, int jtagIndex);

int axeGetThreadInUse(AXEThreadRef thread);
int axeGetThreadID(AXEThreadRef thread);
AXECoreRef axeGetThreadParent(AXEThreadRef thread);

int axeWriteMemory(AXECoreRef core, unsigned address, const void *src,unsigned length);
int axeReadMemory(AXECoreRef core, unsigned address, void *dst, unsigned length);

int axeSetBreakpoint(AXECoreRef core, unsigned address);
void axeUnsetBreakpoint(AXECoreRef core, unsigned address);
void axeUnsetAllBreakpoints(AXESystemRef system);
void axeStepThreadOnce(AXEThreadRef thread);

int  axeSetWatchpoint(AXECoreRef core, unsigned int startAddress, unsigned int endAddress);
void axeUnsetWatchpoint(AXECoreRef core, unsigned int startAddress, unsigned int endAddress);
void axeUnsetAllWatchpoints(AXECoreRef core);
  
AXEThreadRef axeLookupThread(AXECoreRef core, unsigned threadNum);
int axeThreadIsInUse(AXEThreadRef thread);
unsigned axeReadReg(AXEThreadRef thread, AXERegister reg);
int axeWriteReg(AXEThreadRef thread, AXERegister reg, unsigned value);

AXEStopReason axeRun(AXESystemRef system, unsigned maxCycles);
AXEThreadRef axeGetThreadForLastBreakpoint(AXESystemRef system);

#ifdef __cplusplus
}

namespace axe {

class SystemStateWrapper;
class Core;
class Thread;

#define DEFINE_WRAP_UNWRAP_FUNCTIONS(type, ref) \
inline type *unwrap(ref p) { \
  return reinterpret_cast<type*>(p); \
} \
inline ref wrap(const type *p) { \
  return reinterpret_cast<ref>(const_cast<type*>(p)); \
}
  
DEFINE_WRAP_UNWRAP_FUNCTIONS(SystemStateWrapper, AXESystemRef)
DEFINE_WRAP_UNWRAP_FUNCTIONS(Core, AXECoreRef)
DEFINE_WRAP_UNWRAP_FUNCTIONS(Thread, AXEThreadRef)

} // End axe namespace

#endif

#endif // AXE_C_axe_h_
