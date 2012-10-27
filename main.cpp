// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/relaxng.h>
#include <libxslt/transform.h>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <memory>
#include <climits>
#include <set>
#include <map>

#include "Trace.h"
#include "Resource.h"
#include "Core.h"
#include "SyscallHandler.h"
#include "SymbolInfo.h"
#include "XE.h"
#include "ScopedArray.h"
#include "Config.h"
#include "Instruction.h"
#include "Node.h"
#include "SystemState.h"
#include "WaveformTracer.h"
#include "registerAllPeripherals.h"
#include "JIT.h"
#include "Options.h"
#include "BootSequence.h"
#include "PortAliases.h"
#include "PortConnectionManager.h"

// SDL must be included before main so that SDL can substitute main() with
// SDL_main() is required.
#ifdef AXE_ENABLE_SDL
#include <SDL.h>
#endif

const char configSchema[] = {
#include "ConfigSchema.inc"
};

const char XNSchema[] = {
#include "XNSchema.inc"
};

const char XNTransform[] = {
#include "XNTransform.inc"
  // Ensure null termination.
  , '\0'
};

static xmlNode *findChild(xmlNode *node, const char *name)
{
  for (xmlNode *child = node->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE)
      continue;
    if (strcmp(name, (char*)child->name) == 0)
      return child;
  }
  return 0;
}

static xmlAttr *findAttribute(xmlNode *node, const char *name)
{
  for (xmlAttr *child = node->properties; child; child = child->next) {
    if (child->type != XML_ATTRIBUTE_NODE)
      continue;
    if (strcmp(name, (char*)child->name) == 0)
      return child;
  }
  return 0;
}

typedef std::vector<std::pair<PortArg, PortArg> > LoopbackPorts;

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

static long readNumberAttribute(xmlNode *node, const char *name)
{
  xmlAttr *attr = findAttribute(node, name);
  assert(attr);
  errno = 0;
  long value = std::strtol((char*)attr->children->content, 0, 0);
  if (errno != 0) {
    std::cerr << "Invalid " << name << '"' << (char*)attr->children->content
              << "\"\n";
    exit(1);
  }
  return value;
}

static inline std::auto_ptr<Core>
createCoreFromConfig(xmlNode *config)
{
  uint32_t ram_size = RAM_SIZE;
  uint32_t ram_base = RAM_BASE;
  xmlNode *memoryController = findChild(config, "MemoryController");
  xmlNode *ram = findChild(memoryController, "Ram");
  ram_base = readNumberAttribute(ram, "base");
  ram_size = readNumberAttribute(ram, "size");
  if (!isPowerOf2(ram_size)) {
    std::cerr << "Error: ram size is not a power of two\n";
    std::exit(1);
  }
  if ((ram_base % ram_size) != 0) {
    std::cerr << "Error: ram base is not a multiple of ram size\n";
    std::exit(1);
  }
  std::auto_ptr<Core> core(new Core(ram_size, ram_base));
  core->setCoreNumber(readNumberAttribute(config, "number"));
  if (xmlAttr *codeReference = findAttribute(config, "codeReference")) {
    core->setCodeReference((char*)codeReference->children->content);
  }
  return core;
}

static inline std::auto_ptr<Node>
createNodeFromConfig(xmlNode *config,
                     std::map<long,Node*> &nodeNumberMap)
{
  long jtagID = readNumberAttribute(config, "jtagId");
  Node::Type nodeType;
  if (!Node::getTypeFromJtagID(jtagID, nodeType)) {
    std::cerr << "Unknown jtagId 0x" << std::hex << jtagID << std::dec << '\n';
    std::exit(1);
  }
  long numXLinks = 0;
  if (xmlNode *switchNode = findChild(config, "Switch")) {
    if (findAttribute(switchNode, "sLinks")) {
      numXLinks = readNumberAttribute(switchNode, "sLinks");
    }
  }
  std::auto_ptr<Node> node(new Node(nodeType, numXLinks));
  long nodeID = readNumberAttribute(config, "number");
  nodeNumberMap.insert(std::make_pair(nodeID, node.get()));
  for (xmlNode *child = config->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE ||
        strcmp("Processor", (char*)child->name) != 0)
      continue;
    node->addCore(createCoreFromConfig(child));
  }
  return node;
}

static bool parseXLinkEnd(xmlAttr *attr, long &node, long &xlink)
{
  const char *s = (char*)attr->children->content;
  errno = 0;
  char *endp;
  node = std::strtol(s, &endp, 0);
  if (errno != 0 || *endp != ',')
    return false;
  xlink = std::strtol(endp + 1, &endp, 0);
  if (errno != 0 || *endp != '\0')
    return false;
  return true;
}

