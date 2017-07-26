// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PS2Keyboard_h_
#define _PS2Keyboard_h_

#include <memory>

namespace axe {
class PeripheralDescriptor;

std::unique_ptr<PeripheralDescriptor> getPeripheralDescriptorPS2Keyboard();
} // End namespace axe

#endif // _PS2Keyboard_h_
