// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "registerAllPeripherals.h"
#include "PeripheralRegistry.h"
#include "UartRx.h"
#include "SDRAM.h"
#include "SPIFlash.h"
#include "EthernetPhy.h"
#include "PeripheralDescriptor.h"
#include "Config.h"
#if AXE_ENABLE_SDL
#include "LCDScreen.h"
#include "PS2Keyboard.h"
#endif

using namespace axe;

void axe::registerAllPeripherals()
{
  PeripheralRegistry::add(getPeripheralDescriptorUartRx());
  PeripheralRegistry::add(getPeripheralDescriptorSDRAM());
  PeripheralRegistry::add(getPeripheralDescriptorSPIFlash());
  PeripheralRegistry::add(getPeripheralDescriptorEthernetPhy());
#if AXE_ENABLE_SDL
  PeripheralRegistry::add(getPeripheralDescriptorLCDScreen());
  PeripheralRegistry::add(getPeripheralDescriptorPS2Keyboard());
#endif
}
