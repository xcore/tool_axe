// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "NetworkLink.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <memory>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <unistd.h>

using namespace axe;

class NetworkLinkTap : public NetworkLink {
  int fd;
public:
  NetworkLinkTap(const std::string &ifname);
  virtual void transmitFrame(const uint8_t *data, unsigned size) override;
  virtual bool receiveFrame(uint8_t *data, unsigned &size) override;
};

static int tun_alloc(std::string &dev)
{
  int fd;
  if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    std::perror("error: failed to open /dev/net/tun");
    std::exit(1);
  }

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  if (!dev.empty())
    std::strncpy(ifr.ifr_name, dev.c_str(), IFNAMSIZ);
  else
    std::strcpy(ifr.ifr_name, "tap%d");

  int err;
  if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
    close(fd);
    std::perror("error: ioctl failed");
    std::exit(1);
  }
  dev = ifr.ifr_name;
  return fd;
}

NetworkLinkTap::NetworkLinkTap(const std::string &ifname)
{
  std::string name(ifname);
  fd = tun_alloc(name);
  if (fd < 0) {
    std::perror("opening tap interface");
    std::exit(1);
  }
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
    // EIO is returned is device is not yet configured.
    if (errno == EIO)
      return false;
    std::perror("reading from interface");
    close(fd);
    std::exit(1);
  }
  size = nread;
  return true;
}

std::auto_ptr<NetworkLink> axe::createNetworkLinkTap(const std::string &ifname)
{
  return std::auto_ptr<NetworkLink>(new NetworkLinkTap(ifname));
}
