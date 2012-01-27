// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _JITInstructionFunction_h_
#define _JITInstructionFunction_h_

class Thread;

enum JITReturn {
  /// Continue the current trace.
  JIT_RETURN_CONTINUE,
  /// End the current trace.
  JIT_RETURN_END_TRACE,
  /// End the current trace and yield to allow other threads to run.
  JIT_RETURN_YIELD,
};

typedef void (*JITInstructionFunction_t)(Thread &);

#endif // _JITInstructionFunction_h_
