// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Exceptions.h"
#include "Core.h"
#include "SyscallHandler.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "ScopedArray.h"
#include "Trace.h"

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define dup _dup
#define open _open
#define close _close
#define read _read
#define write _write
#define lseek _lseek
#endif

using namespace Register;

class SyscallHandlerImpl {
private:
  const scoped_array<int> fds;
  bool tracing;
  unsigned doneSyscallsRequired;
  char *getString(Thread &thread, uint32_t address);
  void *getBuffer(Thread &thread, uint32_t address, uint32_t size);
  int getNewFd();
  bool isValidFd(int fd);
  int convertOpenFlags(int flags);
  int convertOpenMode(int mode);
  bool convertLseekType(int whence, int &converted);
  void doException(const Thread &state, uint32_t et, uint32_t ed);

public:
  SyscallHandlerImpl();

  void setDoneSyscallsRequired(unsigned count) { doneSyscallsRequired = count; }
  SyscallHandler::SycallOutcome doSyscall(Thread &thread, int &retval);
  void doException(const Thread &thread);

  static SyscallHandlerImpl instance;
};

enum SyscallType {
  OSCALL_EXIT = 0,
  OSCALL_PRINTC = 1,
  OSCALL_PRINTINT = 2,
  OSCALL_OPEN = 3,
  OSCALL_CLOSE = 4,
  OSCALL_READ = 5,
  OSCALL_WRITE = 6,
  OSCALL_DONE = 7,
  OSCALL_LSEEK = 8,
  OSCALL_RENAME = 9,
  OSCALL_TIME = 10,
  OSCALL_REMOVE = 11,
  OSCALL_SYSTEM = 12,
  OSCALL_EXCEPTION = 13,
  OSCALL_IS_SIMULATION = 99
};

enum LseekType {
  XCORE_SEEK_CUR = 1,
  XCORE_SEEK_END = 2,
  XCORE_SEEK_SET = 0
};

enum OpenMode {
  XCORE_S_IREAD = 0400,
  XCORE_S_IWRITE = 0200,
  XCORE_S_IEXEC = 0100
};

enum OpenFlags {
  XCORE_O_RDONLY = 0x0001,
  XCORE_O_WRONLY = 0x0002,
  XCORE_O_RDWR = 0x0004,
  XCORE_O_CREAT = 0x0100,
  XCORE_O_TRUNC = 0x0200,
  XCORE_O_APPEND = 0x0800,
  XCORE_O_BINARY = 0x8000
};

const unsigned MAX_FDS = 512;

SyscallHandlerImpl::SyscallHandlerImpl() :
  fds(new int[MAX_FDS]), tracing(false), doneSyscallsRequired(1)
{
  // Duplicate the standard file descriptors.
  fds[0] = dup(STDIN_FILENO);
  fds[1] = dup(STDOUT_FILENO);
  fds[2] = dup(STDERR_FILENO);
  // The rest are initialised to -1.
  for (unsigned i = 3; i < MAX_FDS; i++) {
    fds[i] = - 1;
  }
}

/// Returns a pointer to a string in memory at the given address.
/// Returns 0 if the address is invalid or the string is not null terminated.
char *SyscallHandlerImpl::getString(Thread &thread, uint32_t startAddress)
{
  Core &core = thread.getParent();
  if (!core.isValidAddress(startAddress))
    return 0;
  // Check the string is null terminated
  uint32_t address = startAddress;
  uint32_t end = core.getRamSize() + core.ram_base;
  for (; address < end && core.loadByte(address); address++) {}
  if (address >= end) {
    return 0;
  }
  return (char *)&core.byte(address);
}

/// Returns a pointer to a buffer in memory of the given size.
/// Returns 0 if the buffer address is invalid.
void *SyscallHandlerImpl::
getBuffer(Thread &thread, uint32_t address, uint32_t size)
{
  Core &core = thread.getParent();
  if (!core.isValidAddress(address) || !core.isValidAddress(address + size))
    return 0;
  return (void *)&core.byte(address);
}

/// Either returns an unused number for a file descriptor or -1 if there are no
/// file descriptors available.
int SyscallHandlerImpl::getNewFd()
{
  for (unsigned i = 0; i < MAX_FDS; i++) {
    if (fds[i] == -1) {
      return i;
    }
  }
  return -1;
}

