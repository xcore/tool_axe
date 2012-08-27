// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _NetworkLink_h
#define _NetworkLink_h

#include <stdint.h>
#include <memory>

class NetworkLink {
public:
  static const unsigned maxFrameSize = 1500 + 18;
  virtual ~NetworkLink();
  virtual void transmitFrame(const uint8_t *data, unsigned size) = 0;
  virtual bool receiveFrame(uint8_t *data, unsigned &size) = 0;
};

std::auto_ptr<NetworkLink> createNetworkLinkTap();

#endif // _NetworkLink_h