static Node *
lookupNodeChecked(const std::map<long,Node*> &nodeNumberMap, unsigned nodeID)
{
  std::map<long,Node*>::const_iterator it = nodeNumberMap.find(nodeID);
  if (it == nodeNumberMap.end()) {
    std::cerr << "No node matching id " << nodeID << std::endl;
    std::exit(1);
  }
  return it->second;
}


static xmlDoc *
applyXSLTTransform(xmlDoc *doc, const char *transformData)
{
  xmlDoc *transformDoc =
    xmlReadDoc(BAD_CAST transformData, "XNTransform.xslt", NULL, 0);
  xsltStylesheetPtr stylesheet = xsltParseStylesheetDoc(transformDoc);
  xmlDoc *newDoc = xsltApplyStylesheet(stylesheet, doc, NULL);
  xsltFreeStylesheet(stylesheet);
  return newDoc;
}

static bool
checkDocAgainstSchema(xmlDoc *doc, const char *schemaData, size_t schemaSize)
{
  xmlRelaxNGParserCtxtPtr schemaContext =
    xmlRelaxNGNewMemParserCtxt(schemaData, schemaSize);
  xmlRelaxNGPtr schema = xmlRelaxNGParse(schemaContext);
  xmlRelaxNGValidCtxtPtr validationContext =
    xmlRelaxNGNewValidCtxt(schema);
  bool isValid = xmlRelaxNGValidateDoc(validationContext, doc) == 0;
  xmlRelaxNGFreeValidCtxt(validationContext);
  xmlRelaxNGFree(schema);
  xmlRelaxNGFreeParserCtxt(schemaContext);
  return isValid;
}

static inline std::auto_ptr<SystemState>
createSystemFromConfig(const std::string &filename,
                       const XESector *configSector)
{
  uint64_t length = configSector->getLength();
  const scoped_array<char> buf(new char[length + 1]);
  if (!configSector->getData(buf.get())) {
    std::cerr << "Error reading config from \"" << filename << "\"" << std::endl;
    std::exit(1);
  }
  if (length < 8) {
    std::cerr << "Error unexpected config config sector length" << std::endl;
    std::exit(1);
  }
  length -= 8;
  buf[length] = '\0';
  
  xmlDoc *doc = xmlReadDoc(BAD_CAST buf.get(), "config.xml", NULL, 0);

  if (!checkDocAgainstSchema(doc, configSchema, sizeof(configSchema)))
    std::exit(1);

  xmlNode *root = xmlDocGetRootElement(doc);
  xmlNode *system = findChild(root, "System");
  xmlNode *nodes = findChild(system, "Nodes");
  std::auto_ptr<SystemState> systemState(new SystemState);
  std::map<long,Node*> nodeNumberMap;
  for (xmlNode *child = nodes->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE ||
        strcmp("Node", (char*)child->name) != 0)
      continue;
    systemState->addNode(createNodeFromConfig(child, nodeNumberMap));
  }
  xmlNode *connections = findChild(system, "Connections");
  for (xmlNode *child = connections->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE ||
        strcmp("SLink", (char*)child->name) != 0)
      continue;
    long nodeID1, link1, nodeID2, link2;
    if (!parseXLinkEnd(findAttribute(child, "end1"), nodeID1, link1)) {
      std::cerr << "Failed to parse \"end1\" attribute" << std::endl;
      std::exit(1);
    }
    if (!parseXLinkEnd(findAttribute(child, "end2"), nodeID2, link2)) {
      std::cerr << "Failed to parse \"end2\" attribute" << std::endl;
      std::exit(1);
    }
    Node *node1 = lookupNodeChecked(nodeNumberMap, nodeID1);
    if (link1 >= node1->getNumXLinks()) {
      std::cerr << "Invalid sLink number " << link1 << std::endl;
      std::exit(1);
    }
    Node *node2 = lookupNodeChecked(nodeNumberMap, nodeID2);
    if (link2 >= node2->getNumXLinks()) {
      std::cerr << "Invalid sLink number " << link2 << std::endl;
      std::exit(1);
    }
    node1->connectXLink(link1, node2, link2);
    node2->connectXLink(link2, node1, link1);
  }
  xmlNode *jtag = findChild(system, "JtagChain");
  unsigned jtagIndex = 0;
  for (xmlNode *child = jtag->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE ||
        strcmp("Node", (char*)child->name) != 0)
      continue;
    long nodeID = readNumberAttribute(child, "id");
    lookupNodeChecked(nodeNumberMap, nodeID)->setJtagIndex(jtagIndex++);
  }
  systemState->finalize();
  xmlFreeDoc(doc);
  return systemState;
}

static inline void
addToCoreMap(std::map<std::pair<unsigned, unsigned>,Core*> &coreMap,
             Node &node)
{
  unsigned jtagIndex = node.getJtagIndex();
  const std::vector<Core*> &cores = node.getCores();
  unsigned coreNum = 0;
  for (Core *core : cores) {
    coreMap.insert(std::make_pair(std::make_pair(jtagIndex, coreNum), core));
    coreNum++;
  }
}


