// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _JIT_h_
#define _JIT_h_

#include <stdint.h>

namespace axe {

class Thread;
class Core;
class JITImpl;

class JIT {
  JITImpl *pImpl;
public:
  JIT();
  JIT(const JIT &) = delete;
  ~JIT();
  /// Preallocate global state for the JIT. Normally this state is initialized
  /// when the JIT is first used but in a multi-threaded applications it should
  /// be preallocated to avoid race conditions.
  static void initializeGlobalState();
  void compileBlock(Core &c, uint32_t pc);
  bool invalidate(Core &c, uint32_t pc);
};
  
} // End axe namespace

#endif // _JIT_h_
