// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SymbolInfo_h_
#define _SymbolInfo_h_

#include <stdint.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <functional>

namespace axe {

struct ElfSymbol {
  std::string name;
  uint32_t value;
  unsigned char info;
  
  ElfSymbol(std::string const &s, uint32_t val, unsigned char i)
    : name(s), value(val), info(i) { }
};

class CoreSymbolInfo {
friend class CoreSymbolInfoBuilder;
private:
  std::vector<ElfSymbol> symbols;
  typedef std::map<uint32_t, ElfSymbol*, std::greater<uint32_t>>
    SymbolAddressMap;
  SymbolAddressMap functionSymbols;
  SymbolAddressMap dataSymbols;
  std::map<std::string,ElfSymbol*> symNameMap;
  static const ElfSymbol *getSymbol(const SymbolAddressMap &symbols,
                                    uint32_t address);
public:
  const ElfSymbol *getGlobalSymbol(const std::string &name) const;
  const ElfSymbol *getFunctionSymbol(uint32_t address) const;
  const ElfSymbol *getDataSymbol(uint32_t address) const;
};

class CoreSymbolInfoBuilder {
private:
  std::vector<ElfSymbol> symbols;
public:
  void addSymbol(const char *name, uint32_t value, unsigned char info);
  std::auto_ptr<CoreSymbolInfo> getSymbolInfo();
};

class Core;

class SymbolInfo {
private:
  std::map<const Core*,CoreSymbolInfo*> coreMap;
  CoreSymbolInfo *getCoreSymbolInfo(const Core *core) const;
  SymbolInfo(const SymbolInfo &); // Not implemented.
  void operator=(const SymbolInfo &); // Not implemented.
public:
  SymbolInfo() {}
  ~SymbolInfo();
  void add(const Core *core, std::auto_ptr<CoreSymbolInfo> info);
  const ElfSymbol *getGlobalSymbol(const Core *core,
                                   const std::string &name) const;
  const ElfSymbol *getFunctionSymbol(const Core *core,
                                     uint32_t address) const;
  const ElfSymbol *getDataSymbol(const Core *core,
                                 uint32_t address) const;
};
  
} // End axe namespace

#endif // _SymbolInfo_h_
