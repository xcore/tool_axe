// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SSwitch.h"
#include <cassert>


SSwitch::SSwitch()
{
  setJunkIncoming(false);
}

void SSwitch::notifyDestClaimed(ticks_t time)
{
  // TODO
  assert(0);
}

void SSwitch::notifyDestCanAcceptTokens(ticks_t time, unsigned tokens)
{
  // TODO
  assert(0);
}

bool SSwitch::canAcceptToken()
{
  return true;
}

bool SSwitch::canAcceptTokens(unsigned tokens)
{
  return true;
}

void SSwitch::receiveDataToken(ticks_t time, uint8_t value)
{
}

void SSwitch::receiveDataTokens(ticks_t time, uint8_t *values, unsigned num)
{
  
}

void SSwitch::receiveCtrlToken(ticks_t time, uint8_t value)
{
  
}
