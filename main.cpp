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
#include "PeripheralRegistry.h"
#include "registerAllPeripherals.h"
#include "UartRx.h"
#include "Property.h"
#include "PortArg.h"
#include "JIT.h"

#define XCORE_ELF_MACHINE 0xB49E

const char configSchema[] = {
#include "ConfigSchema.inc"
};

static void printUsage(const char *ProgName) {
  std::cout << "Usage: " << ProgName << " [options] filename\n";
  std::cout <<
"General Options:\n"
"  -help                       Display this information.\n"
"  --loopback PORT1 PORT2      Connect PORT1 to PORT2.\n"
"  --vcd FILE                  Write VCD trace to FILE.\n"
"  -t                          Enable instruction tracing.\n"
"\n"
"Peripherals:\n";
  for (PeripheralRegistry::iterator it = PeripheralRegistry::begin(),
       e = PeripheralRegistry::end(); it != e; ++it) {
    PeripheralDescriptor *periph = *it;
    std::cout << "  --" << periph->getName() << ' ';
    bool needComma = false;
    for (PeripheralDescriptor::iterator propIt = periph->properties_begin(),
         propE = periph->properties_end(); propIt != propE; ++propIt) {
      const PropertyDescriptor &prop = *propIt;
      if (needComma)
        std::cout << ',';
      std::cout << prop.getName() << '=';
      needComma = true;
    }
    std::cout << '\n';
  }
}

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
  uint32_t ram_base = core.ram_base;
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
    uint32_t offset = phdr.p_paddr - core.ram_base;
    if (offset > ram_size || offset + phdr.p_filesz > ram_size || offset + phdr.p_memsz > ram_size) {
      std::cerr << "Error data from ELF program header " << i << " does not fit in memory" << std::endl;
      std::exit(1);
    }
    std::memcpy(core.mem() + offset, &buf[phdr.p_offset], phdr.p_filesz);
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
  std::auto_ptr<Node> node(new Node(nodeType));
  long nodeID = readNumberAttribute(config, "number");
  nodeNumberMap.insert(std::make_pair(nodeID, node.get()));
  for (xmlNode *child = config->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE ||
        strcmp("Processor", (char*)child->name) != 0)
      continue;
    node->addCore(createCoreFromConfig(child));
  }
  node->setNodeID(nodeID);
  return node;
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
  xmlNode *jtag = findChild(system, "JtagChain");
  unsigned jtagIndex = 0;
  for (xmlNode *child = jtag->children; child; child = child->next) {
    if (child->type != XML_ELEMENT_NODE ||
        strcmp("Node", (char*)child->name) != 0)
      continue;
    long nodeID = readNumberAttribute(child, "id");
    std::map<long,Node*>::iterator it = nodeNumberMap.find(nodeID);
    if (it == nodeNumberMap.end()) {
      std::cerr << "No node matching id " << nodeID << std::endl;
      std::exit(1);
    }
    it->second->setJtagIndex(jtagIndex++);
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
readXE(const char *filename, SymbolInfo &SI,
       std::set<Core*> &coresWithImage, std::map<Core*,uint32_t> &entryPoints)
{
  // Load the file into memory.
  XE xe(filename);
  if (!xe) {
    std::cerr << "Error opening \"" << filename << "\"" << std::endl;
    std::exit(1);
  }
  // TODO handle XEs / XBs without a config sector.
  const XESector *configSector = xe.getConfigSector();
  if (!configSector) {
    std::cerr << "Error: No config file found in \"" << filename << "\"" << std::endl;
    std::exit(1);
  }
  std::auto_ptr<SystemState> system =
    createSystemFromConfig(filename, configSector);
  std::map<std::pair<unsigned, unsigned>,Core*> coreMap;
  addToCoreMap(coreMap, *system);
  for (std::vector<const XESector *>::const_reverse_iterator
       it = xe.getSectors().rbegin(), end = xe.getSectors().rend(); it != end;
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
        if (coresWithImage.count(core))
          continue;
        std::auto_ptr<CoreSymbolInfo> CSI;
        readElf(filename, elfSector, *core, CSI, entryPoints);
        SI.add(core, CSI);
        coresWithImage.insert(core);
        break;
      }
    }
  }
  xe.close();
  return system;
}

typedef std::vector<std::pair<PeripheralDescriptor*, Properties> >
  PeripheralDescriptorWithPropertiesVector;

