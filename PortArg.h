// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortArg_h
#define _PortArg_h

#include <string>
#include <iosfwd>

class Port;
class Core;

class PortArg {
  std::string port;
public:
  PortArg() {}
  PortArg(const std::string s) : port(s) {}
  static bool parse(const std::string &s, PortArg &arg);
  Port *lookup(Core &c) const;
  void dump(std::ostream &s) const;
};

#endif // _PortArg_h
