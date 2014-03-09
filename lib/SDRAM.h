// Copyright (c) 2014, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SDRAM_h_
#define _SDRAM_h_

#include <memory>

namespace axe {

class PeripheralDescriptor;

std::auto_ptr<PeripheralDescriptor> getPeripheralDescriptorSDRAM();

} // End namespace axe

#endif // _SDRAM_h_
