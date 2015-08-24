// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SPIFlash_
#define _SPIFlash_

#include <memory>

namespace axe {

class PeripheralDescriptor;

std::unique_ptr<PeripheralDescriptor> getPeripheralDescriptorSPIFlash();
  
} // End axe namespace

#endif // _SPIFlash_