int
loop(const char *filename, const LoopbackPorts &loopbackPorts,
     const std::string &vcdFile,
     const PeripheralDescriptorWithPropertiesVector &peripherals)
{
  std::auto_ptr<SymbolInfo> SI(new SymbolInfo);
  std::set<Core*> coresWithImage;
  std::map<Core*,uint32_t> entryPoints;
  std::auto_ptr<SystemState> statePtr = readXE(filename, *SI, coresWithImage,
                                               entryPoints);
  SystemState &sys = *statePtr;

  if (!connectLoopbackPorts(sys, loopbackPorts)) {
    std::exit(1);
  }

  for (PeripheralDescriptorWithPropertiesVector::const_iterator it = peripherals.begin(),
       e = peripherals.end(); it != e; ++it) {
    it->first->createInstance(sys, it->second);
  }

  std::auto_ptr<WaveformTracer> waveformTracer;
  // TODO update to handle multiple cores.
  if (!vcdFile.empty()) {
    waveformTracer.reset(new WaveformTracer(vcdFile));
    connectWaveformTracer(sys, *waveformTracer);
  }

  for (std::set<Core*>::iterator it = coresWithImage.begin(),
       e = coresWithImage.end(); it != e; ++it) {
    Core *core = *it;
    sys.schedule(core->getThread(0));

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
    std::map<Core*,uint32_t>::iterator match;
    if ((match = entryPoints.find(core)) != entryPoints.end()) {
      uint32_t entryPc = core->physicalAddress(match->second) >> 1;
      if (entryPc < core->getRamSizeShorts()) {
        core->getThread(0).pc = entryPc;
      } else {
        std::cout << "Warning: invalid ELF entry point 0x";
        std::cout << std::hex << match->second << std::dec << "\n";
      }
    }
  }
  SyscallHandler::setCoreCount(coresWithImage.size());

  Tracer::get().setSymbolInfo(SI);
  return sys.run();
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

static Property
parseIntegerProperty(const PropertyDescriptor *prop, const std::string &s)
{
  char *endp;
  long value = std::strtol(s.c_str(), &endp, 0);
  if (*endp != '\0') {
    std::cerr << "Error: property " << prop->getName();
    std::cerr << " requires an integer argument\n";
    std::exit(1);
  }
  return Property::integerProperty(prop, value);
}

static Property
parsePortProperty(const PropertyDescriptor *prop, const std::string &s)
{
  PortArg portArg;
  if (!PortArg::parse(s, portArg)) {
    std::cerr << "Error: property " << prop->getName();
    std::cerr << " requires an port argument\n";
    std::exit(1);
  }
  return Property::portProperty(prop, portArg);
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
        properties.set(parseIntegerProperty(prop, value));
        break;
      case PropertyDescriptor::STRING:
        properties.set(Property::stringProperty(prop, value));
        break;
      case PropertyDescriptor::PORT:
        properties.set(parsePortProperty(prop, value));
        break;
      }
    } else {
      std::cerr << "Error: Unknown property \"" << name << "\"";
      std::cerr << " for " << periph->getName() << std::endl;
      std::exit(1);
    }
  }
  // Check required properties have been set.
  for (PeripheralDescriptor::const_iterator it = periph->properties_begin(),
       e = periph->properties_end(); it != e; ++it) {
    if (it->getRequired() && !properties.get(it->getName())) {
      std::cerr << "Error: Required property \"" << it->getName() << "\"";
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

int
main(int argc, char **argv) {
  registerAllPeripherals();
  if (argc < 2) {
    printUsage(argv[0]);
    return 1;
  }
  const char *file = 0;
  bool tracing = false;
  LoopbackPorts loopbackPorts;
  std::string vcdFile;
  std::string arg;
  std::vector<std::pair<PeripheralDescriptor*, Properties> > peripherals;
  for (int i = 1; i < argc; i++) {
    arg = argv[i];
    if (arg == "-t") {
      tracing = true;
    } else if (arg == "--vcd") {
      if (i + 1 > argc) {
        printUsage(argv[0]);
        return 1;
      }
      vcdFile = argv[i + 1];
      i++;
    } else if (arg == "--loopback") {
      if (i + 2 >= argc) {
        printUsage(argv[0]);
        return 1;
      }
      loopbackOption(argv[i + 1], argv[i + 2], loopbackPorts);
      i += 2;
    } else if (arg == "--help") {
      printUsage(argv[0]);
      return 0;
    } else if (PeripheralDescriptor *pd = parsePeripheralOption(arg)) {
      peripherals.push_back(std::make_pair(pd, Properties()));
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        parseProperties(argv[++i], pd, peripherals.back().second);
      }
    } else {
      if (file) {
        printUsage(argv[0]);
        return 1;
      }
      file = argv[i];
    }
  }
  if (!file) {
    printUsage(argv[0]);
    return 1;
  }
#ifndef _WIN32
  if (isatty(fileno(stdout))) {
    Tracer::get().setColour(true);
  }
#endif
  if (tracing) {
    Tracer::get().setTracingEnabled(tracing);
  }
  return loop(file, loopbackPorts, vcdFile, peripherals);
}
