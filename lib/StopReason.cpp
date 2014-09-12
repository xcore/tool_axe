// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "StopReason.h"

using namespace axe;

StopReason StopReason::getTimeout(ticks_t time)
{
  return StopReason(TIMEOUT, time);
}

StopReason StopReason::getNoRunnableThreads(ticks_t time)
{
  return StopReason(NO_RUNNABLE_THREADS, time);
}

StopReason StopReason::getBreakpoint(ticks_t time, Thread &thread)
{
  StopReason retval(BREAKPOINT, time);
  retval.thread = &thread;
  return retval;
}

StopReason StopReason::getWatchpoint(ticks_t time, Thread &thread)
{
	StopReason retval(WATCHPOINT, time);
	retval.thread = &thread;
	return retval;
}

StopReason StopReason::getExit(ticks_t time, int status)
{
  StopReason retval(EXIT, time);
  retval.status = status;
  return retval;
}
