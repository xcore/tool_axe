// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PortAliases.h"

bool PortAliases::
lookup(const std::string &name, std::string &core, std::string &port) const
{
  std::map<std::string, std::pair<std::string,std::string> >::const_iterator
  it = aliases.find(name);
  if (it == aliases.end())
    return false;
  core = it->second.first;
  port = it->second.second;
  return true;
}
