// Copyright (c) 2014, XMOS Ltd., All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "TrapInfo.h"
#include "Endianness.h"
#include "LoadedElf.h"
#include <gelf.h>
#include <libelf.h>

using namespace axe;

bool findNamedSection(Elf *elf, const char *name, GElf_Shdr &shdr)
{
  size_t shdrstrndx;
  if (elf_getshdrstrndx(elf, &shdrstrndx) != 0)
    return false;
  GElf_Ehdr ehdr;
  if (!gelf_getehdr(elf, &ehdr))
    return false;
  unsigned num_shdrs = ehdr.e_shnum;
  for (unsigned i = 0; i < num_shdrs; i++) {
    Elf_Scn *scn = elf_getscn(elf, i);
    if (!scn)
      continue;
    if (gelf_getshdr(scn, &shdr) == NULL)
      continue;
    const char *shname = elf_strptr(elf, shdrstrndx, shdr.sh_name);
    if (shname && strcmp(shname, name) == 0)
      return true;
  }
  return false;
}

#include <iostream>

static bool
getDescriptionFromCompileUnit(const uint8_t *&begin, const uint8_t *end,
                              uint32_t spc, uint32_t &strp)
{
  using namespace endianness;
  auto maxLength = end - begin;
  if (maxLength < 8) {
    begin = end;
    return false;
  }
  uint32_t size = read32le(begin);
  uint32_t version = read32le(begin + 4);
  if (size < 8 || size > maxLength) {
    begin = end;
    return false;
  }
  if (version == 1) {
    unsigned numEntries = (size - 8) / 8;
    for (unsigned i = 0; i < numEntries; i++) {
      uint32_t address = read32le(begin + 8 + i * 8);
      if (address == spc) {
        strp = read32le(begin + 8 + i * 8 + 4);
        return true;
      }
    }
  }
  begin += size;
  return false;
}

static bool
getStringPtrFromTrapInfoSection(const uint8_t *begin, const uint8_t *end,
                                uint32_t spc, uint32_t &strp)
{
  while (begin != end) {
    uint32_t strp;
    if (getDescriptionFromCompileUnit(begin, end, spc, strp)) {
      return true;
    }
  }
  return false;
}

/// Lookup the description of the trap with the specified pc in the .trap_info
/// section. This function parses the .trap_info section each time it is called.
/// The expectation is that the trap information will be queried at most once
/// and so there is no point in building a data structure for efficient lookup.
bool axe::
getDescriptionFromTrapInfoSection(const LoadedElf *loadedElf, uint32_t spc,
                                  std::string &desc)
{
  GElf_Shdr trapInfo, trapInfoStr;
  if (!findNamedSection(loadedElf->getElf(), ".trap_info", trapInfo) ||
      !findNamedSection(loadedElf->getElf(), ".trap_info_str", trapInfoStr))
    return false;
  auto begin =
    reinterpret_cast<const uint8_t*>(loadedElf->getBuf()) + trapInfo.sh_offset;
  auto end = begin + trapInfo.sh_size;
  uint32_t strp;
  if (!getStringPtrFromTrapInfoSection(begin, end, spc, strp))
    return false;
  if (strp >= trapInfoStr.sh_size)
    return false;
  desc = loadedElf->getBuf() + trapInfoStr.sh_offset + strp;
  return true;
}
