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
class PortAliases;

class PortArg {
  std::string core;
  std::string port;
  /// Beginning offset of this slice of the port (inclusive).
  signed char beginOffset;
  /// End offset of this slice of the port (exclusive).
  signed char endOffset;
  
  Port *lookupPort(SystemState &system, const PortAliases &portAliases) const;
public:
  PortArg() : beginOffset(0), endOffset(0) {}
  PortArg(const std::string &c, const std::string &p, signed char begin,
          signed char end) :
    core(c), port(p), beginOffset(begin), endOffset(end) {}
  static bool parse(const std::string &s, PortArg &arg);
  bool lookup(SystemState &system, const PortAliases &portAliases, Port *&p,
              unsigned &beginOffset, unsigned &endOffset) const;
  void dump(std::ostream &s) const;
};

#endif // _PortArg_h