static inline void
addToCoreMap(std::map<std::pair<unsigned, unsigned>,Core*> &coreMap,
             SystemState &system)
{
  for (SystemState::node_iterator it = system.node_begin(),
       e = system.node_end(); it != e; ++it) {
    addToCoreMap(coreMap, **it);
  }
}

static inline std::auto_ptr<SystemState> readXE(XE &xe)
{
  // Load the file into memory.
  if (!xe) {
    std::cerr << "Error opening \"" << xe.getFileName() << "\"" << std::endl;
    std::exit(1);
  }
  // TODO handle XEs / XBs without a config sector.
  const XESector *configSector = xe.getConfigSector();
  if (!configSector) {
    std::cerr << "Error: No config file found in \"";
    std::cerr << xe.getFileName() << "\"" << std::endl;
    std::exit(1);
  }
  std::auto_ptr<SystemState> system =
    createSystemFromConfig(xe.getFileName(), configSector);
  return system;
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

const uint32_t romBaseAddress = 0xffffc000;

static void
adjustForBootMode(const Options &options, SystemState &sys,
                  BootSequence &bootSequence)
{
  if (options.bootMode == Options::BOOT_SPI) {
    bootSequence.eraseAllButLastImage();
    bootSequence.overrideEntryPoint(romBaseAddress);
    bootSequence.setLoadImages(false);
    for (auto outerIt = sys.node_begin(), outerE = sys.node_end();
         outerIt != outerE; ++outerIt) {
      Node &node = **outerIt;
      for (auto innerIt = node.core_begin(), innerE = node.core_end();
           innerIt != innerE; ++innerIt) {
        Core *core = *innerIt;
        core->setBootConfig(1 << 2);
      }
    }
  }
}

typedef std::vector<std::pair<PeripheralDescriptor*, Properties> >
  PeripheralDescriptorWithPropertiesVector;

static void readPortAliasesFromCore(PortAliases &aliases, xmlNode *core)
{
  std::string reference((char*)findAttribute(core, "Reference")->children->content);
  
  for (xmlNode *port = core->children; port; port = port->next) {
    if (port->type != XML_ELEMENT_NODE ||
        strcmp("Port", (char*)port->name) != 0)
      continue;
    std::string location((char*)findAttribute(port, "Location")->children->content);
    std::string name((char*)findAttribute(port, "Name")->children->content);
    aliases.add(name, reference, location);
  }
}

static void readPortAliasesFromNodes(PortAliases &aliases, xmlNode *nodes)
{
  for (xmlNode *node = nodes->children; node; node = node->next) {
    if (node->type != XML_ELEMENT_NODE ||
        strcmp("Node", (char*)node->name) != 0)
      continue;
    for (xmlNode *core = node->children; core; core = core->next) {
      if (core->type != XML_ELEMENT_NODE ||
          strcmp("Tile", (char*)core->name) != 0)
        continue;
      readPortAliasesFromCore(aliases, core);
    }
  }
}

static void readPortAliases(PortAliases &aliases, xmlNode *root) {
  xmlNode *packages = findChild(root, "Packages");
  for (xmlNode *child = packages->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE ||
        strcmp("Package", (char*)child->name) != 0)
      continue;
    readPortAliasesFromNodes(aliases, findChild(child, "Nodes"));
  }
}

static void
readPortAliases(PortAliases &aliases, XE &xe)
{
  const XESector *XNSector = xe.getXNSector();
  if (!XNSector)
    return;
  uint64_t length = XNSector->getLength();
  const scoped_array<char> buf(new char[length + 1]);
  if (!XNSector->getData(buf.get())) {
    std::cerr << "Error reading XN from \"" << xe.getFileName() << "\"";
    std::cerr << std::endl;
    std::exit(1);
  }
  if (length < 8) {
    std::cerr << "Error unexpected XN sector length" << std::endl;
    std::exit(1);
  }
  length -= 8;
  buf[length] = '\0';
  
  xmlDoc *doc = xmlReadDoc(BAD_CAST buf.get(), "platform_def.xn", NULL, 0);
  xmlDoc *newDoc = applyXSLTTransform(doc, XNTransform);
  xmlFreeDoc(doc);
  if (!checkDocAgainstSchema(newDoc, XNSchema, sizeof(XNSchema)))
    std::exit(1);

  readPortAliases(aliases, xmlDocGetRootElement(newDoc));
  xmlFreeDoc(newDoc);
}

