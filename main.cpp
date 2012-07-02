// Copyright (c) 2011-2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gelf.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/relaxng.h>
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

#define XCORE_ELF_MACHINE 0xB49E

const char configSchema[] = {
#include "ConfigSchema.inc"
};

static void readSymbols(Elf *e, Elf_Scn *scn, const GElf_Shdr &shdr,
                        unsigned low, unsigned high,
                        std::auto_ptr<CoreSymbolInfo> &SI)
{
  Elf_Data *data = elf_getdata(scn, NULL);
  if (data == NULL) {
    return;
  }
  unsigned count = shdr.sh_size / shdr.sh_entsize;

  CoreSymbolInfoBuilder builder;

  for (unsigned i = 0; i < count; i++) {
    GElf_Sym sym;
    if (gelf_getsym(data, i, &sym) == NULL) {
      continue;
    }
    if (sym.st_shndx == SHN_ABS)
      continue;
    if (sym.st_value < low || sym.st_value >= high)
      continue;
    builder.addSymbol(elf_strptr(e, shdr.sh_link, sym.st_name),
                      sym.st_value,
                      sym.st_info);
  }
  SI = builder.getSymbolInfo();
}

static void readSymbols(Elf *e, unsigned low, unsigned high,
                        std::auto_ptr<CoreSymbolInfo> &SI)
{
  Elf_Scn *scn = NULL;
  GElf_Shdr shdr;
  while ((scn = elf_nextscn(e, scn)) != NULL) {
    if (gelf_getshdr(scn, &shdr) == NULL) {
      continue;
    }
    if (shdr.sh_type == SHT_SYMTAB) {
      // Found the symbol table
      break;
    }
  }
  
  if (scn != NULL) {
    readSymbols(e, scn, shdr, low, high, SI);
  }
}

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

static void readElf(const char *filename, const XEElfSector *elfSector,
                    Core &core, std::auto_ptr<CoreSymbolInfo> &SI,
                    std::map<Core*,uint32_t> &entryPoints)
{
  uint64_t ElfSize = elfSector->getElfSize();
  const scoped_array<char> buf(new char[ElfSize]);
  if (!elfSector->getElfData(buf.get())) {
    std::cerr << "Error reading elf data from \"" << filename << "\"" << std::endl;
    std::exit(1);
  }

  if (elf_version(EV_CURRENT) == EV_NONE) {
    std::cerr << "ELF library intialisation failed: "
              << elf_errmsg(-1) << std::endl;
    std::exit(1);
  }
  Elf *e;
  if ((e = elf_memory(buf.get(), ElfSize)) == NULL) {
    std::cerr << "Error reading ELF: " << elf_errmsg(-1) << std::endl;
    std::exit(1);
  }
  if (elf_kind(e) != ELF_K_ELF) {
    std::cerr << filename << " is not an ELF object" << std::endl;
    std::exit(1);
  }
  GElf_Ehdr ehdr;
  if (gelf_getehdr(e, &ehdr) == NULL) {
    std::cerr << "Reading ELF header failed: " << elf_errmsg(-1) << std::endl;
    std::exit(1);
  }
  if (ehdr.e_machine != XCORE_ELF_MACHINE) {
    std::cerr << "Not a XCore ELF" << std::endl;
    std::exit(1);
  }
  if (ehdr.e_entry != 0) {
    entryPoints.insert(std::make_pair(&core, (uint32_t)ehdr.e_entry));
  }
  unsigned num_phdrs = ehdr.e_phnum;
  if (num_phdrs == 0) {
    std::cerr << "No ELF program headers" << std::endl;
    std::exit(1);
  }
  core.resetCaches();
  uint32_t ram_base = core.getRamBase();
  uint32_t ram_size = core.getRamSize();
  for (unsigned i = 0; i < num_phdrs; i++) {
    GElf_Phdr phdr;
    if (gelf_getphdr(e, i, &phdr) == NULL) {
      std::cerr << "Reading ELF program header " << i << " failed: " << elf_errmsg(-1) << std::endl;
      std::exit(1);
    }
    if (phdr.p_filesz == 0) {
      continue;
    }
    if (phdr.p_offset > ElfSize) {
    	std::cerr << "Invalid offet in ELF program header" << i << std::endl;
    	std::exit(1);
    }
    if (!core.isValidRamAddress(phdr.p_paddr) ||
        !core.isValidRamAddress(phdr.p_paddr + phdr.p_memsz)) {
      std::cerr << "Error data from ELF program header " << i;
      std::cerr << " does not fit in memory" << std::endl;
      std::exit(1);
    }
    core.writeMemory(phdr.p_paddr, &buf[phdr.p_offset], phdr.p_filesz);
  }

  readSymbols(e, ram_base, ram_base + ram_size, SI);

  elf_end(e);
}

