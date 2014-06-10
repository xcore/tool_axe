// Copyright (c) 2014, XMOS Ltd., All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _LoadedElf_h_
#define _LoadedElf_h_

#include <string>
#include <libelf.h>

namespace axe {

class XEElfSector;

class LoadedElf {
  char *buf;
  Elf *elf;
public:
  LoadedElf(const XEElfSector *elfSector);
  LoadedElf(const LoadedElf &) = delete;
  LoadedElf &operator=(const LoadedElf &) = delete;
  ~LoadedElf();
  Elf *getElf() const { return elf; }
  const char *getBuf() const { return buf; }
};

} // End axe namespace

#endif // _LoadedElf_h_
