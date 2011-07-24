// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SSwitch.h"
#include <cassert>


SSwitch::SSwitch() :
  recievedTokens(0),
  junkIncomingTokens(0)
{
  setJunkIncoming(false);
}

static uint16_t read16_be(Token *p)
{
  return (p[0].getValue() << 8) | p[1].getValue();
}

static uint32_t read32_be(Token *p)
{
  return (p[0].getValue() << 24) | (p[1].getValue() << 16) |
         (p[2].getValue() << 8) | p[3].getValue();
}

static bool containsControlToken(Token *p, unsigned size)
{
  for (unsigned i = 0; i < size; ++i) {
    if (p[i].isControl())
      return true;
  }
  return false;
}

bool SSwitch::parseRequest(Request &request)
{
  if (recievedTokens == 0)
    return false;
  if (!buf[0].isControl())
    return false;
  unsigned expectedLength;
  switch (buf[0].getValue()) {
  default:
    return false;
  case CT_READC:
    expectedLength = readRequestLength;
    request.write = false;
    break;
  case CT_WRITEC:
    expectedLength = writeRequestLength;
    request.write = true;
    break;
  }
  if (recievedTokens != expectedLength)
    return false;
  if (containsControlToken(&buf[1], expectedLength - 1))
    return false;
  request.returnNode = read16_be(&buf[1]);
  request.returnNum = buf[3].getValue();
  request.regNum = read16_be(&buf[4]);
  if (request.write) {
    request.data = read32_be(&buf[6]);
  } else {
    request.data = 0;
  }
  return true;
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
  if (junkIncomingTokens) {
    return;
  }
  if (recievedTokens == writeRequestLength) {
    junkIncomingTokens = true;
    return;
  }
  buf[recievedTokens++] = Token(value);
}

void SSwitch::receiveDataTokens(ticks_t time, uint8_t *values, unsigned num)
{
  if (junkIncomingTokens) {
    return;
  }
  if (recievedTokens + num > writeRequestLength) {
    junkIncomingTokens = true;
    return;
  }
  for (unsigned i = 0; i < num; i++) {
    buf[recievedTokens++] = Token(values[i]);
  }
}

void SSwitch::receiveCtrlToken(ticks_t time, uint8_t value)
{
  if (value == CT_END) {
    Request request;
    if (!junkIncomingTokens && parseRequest(request)) {
      // TODO
      assert(0);
    }
    recievedTokens = 0;
    junkIncomingTokens = false;
    return;
  }
  if (junkIncomingTokens || value == CT_PAUSE) {
    return;
  }
  if (recievedTokens == writeRequestLength) {
    junkIncomingTokens = true;
    return;
  }
  buf[recievedTokens++] = Token(value, true);
}
