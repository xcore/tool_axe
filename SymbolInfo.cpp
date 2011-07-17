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

static const ElfSymbol *
getSymbol(const std::vector<ElfSymbol*> &symbols, uint32_t address)
{
  ElfSymbol sym("", address, 0);
  std::vector<ElfSymbol*>::const_reverse_iterator it =
  std::lower_bound(symbols.rbegin(), symbols.rend(), &sym,
                   ElfSymbolGreater());
  if (it == symbols.rbegin())
    return 0;
  return *it;
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
      retval->functionSymbols.push_back(sym);
      break;
    case STT_OBJECT:
      retval->dataSymbols.push_back(sym);
      break;
    case STT_NOTYPE:
      retval->functionSymbols.push_back(sym);
      retval->dataSymbols.push_back(sym);
      break;
    }
    if (ELF32_ST_BIND(sym->info) != STB_LOCAL)
      retval->symNameMap.insert(std::make_pair(sym->name, sym));
  }
  std::sort(retval->functionSymbols.begin(), retval->functionSymbols.end(),
            ElfSymbolLess());
  std::sort(retval->dataSymbols.begin(), retval->dataSymbols.end(),
            ElfSymbolLess());
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
  coreMap.insert(std::make_pair(core, info.release()));
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
