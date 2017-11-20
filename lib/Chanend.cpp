// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "LoggingTracer.h"
#include "Chanend.h"
#include "Core.h"
#include <algorithm>
#include "llvm/Support/raw_ostream.h"

using namespace axe;

bool Chanend::canAcceptToken()
{
  return !buf.full();
}

bool Chanend::canAcceptTokens(unsigned tokens)
{
  return buf.remaining() >= tokens;
}

void Chanend::receiveDataToken(ticks_t time, uint8_t value)
{
  buf.push_back(Token(value));
  update(time);
}

void Chanend::receiveDataTokens(ticks_t time, uint8_t *values, unsigned num)
{
  for (unsigned i = 0; i < num; i++) {
    buf.push_back(Token(values[i]));
  }
  update(time);
}

void Chanend::receiveCtrlToken(ticks_t time, uint8_t value)
{
  switch (value) {
  case CT_END:
    buf.push_back(Token(value, true));
    release(time);
    update(time);
    break;
  case CT_PAUSE:
    release(time);
    break;
  default:
    buf.push_back(Token(value, true));
    update(time);
    break;
  }
}

void Chanend::notifyDestClaimed(ticks_t time)
{
  if (pausedOut) {
    pausedOut->time = time;
    pausedOut->schedule();
    pausedOut = 0;
  }
}

// TODO can this be merged with the above function.
void Chanend::notifyDestCanAcceptTokens(ticks_t time, unsigned tokens)
{
  if (pausedOut) {
    pausedOut->time = time;
    pausedOut->schedule();
    pausedOut = 0;
  }
}

bool Chanend::openRoute()
{
  if (inPacket)
    return true;
  dest = getOwner().getParent().getChanendDest(destID);
  if (!dest) {
    // TODO if dest in unset should give a link error exception.
    junkPacket = true;
  } else if (!dest->claim(this, junkPacket)) {
    return false;
  }
  inPacket = true;
  return true;
}

bool Chanend::setData(Thread &thread, uint32_t value, ticks_t time)
{
  if (inPacket)
    return false;
  ResourceID id(value);
  if (id.type() != RES_TYPE_CHANEND &&
      id.type() != RES_TYPE_CONFIG)
    return false;
  destID = value;
  return true;
}

bool Chanend::getData(Thread &thread, uint32_t &result, ticks_t time)
{
  result = destID;
  return true;
}

Resource::ResOpResult Chanend::
outt(Thread &thread, uint8_t value, ticks_t time)
{
  if (!openRoute()) {
    pausedOut = &thread;
    return DESCHEDULE;
  }
  if (junkPacket)
    return CONTINUE;
  if (!dest->canAcceptToken()) {
    pausedOut = &thread;
    return DESCHEDULE;
  }
  dest->receiveDataToken(time, value);
  return CONTINUE;
}

Resource::ResOpResult Chanend::
out(Thread &thread, uint32_t value, ticks_t time)
{
  if (!openRoute()) {
    pausedOut = &thread;
    return DESCHEDULE;
  }
  if (junkPacket)
    return CONTINUE;
  if (!dest->canAcceptTokens(4)) {
    pausedOut = &thread;
    return DESCHEDULE;
  }
  // Channels are big endian
  uint8_t tokens[4] = {
    static_cast<uint8_t>(value >> 24),
    static_cast<uint8_t>(value >> 16),
    static_cast<uint8_t>(value >> 8),
    static_cast<uint8_t>(value)
  };
  dest->receiveDataTokens(time, tokens, 4);
  return CONTINUE;
}

Resource::ResOpResult Chanend::
outct(Thread &thread, uint8_t value, ticks_t time)
{
  auto *tracer = getTracer();

  if (!openRoute()) {
    pausedOut = &thread;
    return DESCHEDULE;
  }
  if (junkPacket) {
    if (value == CT_END || value == CT_PAUSE) {
      inPacket = false;
      junkPacket = false;
    }
    return CONTINUE;
  }
  if (!dest->canAcceptToken()) {
    pausedOut = &thread;
    return DESCHEDULE;
  }  
  dest->receiveCtrlToken(time, value);
  if (value == CT_END || value == CT_PAUSE) {
    inPacket = false;
    dest = 0;
  }
  return CONTINUE;
}

bool Chanend::
testct(Thread &thread, ticks_t time, bool &isCt)
{
  updateOwner(thread);
  if (buf.empty()) {
    setPausedIn(thread, false);
    return false;
  }
  isCt = buf.front().isControl();
  return true;
}

bool Chanend::
testwct(Thread &thread, ticks_t time, unsigned &position)
{
  updateOwner(thread);
  position = 0;
  unsigned numTokens = std::min(buf.size(), 4U);
  for (unsigned i = 0; i < numTokens; i++) {
    if (buf[i].isControl()) {
      position = i + 1;
      return true;
    }
  }
  if (buf.size() < 4) {
    setPausedIn(thread, true);
    return false;
  }
  return true;
}

uint8_t Chanend::poptoken(ticks_t time)
{
  assert(!buf.empty() && "poptoken on empty buf");
  uint8_t value = buf.front().getValue();
  buf.pop_front();
  if (getSource()) {
    getSource()->notifyDestCanAcceptTokens(time, buf.remaining());
  }
  return value;
}

void Chanend::setPausedIn(Thread &t, bool wordInput)
{
  pausedIn = &t;
  waitForWord = wordInput;
}

Resource::ResOpResult Chanend::
intoken(Thread &thread, ticks_t time, uint32_t &val)
{
  bool isCt;
  if (!testct(thread, time, isCt)) {
    return DESCHEDULE;
  }
  if (isCt)
    return ILLEGAL;
  val = poptoken(time);
  return CONTINUE;
}

LoggingTracer* Chanend::getTracer() { return (LoggingTracer*)getOwner().getParent().getTracer(); }

Resource::ResOpResult Chanend::
inct(Thread &thread, ticks_t time, uint32_t &val)
{
  bool isCt;
  if (!testct(thread, time, isCt)) {
    return DESCHEDULE;
  }
  if (!isCt)
    return ILLEGAL;

  auto *tracer = getTracer();

  val = poptoken(time);
  return CONTINUE;
}

Resource::ResOpResult Chanend::
chkct(Thread &thread, ticks_t time, uint32_t value)
{
  bool isCt;
  if (!testct(thread, time, isCt)) {
    return DESCHEDULE;
  }
  if (!isCt || buf.front().getValue() != value)
    return ILLEGAL;
  (void)poptoken(time);
  return CONTINUE;
}

Resource::ResOpResult Chanend::
in(Thread &thread, ticks_t time, uint32_t &value)
{
  unsigned Position;
  if (!testwct(thread, time, Position))
    return DESCHEDULE;
  if (Position != 0)
    return ILLEGAL;
  value = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
  buf.pop_front(4);
  if (getSource()) {
    getSource()->notifyDestCanAcceptTokens(time, buf.remaining());
  }
  return CONTINUE;
}

void Chanend::update(ticks_t time)
{
  assert(!buf.empty());
  if (eventsPermitted()) {
    event(time);
    return;
  }
  if (!pausedIn)
    return;
  if (waitForWord && buf.size() < 4)
    return;
  pausedIn->time = time;
  pausedIn->schedule();
  pausedIn = 0;
}

void Chanend::run(ticks_t time)
{
  assert(0 && "Shouldn't get here");
}

bool Chanend::seeEventEnable(ticks_t time)
{
  if (buf.empty())
    return false;
  event(time);
  return true;
}
