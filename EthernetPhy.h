// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>


#ifndef _EthernetPhy_h_
#define _EthernetPhy_h_

#include <memory>

namespace axe {

class PeripheralDescriptor;

  std::auto_ptr<PeripheralDescriptor> getPeripheralDescriptorEthernetPhy();
  
} // End axe namespace

#endif // _EthernetPhy_h_
