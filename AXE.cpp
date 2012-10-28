// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "AXE.h"
#include "JIT.h"
#include <libxml/parser.h>
#include <libxslt/transform.h>
#include "registerAllPeripherals.h"
#include "BootSequence.h"

using namespace axe;

void axe::AXEInitialize(bool beLazy)
{
  /*
   * this initialize the library and check potential ABI mismatches
   * between the version it was compiled for and the actual shared
   * library used.
   */
  LIBXML_TEST_VERSION
  registerAllPeripherals();
  if (!beLazy) {
    xmlInitParser();
    xsltInit();
    JIT::initialize();
    BootSequence::initializeElfHandling();
  }
}

void axe::AXECleanup()
{
  xsltCleanupGlobals();
  xmlCleanupParser();
}
