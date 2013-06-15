// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <cassert>
#include <memory>
#include <climits>

#include "AXEInitialize.h"
#include "Tracer.h"
#include "Resource.h"
#include "Core.h"
#include "SyscallHandler.h"
#include "XE.h"
#include "Config.h"
#include "Node.h"
#include "SystemState.h"
#include "WaveformTracer.h"
#include "registerAllPeripherals.h"
#include "Options.h"
#include "BootSequencer.h"
#include "PortAliases.h"
#include "PortConnectionManager.h"
#include "XEReader.h"
#include "Property.h"
#include "LoggingTracer.h"
#include "DelegatingTracer.h"

// SDL must be included before main so that SDL can substitute main() with
// SDL_main() if required.
#ifdef AXE_ENABLE_SDL
#include <SDL.h>
#endif

using namespace axe;

typedef std::vector<std::pair<PortArg, PortArg>> LoopbackPorts;

static bool
connectLoopbackPorts(PortConnectionManager &connectionManager,
                     const LoopbackPorts &ports)
{
  for (LoopbackPorts::const_iterator it = ports.begin(), e = ports.end();
       it != e; ++it) {
    PortConnectionWrapper first = connectionManager.get(it->first);
    if (!first) {
      std::cerr << "Error: Invalid port ";
      it->first.dump(std::cerr);
      std::cerr << '\n';
      return false;
    }
    PortConnectionWrapper second = connectionManager.get(it->second);
    if (!second) {
      std::cerr << "Error: Invalid port ";
      it->second.dump(std::cerr);
      std::cerr << '\n';
      return false;
    }
    first.attach(second.getInterface());
    second.attach(first.getInterface());
  }
  return true;
}

static void connectWaveformTracer(Core &core, WaveformTracer &waveformTracer)
{
  for (Core::port_iterator it = core.port_begin(), e = core.port_end();
       it != e; ++it) {
    waveformTracer.add(core.getCoreName(), *it);
  }
}

static void
connectWaveformTracer(SystemState &system, WaveformTracer &waveformTracer)
{
  for (SystemState::node_iterator outerIt = system.node_begin(),
       outerE = system.node_end(); outerIt != outerE; ++outerIt) {
    Node &node = **outerIt;
    for (Node::core_iterator innerIt = node.core_begin(),
         innerE = node.core_end(); innerIt != innerE; ++innerIt) {
      Core &core = **innerIt;
      connectWaveformTracer(core, waveformTracer);
    }
  }
  waveformTracer.finalizePorts();
}

static bool
checkPeripheralPorts(PortConnectionManager &connectionManager,
                     const PeripheralDescriptor *descriptor,
                     const Properties &properties)
{
  for (PeripheralDescriptor::const_iterator it = descriptor->properties_begin(),
       e = descriptor->properties_end(); it != e; ++it) {
    if (it->getType() != PropertyDescriptor::PORT)
      continue;
    const Property *property = properties.get(it->getName());
    if (!property)
      continue;
    const PortArg &portArg = property->getAsPort();
    if (!connectionManager.get(portArg)) {
      std::cerr << "Error: Invalid port ";
      portArg.dump(std::cerr);
      std::cerr << '\n';
      return false;
    }
  }
  return true;
}

static void readRom(const std::string &filename, std::vector<uint8_t> &rom)
{
  std::ifstream file(filename.c_str(),
                     std::ios::in | std::ios::binary | std::ios::ate);
  if (!file) {
    std::cerr << "Error opening \"" << filename << "\"\n";
    std::exit(1);
  }
  rom.resize(file.tellg());
  if (rom.empty())
    return;
  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char*>(&rom[0]), rom.size());
  if (!file) {
    std::cerr << "Error reading \"" << filename << "\"\n";
    std::exit(1);
  }
  file.close();
}

static std::auto_ptr<Tracer>
createTracerFromOptions(const Options &options)
{
  std::vector<Tracer *> tracers;
  if (options.tracing) {
    tracers.push_back(new LoggingTracer);
  }
  std::auto_ptr<Tracer> tracer;
  if (!tracers.empty()) {
    if (tracers.size() == 1) {
      tracer.reset(tracers.front());
    } else {
      std::auto_ptr<DelegatingTracer> delegatingTracer(new DelegatingTracer);
      for (Tracer *subTracer : tracers) {
        delegatingTracer->addDelegate(std::auto_ptr<Tracer>(subTracer));
      }
      tracer.reset(delegatingTracer.release());
    }
  }
  return tracer;
}

typedef std::vector<std::pair<PeripheralDescriptor*, Properties*>>
  PeripheralDescriptorWithPropertiesVector;

int
loop(const Options &options)
{
  XE xe(options.file);
  XEReader xeReader(xe);
  std::auto_ptr<Tracer> tracer = createTracerFromOptions(options);
  std::auto_ptr<SystemState> statePtr = xeReader.readConfig(tracer);
  PortAliases portAliases;
  xeReader.readPortAliases(portAliases);
  SystemState &sys = *statePtr;
  PortConnectionManager connectionManager(sys, portAliases);

  if (!connectLoopbackPorts(connectionManager, options.loopbackPorts)) {
    std::exit(1);
  }

  const PeripheralDescriptorWithPropertiesVector &peripherals =
    options.peripherals;
  for (PeripheralDescriptorWithPropertiesVector::const_iterator it = peripherals.begin(),
       e = peripherals.end(); it != e; ++it) {
    if (!checkPeripheralPorts(connectionManager, it->first, *it->second)) {
      std::exit(1);
    }
    it->first->createInstance(sys, connectionManager, *it->second);
  }

  std::auto_ptr<WaveformTracer> waveformTracer;
  if (!options.vcdFile.empty()) {
    waveformTracer.reset(new WaveformTracer(options.vcdFile));
    connectWaveformTracer(sys, *waveformTracer);
  }

  if (!options.rom.empty()) {
    std::vector<uint8_t> rom;
    readRom(options.rom, rom);
    sys.setRom(&rom[0], rom.size());
  }

  BootSequencer bootSequencer(sys);
  bootSequencer.populateFromXE(xe);
  switch (options.bootMode) {
  default: assert(0 && "Unexpected bootmode");
  case Options::BOOT_SIM:
    break;
  case Options::BOOT_SPI:
    bootSequencer.adjustForSPIBoot();
    break;
  }
  if (options.maxCycles != 0) {
    sys.setTimeout(options.maxCycles);
  }
  return bootSequencer.execute();
}

int
main(int argc, char **argv) {
  AXEInitialize(true);
  Options options;
  options.parse(argc, argv);
  int retval = loop(options);
  AXECleanup();
  return retval;
}