bool SyscallHandlerImpl::isValidFd(int fd)
{
  return fd >= 0 && (unsigned)fd < MAX_FDS && fds[fd] != -1;
}

int SyscallHandlerImpl::convertOpenFlags(int flags)
{
  int converted = 0;
  if (flags & XCORE_O_RDONLY)
    converted |= O_RDONLY;
  if (flags & XCORE_O_WRONLY)
    converted |= O_WRONLY;
  if (flags & XCORE_O_RDWR)
    converted |= O_RDWR;
  if (flags & XCORE_O_CREAT)
    converted |= O_CREAT;
  if (flags & XCORE_O_TRUNC)
    converted |= O_TRUNC;
  if (flags & XCORE_O_APPEND)
    converted |= O_APPEND;
#ifdef O_BINARY
  if (flags & XCORE_O_BINARY)
    converted |= O_BINARY;
#endif
  return converted;
}

int SyscallHandlerImpl::convertOpenMode(int mode)
{
  int converted = 0;
  if (mode & XCORE_S_IREAD)
    converted |= S_IREAD;
  if (mode & XCORE_S_IWRITE)
    converted |= S_IWRITE;
  if (mode & XCORE_S_IEXEC)
    converted |= S_IEXEC;
  return converted;
}

bool SyscallHandlerImpl::convertLseekType(int whence, int &converted)
{
  switch (whence) {
  case XCORE_SEEK_CUR:
    converted = SEEK_CUR;
    return true;
  case XCORE_SEEK_END:
    converted = SEEK_END;
    return true;
  case XCORE_SEEK_SET:
    converted = SEEK_SET;
    return true;
  default:
    return false;
  }
}


void SyscallHandlerImpl::doException(const Thread &thread, uint32_t et, uint32_t ed)
{
  std::cout << "Unhandled exception: "
            << Exceptions::getExceptionName(et)
            << ", data: 0x" << std::hex << ed << std::dec << "\n";
  std::cout << "Register state:\n";
  thread.dump();
}

void SyscallHandlerImpl::doException(const Thread &thread)
{
  doException(thread, thread.regs[ET], thread.regs[ED]);
}

#define TRACE_BEGIN() \
do { \
if (Tracer::get().getTracingEnabled()) { \
Tracer::get().syscall(thread); \
} \
} while(0)
#define TRACE_END() \
do { \
if (Tracer::get().getTracingEnabled()) { \
Tracer::get().traceEnd(); \
} \
} while(0)
#define TRACE(...) \
do { \
if (Tracer::get().getTracingEnabled()) { \
Tracer::get().syscall(thread, __VA_ARGS__); \
Tracer::get().traceEnd(); \
} \
} while(0)
#define TRACE_REG_WRITE(register, value) \
do { \
if (Tracer::getInstance().getTracingEnabled()) { \
Tracer::getInstance().regWrite(register, value); \
} \
} while(0)

