// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _JitGlobalMap_h_
#define _JitGlobalMap_h_

#include <utility>

namespace axe {

extern const std::pair<const char *, void (*)()> jitFunctionMap[];
extern const unsigned jitFunctionMapSize;

}

#endif // _JitGlobalMap_h_
