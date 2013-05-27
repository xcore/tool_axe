// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _JITInstructionFunction_h_
#define _JITInstructionFunction_h_

namespace axe {

class Thread;

enum class InstReturn {
  /// Continue the current trace.
  CONTINUE,
  /// End the current trace.
  END_TRACE,
  /// End execution of the current thread.
  END_THREAD_EXECUTION,
};

typedef InstReturn (*InstFunction_t)(Thread &);
  
} // End axe namespace

#endif // _JITInstructionFunction_h_
