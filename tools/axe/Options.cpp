// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Options.h"
#include "PeripheralRegistry.h"
#include "Property.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "Range.h"
#include "AXEVersion.h"

using namespace axe;

Options::Options() :
  bootMode(BOOT_SIM),
  file(0),
  tracing(false),
  time(false),
  stats(false),
  warnPacketOvertake(false),
  maxCycles(0),
  clientArgc(0),
  clientArgv(0)
{
}

Options::~Options()
{
  for (const auto &entry : peripherals) {
    delete entry.first;
    delete entry.second;
  }
}

static void printUsage(const char *ProgName) {
  std::cout << "Usage: " << ProgName << " [options] filename\n";
  std::cout << "Usage: " << ProgName << " [options] --args filename"
                                        " [inferior-arguments ...]\n";
  std::cout <<
  "General Options:\n"
  "  --args                      Arguments after filename are passed to client.\n"
  "  -help                       Display this information.\n"
  "  --version                   Print version information and then exit.\n"
  "  --loopback PORT1 PORT2      Connect PORT1 to PORT2.\n"
  "  --vcd FILE                  Write VCD trace to FILE.\n"
  "  --boot-spi                  Specify boot from SPI\n"
  "  --rom FILE                  Specify boot rom.\n"
  "  --max-cycles <n>            Exit after <n> cycles\n"
  "  -t                          Enable instruction tracing.\n"
  "  --trace-cycles              Display cycle count when tracing.\n"
  "  --time                      Display elapsed time on exit.\n"
  "  --stats                     Display simulator statistics on exit.\n"
  "  --warn-packet-overtake      Warn about possible packet overtaking.\n"
  "\n"
  "Peripherals:\n";
  for (PeripheralDescriptor *periph : make_range(PeripheralRegistry::begin(),
                                                 PeripheralRegistry::end())) {
    std::cout << "  --" << periph->getName() << ' ';
    bool needComma = false;
    for (const PropertyDescriptor &prop : periph->getProperties()) {
      if (needComma)
        std::cout << ',';
      std::cout << prop.getName() << '=';
      needComma = true;
    }
    std::cout << '\n';
  }
}

static void loopbackOption(const char *a, const char *b, LoopbackPorts &loopbackPorts)
{
  PortArg firstArg;
  if (!PortArg::parse(a, firstArg)) {
    std::cerr << "Error: Invalid port " << a << '\n';
    exit(1);
  }
  PortArg secondArg;
  if (!PortArg::parse(b, secondArg)) {
    std::cerr << "Error: Invalid port " << b << '\n';
    exit(1);
  }
  loopbackPorts.push_back(std::make_pair(firstArg, secondArg));
}

static uint32_t
parseIntegerProperty(const PropertyDescriptor *prop, const std::string &s)
{
  char *endp;
  long value = std::strtol(s.c_str(), &endp, 0);
  if (*endp != '\0') {
    std::cerr << "Error: property " << prop->getName();
    std::cerr << " requires an integer argument\n";
    std::exit(1);
  }
  return value;
}

static PortArg
parsePortProperty(const PropertyDescriptor *prop, const std::string &s)
{
  PortArg portArg;
  if (!PortArg::parse(s, portArg)) {
    std::cerr << "Error: property " << prop->getName();
    std::cerr << " requires an port argument\n";
    std::exit(1);
  }
  return portArg;
}

static void
parseProperties(const std::string &str, const PeripheralDescriptor *periph,
                Properties &properties)
{
  //port=PORT_1A,bitrate=28800
  const char *s = str.c_str();
  while (*s != '\0') {
    const char *p = std::strpbrk(s, "=,");
    std::string name;
    std::string value;
    if (!p) {
      name = std::string(s);
      s += name.length();
    } else {
      name = std::string(s, p - s);
      s = p + 1;
      if (*p == '=') {
        p = strchr(s, ',');
        if (p) {
          value = std::string(s, p - s);
          s += value.length() + 1;
        } else {
          value = std::string(s);
          s += value.length();
        }
      }
    }
    if (const PropertyDescriptor *prop = periph->getProperty(name)) {
      switch (prop->getType()) {
        default:
          assert(0 && "Unexpected property type");
        case PropertyDescriptor::INTEGER:
          properties.setIntegerProperty(prop,
                                        parseIntegerProperty(prop, value));
          break;
        case PropertyDescriptor::STRING:
          properties.setStringProperty(prop, value);
          break;
        case PropertyDescriptor::PORT:
          properties.setPortProperty(prop, parsePortProperty(prop, value));
          break;
      }
    } else {
      std::cerr << "Error: Unknown property \"" << name << "\"";
      std::cerr << " for " << periph->getName() << std::endl;
      std::exit(1);
    }
  }
}

