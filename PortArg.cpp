// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortArg.h"
#include <iostream>
#include <cstdlib>
#include "Core.h"

bool PortArg::parse(const std::string &s, PortArg &arg)
{
  arg = PortArg(s);
  return true;
};

Port *PortArg::lookup(Core &c) const
{
  char *endp;
  long value = std::strtol(port.c_str(), &endp, 0);
  if (*endp != '\0')
    return 0;
  Resource *res = c.getResourceByID(value);
  if (!res || res->getType() != RES_TYPE_PORT)
    return 0;
  return static_cast<Port*>(res);
}

void PortArg::dump(std::ostream &s) const
{
  s << port;
}
