// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _PortConnectionManager_
#define _PortConnectionManager_

#include <string>

class Port;
class SystemState;
class PortAliases;
class PortArg;
class PortInterface;
class Properties;

class PortConnectionWrapper {
  friend class PortConnectionManager;
  Port *port;
  PortConnectionWrapper(Port *p) : port(p) {}
public:
  void attach(PortInterface *p);
  PortInterface *getInterface();
  bool operator!() const { return !port; }
};

class PortConnectionManager {
  friend class PortConnectionWrapper;
  SystemState &system;
  const PortAliases &portAliases;
public:
  PortConnectionManager(SystemState &sys, PortAliases &aliases);
  PortConnectionWrapper get(const PortArg &arg);
  PortConnectionWrapper get(const Properties &properties,
                            const std::string &name);
};

#endif // _PortConnectionManager_
