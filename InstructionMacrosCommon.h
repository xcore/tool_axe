// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _InstructionMacrosCommon_h_
#define _InstructionMacrosCommon_h_

#define LOAD_WORD(addr) CORE.loadWord(addr)
#define LOAD_SHORT(addr) CORE.loadShort(addr)
#define LOAD_BYTE(addr) CORE.loadByte(addr)
#define LOAD_ROM_WORD(addr) CORE.loadRomWord(addr)
#define LOAD_ROM_SHORT(addr) CORE.loadRomShort(addr)
#define LOAD_ROM_BYTE(addr) CORE.loadRomByte(addr)
#define INVALIDATE_WORD(addr) CORE.invalidateWord(addr)
#define INVALIDATE_SHORT(addr) CORE.invalidateShort(addr)
#define INVALIDATE_BYTE(addr) CORE.invalidateByte(addr)
#define STORE_WORD(value, addr) CORE.storeWord(value, addr)
#define STORE_SHORT(value, addr) CORE.storeShort(value, addr)
#define STORE_BYTE(value, addr) CORE.storeByte(value, addr)
#define CHECK_ADDR_BYTE(addr) CHECK_ADDR(addr)
#define CHECK_ADDR_SHORT(addr) (!((addr) & 1) && CHECK_ADDR(addr))
#define CHECK_ADDR_WORD(addr) (!((addr) & 3) && CHECK_ADDR(addr))
#define CHECK_ADDR_ROM(addr) ((uint32_t(addr) - CORE.romBase) < CORE.romSize)
#define CHECK_ADDR_ROM_BYTE(addr) CHECK_ADDR_ROM(addr)
#define CHECK_ADDR_ROM_SHORT(addr) (!((addr) & 1) && CHECK_ADDR(addr))
#define CHECK_ADDR_ROM_WORD(addr) (!((addr) & 3) && CHECK_ADDR(addr))
#define FROM_PC(addr) VIRTUAL_ADDR((addr) << 1)
#define TO_PC(addr) (PHYSICAL_ADDR(addr) >> 1)
#define TIME THREAD.time

#endif //_InstructionMacrosCommon_h_
