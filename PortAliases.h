// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortAliases_h_
#define _PortAliases_h_

#include <map>
#include <string>

class PortAliases {
  std::map<std::string, std::pair<std::string,std::string> > aliases;
public:
  void add(const std::string &name, const std::string &core,
           const std::string &port) {
    aliases.insert(std::make_pair(name, std::make_pair(core, port)));
  }
  bool lookup(const std::string &name, std::string &core,
              std::string &port) const;
};

#endif // _PortAliases_h_
