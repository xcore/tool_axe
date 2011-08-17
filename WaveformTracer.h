// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _WaveformTracer_h_
#define _WaveformTracer_h_

#include <vector>
#include <fstream>
#include <string>
#include "PortInterface.h"

class Port;
class WaveformTracer;

class WaveformTracerPort : public PortInterface {
  WaveformTracer *parent;
  Port *port;
  std::string identifier;
public:
  WaveformTracerPort(WaveformTracer *w, Port *p, const std::string id) :
    parent(w),
    port(p),
    identifier(id) {}
  void seePinsChange(const Signal &value, ticks_t time);
  Port *getPort() { return port; }
  const std::string &getIdentifier() const { return identifier; }
};

class WaveformTracer {
  std::fstream out;
  std::vector<WaveformTracerPort> ports;
  bool portsFinalized;
  ticks_t currentTime;
  std::string makeIdentifier(unsigned index);
  void emitDate();
  void emitDeclarations();
  void dumpPortValue(const std::string &identifer, Port *port, uint32_t value);
  void dumpInitialValues();
public:
  WaveformTracer(const std::string &name);
  void add(Port *port);
  void finalizePorts();
  void seePinsChange(const std::string &identifier, Port *port,
                     const Signal &value, ticks_t time);
};

#endif // _WaveformTracer_h_