typedef std::vector<std::pair<PortArg, PortArg> > LoopbackPorts;

static bool
connectLoopbackPorts(SystemState &state, const LoopbackPorts &ports)
{
  for (LoopbackPorts::const_iterator it = ports.begin(), e = ports.end();
       it != e; ++it) {
    Port *first = it->first.lookup(state);
    if (!first) {
      std::cerr << "Error: Invalid port ";
      it->first.dump(std::cerr);
      std::cerr << '\n';
      return false;
    }
    Port *second = it->second.lookup(state);
    if (!second) {
      std::cerr << "Error: Invalid port ";
      it->second.dump(std::cerr);
      std::cerr << '\n';
      return false;
    }
    first->setLoopback(second);
    second->setLoopback(first);
  }
  return true;
}

static void connectWaveformTracer(Core &core, WaveformTracer &waveformTracer)
{
  for (Core::port_iterator it = core.port_begin(), e = core.port_end();
       it != e; ++it) {
    waveformTracer.add(core.getCoreName(), *it);
  }
  waveformTracer.finalizePorts();
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

static inline std::auto_ptr<SystemState>
createSystemFromConfig(const char *filename, const XESector *configSector)
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
  
  /*
   * this initialize the library and check potential ABI mismatches
   * between the version it was compiled for and the actual shared
   * library used.
   */
  LIBXML_TEST_VERSION
  
  xmlDoc *doc = xmlReadDoc((xmlChar*)buf.get(), "config.xml", NULL, 0);

  xmlRelaxNGParserCtxtPtr schemaContext =
    xmlRelaxNGNewMemParserCtxt(configSchema, sizeof(configSchema));
  xmlRelaxNGPtr schema = xmlRelaxNGParse(schemaContext);
  xmlRelaxNGValidCtxtPtr validationContext =
    xmlRelaxNGNewValidCtxt(schema);
  if (xmlRelaxNGValidateDoc(validationContext, doc) != 0) {
    std::exit(1);
  }

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
  xmlCleanupParser();
  return systemState;
}

