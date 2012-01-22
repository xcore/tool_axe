// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _InstructionMacrosCommon_h_
#define _InstructionMacrosCommon_h_

#define LOAD_WORD(addr) CORE.loadWord(addr)
#define LOAD_SHORT(addr) CORE.loadShort(addr)
#define LOAD_BYTE(addr) CORE.loadByte(addr)
#define STORE_WORD(value, addr) CORE.storeWord(value, addr)
#define STORE_SHORT(value, addr) CORE.storeShort(value, addr)
#define STORE_BYTE(value, addr) CORE.storeByte(value, addr)
#define ADDR(addr) CORE.physicalAddress(addr)
#define PHYSICAL_ADDR(addr) CORE.physicalAddress(addr)
#define VIRTUAL_ADDR(addr) CORE.virtualAddress(addr)
#define CHECK_ADDR(addr) CORE.isValidAddress(addr)
#define CHECK_ADDR_SHORT(addr) (!((addr) & 1) && CHECK_ADDR(addr))
#define CHECK_ADDR_WORD(addr) (!((addr) & 3) && CHECK_ADDR(addr))
#define FROM_PC(addr) CORE.virtualAddress((addr) << 1)
#define TO_PC(addr) (CORE.physicalAddress(addr) >> 1)
#define CHECK_PC(addr) ((addr) < (CORE.ram_size << 1))

#endif //_InstructionMacrosCommon_h_
