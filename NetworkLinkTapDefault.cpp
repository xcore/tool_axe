// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "NetworkLink.h"
#include <iostream>
#include <cstdlib>

std::auto_ptr<NetworkLink> createNetworkLinkTap()
{
  std::cerr << "error: network TAP not supported on this platform\n";
  std::exit(1);
}