static void checkRequiredProperties(const PeripheralDescriptor *periph,
                                    const Properties &properties)
{
  // Check required properties have been set.
  for (const PropertyDescriptor &property : periph->getProperties()) {
    if (property.getRequired() && !properties.get(property.getName())) {
      std::cerr << "Error: Required property \"" << property.getName() << "\"";
      std::cerr << " for " << periph->getName();
      std::cerr << " is not set " << std::endl;
      std::exit(1);
    }
  }
}

static PeripheralDescriptor *parsePeripheralOption(const std::string arg)
{
  if (arg.substr(0, 2) != "--")
    return 0;
  return PeripheralRegistry::get(arg.substr(2));
}


static void printVersion()
{
  std::cout << "AXE ";
  std::cout << AXE_VERSION_MAJOR;
  std::cout << '.' << AXE_VERSION_MINOR;
  std::cout << AXE_VERSION_TWEAK;
  std::cout << '\n';
}

void Options::parse(int argc, char **argv)
{
  if (argc < 2) {
    printUsage(argv[0]);
    std::exit(1);
  }
  std::string arg;
  for (int i = 1; i < argc; i++) {
    arg = argv[i];
    if (arg == "-t") {
      tracing = true;
    } else if (arg == "--trace-cycles") {
      traceCycles = true;
    } else if (arg == "--time") {
      time = true;
    } else if (arg == "--stats") {
      stats = true;
    } else if (arg == "--max-cycles") {
      if (i + 1 > argc) {
        printUsage(argv[0]);
        std::exit(1);
      }
      char *endp;
      errno = 0;
      long value = std::strtol(argv[i + 1], &endp, 10);
      if (errno != 0 || *endp != '\0' || value < 0) {
        std::cerr << "Error: failed to parse number of cycles\n";
        std::exit(1);
      }
      maxCycles = value;
      i++;
    } else if (arg == "--vcd") {
      if (i + 1 > argc) {
        printUsage(argv[0]);
        std::exit(1);
      }
      vcdFile = argv[i + 1];
      i++;
    } else if (arg == "--loopback") {
      if (i + 2 >= argc) {
        printUsage(argv[0]);
        std::exit(1);
      }
      loopbackOption(argv[i + 1], argv[i + 2], loopbackPorts);
      i += 2;
    } else if (arg == "--rom") {
      if (i + 1 > argc) {
        printUsage(argv[0]);
        std::exit(1);
      }
      rom = argv[i + 1];
      i++;
    } else if (arg == "--warn-packet-overtake") {
      warnPacketOvertake = true;
    } else if (arg == "--boot-spi") {
      bootMode = BOOT_SPI;
    } else if (arg == "--args") {
      ++i;
      if (file || i >= argc) {
        printUsage(argv[0]);
        std::exit(1);
      }
      file = argv[i];
      clientArgc = argc - i;
      clientArgv = &argv[i];
      break;
    } else if (arg == "--version") {
      printVersion();
      std::exit(0);
    } else if (arg == "--help") {
      printUsage(argv[0]);
      std::exit(0);
    } else if (PeripheralDescriptor *pd = parsePeripheralOption(arg)) {
      peripherals.push_back(std::make_pair(pd, new Properties()));
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        parseProperties(argv[++i], pd, *peripherals.back().second);
      }
      checkRequiredProperties(pd, *peripherals.back().second);
    } else {
      if (file) {
        printUsage(argv[0]);
        std::exit(1);
      }
      file = argv[i];
    }
  }
  if (!file) {
    printUsage(argv[0]);
    std::exit(1);
  }
}
