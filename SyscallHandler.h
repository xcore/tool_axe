// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SyscallHandler_h_
#define _SyscallHandler_h_

class SyscallHandler {
public:
  enum SycallOutcome {
    CONTINUE,
    DESCHEDULE,
    EXIT
  };
  static void setCoreCount(unsigned number);
  static SycallOutcome doSyscall(ThreadState &thread, int &retval);
  static void doException(const ThreadState &thread);
};

#endif // _SyscallHandler_h_
