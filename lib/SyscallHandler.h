// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SyscallHandler_h_
#define _SyscallHandler_h_

#include "ScopedArray.h"
#include <set>
#include <string>
#include <vector>
#include <boost/function.hpp>

namespace axe {

class Core;

class SyscallHandler {
private:
  const scoped_array<int> fds;
  std::set<Core*> doneSyscallsSeen;
  unsigned doneSyscallsRequired;
  struct {
    std::vector<std::string> arg;
    int minBufBytes;
  } cmdLine;
  char *getString(Thread &thread, uint32_t address);
  const void *getBuffer(Thread &thread, uint32_t address, uint32_t size);
  void *getRamBuffer(Thread &thread, uint32_t address, uint32_t size);
  int getNewFd();
  bool isValidFd(int fd);
  int convertOpenFlags(int flags);
  int convertOpenMode(int mode);
  bool convertLseekType(int whence, int &converted);
  void doException(const Thread &state, uint32_t et, uint32_t ed);
  boost::function<bool (Core &, void *, uint32_t, uint32_t)> loadImageCallback;
  
public:
  enum SycallOutcome {
    CONTINUE,
    EXIT
  };
  SyscallHandler();
  
  void setCmdLine(int clientArgc, char **clientArgv);
  void setDoneSyscallsRequired(unsigned count);
  void setLoadImageCallback(
    const boost::function<bool (Core &,void *,uint32_t,uint32_t)> &callback) {
    loadImageCallback = callback;
  }
  SycallOutcome doSyscall(Thread &thread, int &retval);
  void doException(const Thread &thread);
};
  
} // End axe namespace

#endif // _SyscallHandler_h_
