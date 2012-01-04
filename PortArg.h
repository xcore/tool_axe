// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortArg_h
#define _PortArg_h

#include <string>
#include <iosfwd>

class Port;
class SystemState;

class PortArg {
  std::string core;
  std::string port;
public:
  PortArg() {}
  PortArg(const std::string &c, const std::string &p) : core(c), port(p) {}
  static bool parse(const std::string &s, PortArg &arg);
  Port *lookup(SystemState &system) const;
  void dump(std::ostream &s) const;
};

#endif // _PortArg_h
