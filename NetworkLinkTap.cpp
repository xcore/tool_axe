// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "NetworkLink.h"
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <memory>
#include <iostream>
#include <iomanip>

class NetworkLinkTap : public NetworkLink {
  int fd;
public:
  NetworkLinkTap();
  virtual void transmitFrame(const uint8_t *data, unsigned size);
  virtual bool receiveFrame(uint8_t *data, unsigned &size);
};

NetworkLinkTap::NetworkLinkTap()
{
  fd = open("/dev/tap0", O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    std::perror("opening tap interface");
    std::exit(1);
  }
}

void NetworkLinkTap::transmitFrame(const uint8_t *data, unsigned size)
{
  assert(fd >= 0);
  if (write(fd, data, size) < 0) {
    std::perror("writing to tap interface");
    std::exit(1);
  }
}

bool NetworkLinkTap::receiveFrame(uint8_t *data, unsigned &size)
{
  assert(fd >= 0);
  ssize_t nread = read(fd, data, maxFrameSize);
  if (nread < 0) {
    if (errno != EAGAIN)
      return false;
    std::perror("reading from interface");
    close(fd);
    std::exit(1);
  }
  size = nread;
  return true;
}

std::auto_ptr<NetworkLink> createNetworkLinkTap()
{
  return std::auto_ptr<NetworkLink>(new NetworkLinkTap);
}
