// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _LCDScreen_h_
#define _LCDScreen_h_

#include <memory>

namespace axe {

class PeripheralDescriptor;

std::auto_ptr<PeripheralDescriptor> getPeripheralDescriptorLCDScreen();

} // End namespace axe

#endif // _LCDScreen_h_
