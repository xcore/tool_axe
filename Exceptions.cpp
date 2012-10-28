// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Exceptions.h"

using namespace axe;

const char *Exceptions::getExceptionName(int type)
{
  switch (type) {
  case ET_LINK_ERROR: return "LINK_ERROR";
  case ET_ILLEGAL_PC: return "ILLEGAL_PC";
  case ET_ILLEGAL_INSTRUCTION: return "ILLEGAL_INSTRUCTION";
  case ET_ILLEGAL_RESOURCE: return "ILLEGAL_RESOURCE";
  case ET_LOAD_STORE: return "LOAD_STORE";
  case ET_ILLEGAL_PS: return "ILLEGAL_PS";
  case ET_ARITHMETIC: return "ARITHMETIC";
  case ET_ECALL: return "ECALL";
  case ET_RESOURCE_DEP: return "RESOURCE_DEP";
  case ET_KCALL: return "KCALL";
  default: return "Unknown";
  }
}
