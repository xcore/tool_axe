// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "StopReason.h"

using namespace axe;

StopReason StopReason::getTimeout()
{
  return StopReason(TIMEOUT);
}

StopReason StopReason::getNoRunnableThreads()
{
  return StopReason(NO_RUNNABLE_THREADS);
}

StopReason StopReason::getBreakpoint(Thread &thread)
{
  StopReason retval(BREAKPOINT);
  retval.thread = &thread;
  return retval;
}

StopReason StopReason::getExit(int status)
{
  StopReason retval(EXIT);
  retval.status = status;
  return retval;
}
