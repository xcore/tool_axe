// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SymbolInfo.h"
#include "libelf.h"
#include <algorithm>
#include <cstring>

struct ElfSymbolLess {
  bool operator()(const ElfSymbol *sym1, const ElfSymbol *sym2) {
    return sym1->value < sym2->value;
  }
};

struct ElfSymbolGreater {
  bool operator()(const ElfSymbol *sym1, const ElfSymbol *sym2) {
    return sym1->value > sym2->value;
  }
};

const ElfSymbol *CoreSymbolInfo::getGlobalSymbol(const std::string &name) const
{
  std::map<std::string,ElfSymbol*>::const_iterator it = symNameMap.find(name);
  if (it == symNameMap.end())
    return 0;
  return it->second;
}

const ElfSymbol *CoreSymbolInfo::
getSymbol(const SymbolAddressMap &symbols, uint32_t address)
{
  SymbolAddressMap::const_iterator it = symbols.lower_bound(address);
  if (it == symbols.end())
    return 0;
  return it->second;
}

const ElfSymbol *CoreSymbolInfo::getFunctionSymbol(uint32_t address) const
{
  return getSymbol(functionSymbols, address);
}

const ElfSymbol *CoreSymbolInfo::getDataSymbol(uint32_t address) const
{
  return getSymbol(dataSymbols, address);
}

void CoreSymbolInfoBuilder::
addSymbol(const char *name, uint32_t value, unsigned char info)
{
  symbols.push_back(ElfSymbol(name, value, info));
}

std::auto_ptr<CoreSymbolInfo> CoreSymbolInfoBuilder::getSymbolInfo()
{
  std::auto_ptr<CoreSymbolInfo> retval(new CoreSymbolInfo);

  std::swap(symbols, retval->symbols);
  for (std::vector<ElfSymbol>::iterator it = retval->symbols.begin(),
       e = retval->symbols.end(); it != e; ++it) {
    ElfSymbol *sym = &*it;
    switch (ELF32_ST_TYPE(sym->info)) {
    case STT_FUNC:
      // Could replace an existing symbol.
      retval->functionSymbols[sym->value] = sym;
      break;
    case STT_OBJECT:
      // Could replace an existing symbol.
      retval->dataSymbols[sym->value] = sym;
      break;
    case STT_NOTYPE:
      // Only inserted if no other symbol at that address.
      retval->functionSymbols.insert(std::make_pair(sym->value, sym));
      retval->dataSymbols.insert(std::make_pair(sym->value, sym));
      break;
    }
    if (ELF32_ST_BIND(sym->info) != STB_LOCAL)
      retval->symNameMap.insert(std::make_pair(sym->name, sym));
  }
  return retval;
}

SymbolInfo::~SymbolInfo()
{
  for (std::map<const Core*,CoreSymbolInfo*>::iterator it = coreMap.begin(),
       e = coreMap.end(); it != e; ++it) {
    delete it->second;
  }
}

void SymbolInfo::add(const Core *core, std::auto_ptr<CoreSymbolInfo> info)
{
  CoreSymbolInfo *&entry = coreMap[core];
  if (entry) {
    delete entry;
  }
  entry = info.release();
}

CoreSymbolInfo *SymbolInfo::getCoreSymbolInfo(const Core *core) const
{
  std::map<const Core*,CoreSymbolInfo*>::const_iterator it =
    coreMap.find(core);
  if (it == coreMap.end())
    return 0;
  return it->second;
}

const ElfSymbol *SymbolInfo::
getGlobalSymbol(const Core *core, const std::string &name) const
{
  if (CoreSymbolInfo *CSI = getCoreSymbolInfo(core)) {
    return CSI->getGlobalSymbol(name);
  }
  return 0;
}

const ElfSymbol *SymbolInfo::
getFunctionSymbol(const Core *core, uint32_t address) const
{
  if (CoreSymbolInfo *CSI = getCoreSymbolInfo(core)) {
    return CSI->getFunctionSymbol(address);
  }
  return 0;
}

const ElfSymbol *SymbolInfo::
getDataSymbol(const Core *core, uint32_t address) const
{
  if (CoreSymbolInfo *CSI = getCoreSymbolInfo(core)) {
    return CSI->getDataSymbol(address);
  }
  return 0;
}
