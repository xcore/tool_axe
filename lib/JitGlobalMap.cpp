// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "JitGlobalMap.h"
#include "Array.h"

using namespace axe;

#define DO_FUNCTION(name) extern "C" void name();
#include "JitGlobalMap.inc"
#undef DO_FUNCTION

#define QUOTE_AUX(x) #x
#define QUOTE(x) QUOTE_AUX(x)
const std::pair<const char *, void (*)()> axe::jitFunctionMap[] = {
#define DO_FUNCTION(name) { QUOTE(name), &name },
#include "JitGlobalMap.inc"
};

const unsigned axe::jitFunctionMapSize = arraySize(jitFunctionMap);