static inline void
addToCoreMap(std::map<std::pair<unsigned, unsigned>,Core*> &coreMap,
             Node &node)
{
  unsigned jtagIndex = node.getJtagIndex();
  const std::vector<Core*> &cores = node.getCores();
  unsigned coreNum = 0;
  for (std::vector<Core*>::const_iterator it = cores.begin(), e = cores.end();
       it != e; ++it) {
    coreMap.insert(std::make_pair(std::make_pair(jtagIndex, coreNum), *it));
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

static inline std::auto_ptr<SystemState>
readXE(XE &xe, const char *filename)
{
  // Load the file into memory.
  if (!xe) {
    std::cerr << "Error opening \"" << filename << "\"" << std::endl;
    std::exit(1);
  }
  // TODO handle XEs / XBs without a config sector.
  const XESector *configSector = xe.getConfigSector();
  if (!configSector) {
    std::cerr << "Error: No config file found in \"";
    std::cerr << filename << "\"" << std::endl;
    std::exit(1);
  }
  std::auto_ptr<SystemState> system =
    createSystemFromConfig(filename, configSector);
  return system;
}

static int
runCores(SystemState &sys, const std::set<Core*> &cores,
         const std::map<Core*,uint32_t> &entryPoints)
{
  for (std::set<Core*>::iterator it = cores.begin(), e = cores.end(); it != e;
       ++it) {
    Core *core = *it;
    sys.schedule(core->getThread(0));
    std::map<Core*,uint32_t>::const_iterator match;
    if ((match = entryPoints.find(core)) != entryPoints.end()) {
      uint32_t address = match->second;
      if ((address & 1) == 0 && core->isValidAddress(address)) {
        core->getThread(0).setPcFromAddress(address);
      } else {
        std::cout << "Warning: invalid ELF entry point 0x";
        std::cout << std::hex << match->second << std::dec << "\n";
      }
    }
  }
  SyscallHandler::setDoneSyscallsRequired(cores.size());
  return sys.run();
}

static int
runCoreRoms(SystemState &system)
{
  unsigned numCores = 0;
  for (SystemState::node_iterator outerIt = system.node_begin(),
       outerE = system.node_end(); outerIt != outerE; ++outerIt) {
    Node &node = **outerIt;
    for (Node::core_iterator innerIt = node.core_begin(),
         innerE = node.core_end(); innerIt != innerE; ++innerIt) {
      Core &core = **innerIt;
      system.schedule(core.getThread(0));
      core.getThread(0).setPcFromAddress(core.romBase);
      numCores++;
    }
  }
  SyscallHandler::setDoneSyscallsRequired(numCores);
  return system.run();
}

static bool
checkPeripheralPorts(SystemState &sys, const PeripheralDescriptor *descriptor,
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
    if (!portArg.lookup(sys)) {
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

typedef std::vector<std::pair<PeripheralDescriptor*, Properties> >
  PeripheralDescriptorWithPropertiesVector;

int
loop(const Options &options)
{
  XE xe(options.file);
  std::auto_ptr<SystemState> statePtr = readXE(xe, options.file);
  SystemState &sys = *statePtr;

  if (!connectLoopbackPorts(sys, options.loopbackPorts)) {
    std::exit(1);
  }

  const PeripheralDescriptorWithPropertiesVector &peripherals =
    options.peripherals;
  for (PeripheralDescriptorWithPropertiesVector::const_iterator it = peripherals.begin(),
       e = peripherals.end(); it != e; ++it) {
    if (!checkPeripheralPorts(sys, it->first, it->second)) {
      std::exit(1);
    }
    it->first->createInstance(sys, it->second);
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
    sys.setRom(&rom[0], 0xffffc000, rom.size());
    return runCoreRoms(sys);
  }

  std::map<std::pair<unsigned, unsigned>,Core*> coreMap;
  addToCoreMap(coreMap, *statePtr);

  std::auto_ptr<SymbolInfo> SIAutoPtr(new SymbolInfo);
  Tracer::get().setSymbolInfo(SIAutoPtr);
  SymbolInfo *SI = Tracer::get().getSymbolInfo();

  std::map<Core*,uint32_t> entryPoints;
  std::set<Core*> gotoSectors;
  std::set<Core*> callSectors;
  for (std::vector<const XESector *>::const_iterator
       it = xe.getSectors().begin(), end = xe.getSectors().end(); it != end;
       ++it) {
    switch((*it)->getType()) {
    case XESector::XE_SECTOR_ELF:
      {
        const XEElfSector *elfSector = static_cast<const XEElfSector*>(*it);
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
          return runCores(sys, gotoSectors, entryPoints);
        }
        if (callSectors.count(core)) {
          int status = runCores(sys, callSectors, entryPoints);
          if (status != 0)
            return status;
          callSectors.clear();
        }
        std::auto_ptr<CoreSymbolInfo> CSI;
        readElf(options.file, elfSector, *core, CSI, entryPoints);
        SI->add(core, CSI);
        // TODO check old instructions are cleared.

        // Patch in syscall instruction at the syscall address.
        if (const ElfSymbol *syscallSym = SI->getGlobalSymbol(core, "_DoSyscall")) {
          if (!core->setSyscallAddress(syscallSym->value)) {
            std::cout << "Warning: invalid _DoSyscall address "
            << std::hex << syscallSym->value << std::dec << "\n";
          }
        }
        // Patch in exception instruction at the exception address
        if (const ElfSymbol *doExceptionSym = SI->getGlobalSymbol(core, "_DoException")) {
          if (!core->setExceptionAddress(doExceptionSym->value)) {
            std::cout << "Warning: invalid _DoException address "
            << std::hex << doExceptionSym->value << std::dec << "\n";
          }
        }
      }
      break;
    case XESector::XE_SECTOR_CALL:
      {
        const XECallOrGotoSector *callSector =
        static_cast<const XECallOrGotoSector*>(*it);
        if (!gotoSectors.empty()) {
          // Shouldn't happen.
          return runCores(sys, gotoSectors, entryPoints);
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
          int status = runCores(sys, callSectors, entryPoints);
          if (status != 0)
            return status;
          callSectors.clear();
          callSectors.insert(core);
        }
      }
      break;
    case XESector::XE_SECTOR_GOTO:
      {
        const XECallOrGotoSector *gotoSector =
          static_cast<const XECallOrGotoSector*>(*it);
        if (!callSectors.empty()) {
          // Handle calls.
          int status = runCores(sys, callSectors, entryPoints);
          if (status != 0)
            return status;
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
          return runCores(sys, gotoSectors, entryPoints);
        }
      }
      break;
    }
  }
  if (!gotoSectors.empty()) {
    return runCores(sys, gotoSectors, entryPoints);
  }
  if (!callSectors.empty()) {
    // Shouldn't happen.
    return runCores(sys, callSectors, entryPoints);
  }
  return 0;
}

int
main(int argc, char **argv) {
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
  return loop(options);
}
