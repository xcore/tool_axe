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
  NetworkLinkTap(const std::string &ifname);
  virtual void transmitFrame(const uint8_t *data, unsigned size);
  virtual bool receiveFrame(uint8_t *data, unsigned &size);
};

static int tun_alloc(std::string &dev)
{
  int fd = -1;
  if (dev.empty()) {
    char devName[11] = "/dev/tap";
    for (unsigned i = 0; i < 16; i++) {
      std::snprintf(&devName[8], 3, "%d", i);
      fd = open(devName, O_RDWR);
      if (fd >= 0)
        break;
      if (errno == ENXIO || errno == ENOENT)
        break;
    }
  } else {
    fd = open(("/dev/" + dev).c_str(), O_RDWR);
  }
  if (fd < 0) {
    std::perror("error opening tap interface");
    std::exit(1);
  }
  return fd;
}

NetworkLinkTap::NetworkLinkTap(const std::string &ifname)
{
  std::string name(ifname);
  fd = tun_alloc(name);
  fcntl(fd, F_SETFL, O_NONBLOCK);
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
    if (errno == EAGAIN)
      return false;
    // EIO is returned if the device is not yet configured.
    if (errno == EIO)
      return false;
    std::perror("reading from interface");
    close(fd);
    std::exit(1);
  }
  size = nread;
  return true;
}

std::auto_ptr<NetworkLink> createNetworkLinkTap(const std::string &ifname)
{
  return std::auto_ptr<NetworkLink>(new NetworkLinkTap(ifname));
}
