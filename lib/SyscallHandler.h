// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SyscallHandler_h_
#define _SyscallHandler_h_

#include "ScopedArray.h"
#include <set>

namespace axe {

class Core;

class SyscallHandler {
private:
  const scoped_array<int> fds;
  std::set<Core*> doneSyscallsSeen;
  unsigned doneSyscallsRequired;
  char *getString(Thread &thread, uint32_t address);
  const void *getBuffer(Thread &thread, uint32_t address, uint32_t size);
  void *getRamBuffer(Thread &thread, uint32_t address, uint32_t size);
  int getNewFd();
  bool isValidFd(int fd);
  int convertOpenFlags(int flags);
  int convertOpenMode(int mode);
  bool convertLseekType(int whence, int &converted);
  void doException(const Thread &state, uint32_t et, uint32_t ed);
  
public:
  enum SycallOutcome {
    CONTINUE,
    EXIT
  };
  SyscallHandler();
  
  void setDoneSyscallsRequired(unsigned count);
  SycallOutcome doSyscall(Thread &thread, int &retval);
  void doException(const Thread &thread);
};
  
} // End axe namespace

#endif // _SyscallHandler_h_
