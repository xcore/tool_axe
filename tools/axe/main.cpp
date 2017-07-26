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
#include <ctime>
#include <cstdlib>

#include "AXEInitialize.h"
#include "Tracer.h"
#include "Resource.h"
#include "Core.h"
#include "SyscallHandler.h"
#include "XE.h"
#include "Config.h"
#include "ProcessorNode.h"
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
#include "StatsTracer.h"
#include "DelegatingTracer.h"
#include "CheckPacketOvertakeTracer.h"

// SDL must be included before main so that SDL can substitute main() with
// SDL_main() if required.
#if AXE_ENABLE_SDL
#include <SDL.h>
#endif

using namespace axe;

typedef std::vector<std::pair<PortArg, PortArg>> LoopbackPorts;

static bool
connectLoopbackPorts(PortConnectionManager &connectionManager,
                     const LoopbackPorts &ports)
{
  for (const auto &entry : ports) {
    PortConnectionWrapper first = connectionManager.get(entry.first);
    if (!first) {
      std::cerr << "Error: Invalid port ";
      entry.first.dump(std::cerr);
      std::cerr << '\n';
      return false;
    }
    PortConnectionWrapper second = connectionManager.get(entry.second);
    if (!second) {
      std::cerr << "Error: Invalid port ";
      entry.second.dump(std::cerr);
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
  for (Port *port : core.getPorts()) {
    waveformTracer.add(core.getCoreName(), port);
  }
}

static void
connectWaveformTracer(SystemState &system, WaveformTracer &waveformTracer)
{
  for (Node *node : system.getNodes()) {
    if (!node->isProcessorNode())
      continue;
    for (Core *core : static_cast<ProcessorNode*>(node)->getCores()) {
      connectWaveformTracer(*core, waveformTracer);
    }
  }
  waveformTracer.finalizePorts();
}

static bool
checkPeripheralPorts(PortConnectionManager &connectionManager,
                     const PeripheralDescriptor *descriptor,
                     const Properties &properties)
{
  for (const PropertyDescriptor &pd : descriptor->getProperties()) {
    if (pd.getType() != PropertyDescriptor::PORT)
      continue;
    const Property *property = properties.get(pd.getName());
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

static std::unique_ptr<Tracer>
createTracerFromOptions(const Options &options)
{
  std::vector<Tracer *> tracers;
  if (options.tracing) {
    tracers.push_back(new LoggingTracer(options.traceCycles));
  }
  if (options.stats) {
    tracers.push_back(new StatsTracer);
  }
  if (options.warnPacketOvertake) {
    tracers.push_back(new CheckPacketOvertakeTracer);
  }
  std::unique_ptr<Tracer> tracer;
  if (!tracers.empty()) {
    if (tracers.size() == 1) {
      tracer.reset(tracers.front());
    } else {
      std::unique_ptr<DelegatingTracer> delegatingTracer(new DelegatingTracer);
      for (Tracer *subTracer : tracers) {
        delegatingTracer->addDelegate(std::unique_ptr<Tracer>(subTracer));
      }
      tracer.reset(delegatingTracer.release());
    }
  }
  return tracer;
}

static void displayElapsedTime(ticks_t simTime, clock_t realTime)
{
  double simTimeSeconds =
    static_cast<double>(simTime) / (CYCLES_PER_TICK * 100000000);
  double realTimeSeconds =
    static_cast<double>(realTime) / CLOCKS_PER_SEC;
  double relativeSpeed = simTimeSeconds / realTimeSeconds;
  std::cout << "Time:\n";
  std::cout << "-----\n";
  std::cout << std::fixed;
  std::cout << "Elapsed simulated time: " << simTimeSeconds << "s\n";
  std::cout << "Elapsed real time: " << realTimeSeconds << "s\n";
  std::cout << "Relative simulator speed: " << relativeSpeed << '\n';
}

typedef std::vector<std::pair<PeripheralDescriptor*, Properties*>>
  PeripheralDescriptorWithPropertiesVector;

int
loop(const Options &options)
{
  XE xe(options.file);
  XEReader xeReader(xe);
  std::unique_ptr<Tracer> tracer = createTracerFromOptions(options);
  std::unique_ptr<SystemState> statePtr = xeReader.readConfig(std::move(tracer));
  if (!options.tracing)
    statePtr->setExitTracer(std::unique_ptr<Tracer>(new LoggingTracer(false)));
  PortAliases portAliases;
  xeReader.readPortAliases(portAliases);
  SystemState &sys = *statePtr;
  PortConnectionManager connectionManager(sys, portAliases);

  if (!connectLoopbackPorts(connectionManager, options.loopbackPorts)) {
    std::exit(1);
  }

  const PeripheralDescriptorWithPropertiesVector &peripherals =
    options.peripherals;
  for (auto &entry : peripherals) {
    if (!checkPeripheralPorts(connectionManager, entry.first, *entry.second)) {
      std::exit(1);
    }
    entry.first->createInstance(sys, connectionManager, *entry.second);
  }

  std::unique_ptr<WaveformTracer> waveformTracer;
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
  bootSequencer.getSyscallHandler()->setCmdLine(options.clientArgc,
                                                options.clientArgv);
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
  ticks_t before;
  if (options.time)
    before = std::clock();
  int retval = bootSequencer.execute();
  if (options.time) {
    ticks_t after = std::clock();
    displayElapsedTime(sys.getLatestThreadTime(), after - before);
  }
  return retval;
}

int
main(int argc, char **argv) {
  AXEInitialize(true);
  Options options;
  options.parse(argc, argv);
  int retval = loop(options);
  AXECleanup();
  // llvm::outs() closes the file descriptor for stdout in a destructor.
  // Flush std::cout to ensure we don't lose any messages.
  // TODO find a better workaround.
  std::cout.flush();
  return retval;
}
