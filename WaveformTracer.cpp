// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "WaveformTracer.h"
#include "Port.h"
#include "BitManip.h"
#include <ctime>

using namespace axe;

WaveformTracer::WaveformTracer(const std::string &name) :
  out(name.c_str(), std::fstream::out),
  portsFinalized(false),
  currentTime(0) {}

void WaveformTracerPort::update(ticks_t newTime)
{
  if (!prev.isClock())
    return;
  EdgeIterator it = prev.getEdgeIterator(time);
  while (it->time <= newTime) {
    uint32_t value = it->type == Edge::RISING ? 1 : 0;
    parent->seePinsChange(this, value, it->time);
    ++it;
  }
  time = newTime;
  parent->schedule(this, it->time);
}

void WaveformTracerPort::seePinsChange(const Signal &value, ticks_t newTime)
{
  if (value == prev)
    return;
  parent->runUntil(newTime);
  uint32_t prevValue = prev.getValue(newTime);
  uint32_t newValue = value.getValue(newTime);
  if (prevValue != newValue)
    parent->seePinsChange(this, newValue, newTime);
  time = newTime;
  prev = value;
  if (value.isClock()) {
    parent->schedule(this, value.getNextEdge(newTime).time);
  }
}

void WaveformTracer::add(const std::string &name, Port *port)
{
  assert(!portsFinalized);
  modules[name].push_back(ports.size());
  ports.push_back(WaveformTracerPort(this, port,
                                     makeIdentifier(ports.size())));
}

std::string WaveformTracer::makeIdentifier(unsigned index)
{
  std::string identifier;
  unsigned offset = '!';
  unsigned base = '~' - '!' + 1;
  if (index) {
    while (index) {
      identifier.push_back(offset + (index % base));
      index /= base;
    }
  } else {
    identifier.push_back(offset);
  }
  return identifier;
}

void WaveformTracer::emitDate()
{
  time_t rawTime = std::time(0);
  if (rawTime == -1)
    return;
  struct tm *timeinfo = localtime(&rawTime);
  char buf[256];
  if (std::strftime(buf, 256, "%B %d, %Y %X", timeinfo) == 0)
    return;
  out << "$date\n";
  out << "  " << buf << '\n';
  out << "$end\n";
}

void WaveformTracer::emitDeclarations()
{
  emitDate();

  out << "$version\n";
  out << "  AXE (An XCore Emulator)\n";
  out << "$end\n";

  out << "$timescale\n";
  out << "  100 ps\n";
  out << "$end\n";
}


void WaveformTracer::
dumpPortValue(const std::string &identifier, Port *port, uint32_t value)
{
  if (port->getPortWidth() == 1) {
    // Dumps of value changes to scalar variables shall not have any white space
    // between the value and the identifier code.
    out << (value & 1);
  } else {
    // Dumps of value changes to vectors shall not have any white space between
    // the base letter and the value digits, but they shall have one white space
    // between the value digits and the identifier code.
    // The output format for each value is right-justified. Vector values appear
    // in the shortest form possible: redundant bit values which result from
    // left-extending values to fill a particular vector size are eliminated.
    out << 'b';
    if (value) {
      for (int i = 31 - countLeadingZeros(value); i >= 0; i--) {
        out << ((value & (1 << i)) ? '1' : '0');
      }
    } else {
      out << '0';
    }
    out << ' ';
  }
  out << identifier;
  out << '\n';
}

void WaveformTracer::dumpInitialValues()
{
  out << "$dumpvars\n";
  for (WaveformTracerPort &port : ports) {
    dumpPortValue(port.getIdentifier(), port.getPort(), 0);
  }
  out << "$end\n";
}

void WaveformTracer::finalizePorts()
{
  assert(!portsFinalized);
  portsFinalized = true;
  emitDeclarations();
  for (auto &entry : modules) {
    const std::string &moduleName = entry.first;
    if (!moduleName.empty()) {
      out << "$scope\n";
      out << "  module " << moduleName << '\n';
      out << "$end\n";
    }
    const std::vector<unsigned> &modulePorts = entry.second;
    for (unsigned index : modulePorts) {
      WaveformTracerPort &waveformTracerPort = ports[index];
      Port *port = waveformTracerPort.getPort();
      port->setTracer(&waveformTracerPort);
      out << "$var\n";
      out << "  wire";
      out << ' ' << std::dec << port->getID().width();
      out << ' ' << waveformTracerPort.getIdentifier();
      out << ' ' << port->getName();
      out << '\n';
      out << "$end\n";
    }
    if (!moduleName.empty()) {
      out << "$upscope $end\n";
    }
  }
  out << "$enddefinitions $end\n";
  dumpInitialValues();
}

void WaveformTracer::schedule(WaveformTracerPort *port, ticks_t time)
{
  queue.push(Event(port, time));
}

void WaveformTracer::runUntil(ticks_t time)
{
  if (!queue.empty()) {
    while (queue.top().time < time) {
      Event event = queue.top();
      event.port->update(event.time);
      queue.pop();
    }
  }
}

void WaveformTracer::
seePinsChange(WaveformTracerPort *port, uint32_t value, ticks_t time)
{
  ticks_t translatedTime = time * (100 / CYCLES_PER_TICK);
  if (translatedTime != currentTime) {
    out << '#' << translatedTime << '\n';
    currentTime = translatedTime;
  }
  dumpPortValue(port->getIdentifier(), port->getPort(), value);
}