SyscallHandler::SycallOutcome SyscallHandlerImpl::
doSyscall(Thread &thread, int &retval)
{
  switch (thread.regs[R0]) {
  case OSCALL_EXIT:
    TRACE("exit", thread.regs[R1]);
    retval = thread.regs[R1];
    return SyscallHandler::EXIT;
  case OSCALL_DONE:
    TRACE("done");
    if (--doneSyscallsRequired == 0) {
      retval = 0;
      return SyscallHandler::EXIT;
    }
    return SyscallHandler::DESCHEDULE;
  case OSCALL_EXCEPTION:
    doException(thread, thread.regs[R1], thread.regs[R2]);
    retval = 1;
    return SyscallHandler::EXIT;
  case OSCALL_OPEN:
    {
      uint32_t PathAddr = thread.regs[R1];
      const char *path = getString(thread, PathAddr);
      if (!path) {
        // Invalid argument
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      int flags = convertOpenFlags(thread.regs[R2]);
      int mode = convertOpenMode(thread.regs[R3]);
      int fd = getNewFd();
      if (fd == -1) {
        // No free file descriptors
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      int hostfd = open(path, flags, mode);
      if (hostfd == -1) {
        // Call to open failed.
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      fds[fd] = hostfd;
      thread.regs[R0] = fd;
      return SyscallHandler::CONTINUE;
    }
  case OSCALL_CLOSE:
    {
      if (!isValidFd(thread.regs[R1])) {
        // Invalid fd
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      int retval = close(fds[thread.regs[R1]]);
      if (retval == 0) {
        fds[thread.regs[R1]] = (uint32_t)-1;
      }
      thread.regs[R0] = retval;
      return SyscallHandler::CONTINUE;
    }
  case OSCALL_READ:
    {
      if (!isValidFd(thread.regs[R1])) {
        // Invalid fd
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      void *buf = getBuffer(thread, thread.regs[R2], thread.regs[R3]);
      if (!buf) {
        // Invalid buffer
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      thread.regs[R0] = read(fds[thread.regs[R1]], buf, thread.regs[R3]);
      return SyscallHandler::CONTINUE;
    }
  case OSCALL_WRITE:
    {
      if (!isValidFd(thread.regs[R1])) {
        // Invalid fd
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      const void *buf = getBuffer(thread, thread.regs[R2], thread.regs[R3]);
      if (!buf) {
        // Invalid buffer
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      thread.regs[R0] = write(fds[thread.regs[R1]], buf, thread.regs[R3]);
      return SyscallHandler::CONTINUE;
    }
  case OSCALL_LSEEK:
    {
      if (!isValidFd(thread.regs[R1])) {
        // Invalid fd
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      int whence;
      if (! convertLseekType(thread.regs[R3], whence)) {
        // Invalid seek type
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      thread.regs[R0] = lseek(fds[thread.regs[R1]], thread.regs[R2], whence);
      return SyscallHandler::CONTINUE;
    }
  case OSCALL_RENAME:
    {
      uint32_t OldPathAddr = thread.regs[R1];
      uint32_t NewPathAddr = thread.regs[R2];
      const char *oldpath = getString(thread, OldPathAddr);
      const char *newpath = getString(thread, NewPathAddr);
      if (!oldpath || !newpath) {
        // Invalid argument
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      thread.regs[R0] = std::rename(oldpath, newpath);
      return SyscallHandler::CONTINUE;
    }
  case OSCALL_TIME:
    {
      uint32_t TimeAddr = thread.regs[R1];
      uint32_t Time = (uint32_t)std::time(0);
      Core &core = thread.getParent();
      if (TimeAddr != 0) {
        if (!core.isValidAddress(TimeAddr) || (TimeAddr & 3)) {
          // Invalid address
          thread.regs[R0] = (uint32_t)-1;
          return SyscallHandler::CONTINUE;
        }
        core.storeWord(Time, TimeAddr);
        core.invalidateWord(TimeAddr);
      }
      thread.regs[R0] = Time;
      return SyscallHandler::CONTINUE;
    }
  case OSCALL_REMOVE:
    {
      uint32_t FileAddr = thread.regs[R1];
      const char *file = getString(thread, FileAddr);
      if (file == 0) {
        // Invalid argument
        thread.regs[R0] = (uint32_t)-1;
        return SyscallHandler::CONTINUE;
      }
      thread.regs[R0] = std::remove(file);
      return SyscallHandler::CONTINUE;
    }
  case OSCALL_SYSTEM:
    {
      uint32_t CmdAddr = thread.regs[R1];
      const char *command = 0;
      if (CmdAddr != 0) {
        command = getString(thread, CmdAddr);
        if (command == 0) {
          // Invalid argument
          thread.regs[R0] = (uint32_t)-1;
          return SyscallHandler::CONTINUE;
        }
      }
      thread.regs[R0] = std::system(command);
      return SyscallHandler::CONTINUE;
    }
  case OSCALL_IS_SIMULATION:
    thread.regs[R0] = 1;
    return SyscallHandler::CONTINUE;
  default:
    std::cout << "Error: unknown system call number: " << thread.regs[R0] << "\n";
    retval = 1;
    return SyscallHandler::EXIT;
  }
}

SyscallHandlerImpl SyscallHandlerImpl::instance;

void SyscallHandler::setDoneSyscallsRequired(unsigned number)
{
  SyscallHandlerImpl::instance.setDoneSyscallsRequired(number);
}

SyscallHandler::SycallOutcome SyscallHandler::
doSyscall(Thread &thread, int &retval)
{
  return SyscallHandlerImpl::instance.doSyscall(thread, retval);
}

void SyscallHandler::doException(const Thread &thread)
{
  return SyscallHandlerImpl::instance.doException(thread);
}
