// Copyright (c) 2014, XMOS Ltd., All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "LoadedElf.h"
#include "XE.h"
#include <iostream>
#include <cstdlib>

using namespace axe;

LoadedElf::LoadedElf(const XEElfSector *elfSector) :
  buf(new char[elfSector->getElfSize()])
{
  if (!elfSector->getElfData(buf)) {
    std::cerr << "Error reading ELF data from ELF sector" << std::endl;
    std::exit(1);
  }
  if ((elf = elf_memory(buf, elfSector->getElfSize())) == NULL) {
    std::cerr << "Error reading ELF: " << elf_errmsg(-1) << std::endl;
    std::exit(1);
  }
}

LoadedElf::~LoadedElf()
{
  delete[] buf;
  if (elf)
    elf_end(elf);
}