static void
populateBootSequence(XE &xe, SystemState &sys, BootSequence &bootSequence)
{
  std::map<std::pair<unsigned, unsigned>,Core*> coreMap;
  addToCoreMap(coreMap, sys);
  
  std::set<Core*> gotoSectors;
  std::set<Core*> callSectors;
  for (const XESector *sector : xe.getSectors()) {
    switch (sector->getType()) {
    case XESector::XE_SECTOR_ELF:
      {
        const XEElfSector *elfSector = static_cast<const XEElfSector*>(sector);
        unsigned jtagIndex = elfSector->getNode();
        unsigned coreNum = elfSector->getCore();
        Core *core = coreMap[std::make_pair(jtagIndex, coreNum)];
        if (!core) {
          std::cerr << "Error: cannot find node " << jtagIndex
          << ", core " << coreNum << std::endl;
          std::exit(1);
        }
        if (gotoSectors.count(core)) {
          // Shouldn't happen.
          break;
        }
        if (callSectors.count(core)) {
          bootSequence.addRun(callSectors.size());
          callSectors.clear();
        }
        bootSequence.addElf(core, elfSector);
      }
      break;
    case XESector::XE_SECTOR_CALL:
      {
        const XECallOrGotoSector *callSector =
        static_cast<const XECallOrGotoSector*>(sector);
        if (!gotoSectors.empty()) {
          // Shouldn't happen.
          break;
        }
        unsigned jtagIndex = callSector->getNode();
        unsigned coreNum = callSector->getCore();
        Core *core = coreMap[std::make_pair(jtagIndex, coreNum)];
        if (!core) {
          std::cerr << "Error: cannot find node " << jtagIndex
          << ", core " << coreNum << std::endl;
          std::exit(1);
        }
        if (!callSectors.insert(core).second) {
          bootSequence.addRun(callSectors.size());
          callSectors.clear();
          callSectors.insert(core);
        }
      }
      break;
    case XESector::XE_SECTOR_GOTO:
      {
        const XECallOrGotoSector *gotoSector =
        static_cast<const XECallOrGotoSector*>(sector);
        if (!callSectors.empty()) {
          // Handle calls.
          bootSequence.addRun(callSectors.size());
          callSectors.clear();
        }
        unsigned jtagIndex = gotoSector->getNode();
        unsigned coreNum = gotoSector->getCore();
        Core *core = coreMap[std::make_pair(jtagIndex, coreNum)];
        if (!core) {
          std::cerr << "Error: cannot find node " << jtagIndex
          << ", core " << coreNum << std::endl;
          std::exit(1);
        }
        if (!gotoSectors.insert(core).second) {
          // Shouldn't happen.
          break;
        }
      }
      break;
    }
  }
  if (!gotoSectors.empty()) {
    bootSequence.addRun(gotoSectors.size());
  } else if (!callSectors.empty()) {
    // Shouldn't happen.
    bootSequence.addRun(callSectors.size());
  }
}

int
loop(const Options &options)
{
  XE xe(options.file);
  std::auto_ptr<SystemState> statePtr = readXE(xe);
  PortAliases portAliases;
  readPortAliases(portAliases, xe);
  SystemState &sys = *statePtr;
  PortConnectionManager connectionManager(sys, portAliases);

  if (!connectLoopbackPorts(connectionManager, options.loopbackPorts)) {
    std::exit(1);
  }

  const PeripheralDescriptorWithPropertiesVector &peripherals =
    options.peripherals;
  for (PeripheralDescriptorWithPropertiesVector::const_iterator it = peripherals.begin(),
       e = peripherals.end(); it != e; ++it) {
    if (!checkPeripheralPorts(connectionManager, it->first, it->second)) {
      std::exit(1);
    }
    it->first->createInstance(sys, connectionManager, it->second);
  }

  std::auto_ptr<WaveformTracer> waveformTracer;
  // TODO update to handle multiple cores.
  if (!options.vcdFile.empty()) {
    waveformTracer.reset(new WaveformTracer(options.vcdFile));
    connectWaveformTracer(sys, *waveformTracer);
  }

  if (!options.rom.empty()) {
    std::vector<uint8_t> rom;
    readRom(options.rom, rom);
    sys.setRom(&rom[0], romBaseAddress, rom.size());
  }

  BootSequence bootSequence(sys);

  populateBootSequence(xe, sys, bootSequence);
  adjustForBootMode(options, sys, bootSequence);

  return bootSequence.execute();
}

int
main(int argc, char **argv) {
  /*
   * this initialize the library and check potential ABI mismatches
   * between the version it was compiled for and the actual shared
   * library used.
   */
  LIBXML_TEST_VERSION

  registerAllPeripherals();
  Options options;
  options.parse(argc, argv);
#ifndef _WIN32
  if (isatty(fileno(stdout))) {
    Tracer::get().setColour(true);
  }
#endif
  if (options.tracing) {
    Tracer::get().setTracingEnabled(true);
  }
  int retval = loop(options);
  xsltCleanupGlobals();
  xmlCleanupParser();
  return retval;
}
