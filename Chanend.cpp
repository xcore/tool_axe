// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Chanend.h"
#include "Core.h"

bool Chanend::claim(Chanend *newSource, bool &junkPacket)
{
  if (!isInUse()) {
    junkPacket = true;
    return true;
  }
  // Check if the route is already open.
  if (source == newSource) {
    return true;
  }
  // Check if we are already in the middle of a packet.
  if (source) {
    queue.push(newSource);
    return false;
  }
  // Claim the channel
  source = newSource;
  return true;
}

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
  buf.push_back(Token(value, time));
  scheduleUpdate(time);
}

void Chanend::receiveDataTokens(ticks_t time, uint8_t *values, unsigned num)
{
  for (unsigned i = 0; i < num; i++) {
    buf.push_back(Token(values[i], time));
  }
  scheduleUpdate(time);
}

void Chanend::receiveCtrlToken(ticks_t time, uint8_t value)
{
  switch (value) {
  case CT_END:
    buf.push_back(Token(value, time, true));
    release(time);
    break;
  case CT_PAUSE:
    release(time);
    break;
  default:
    buf.push_back(Token(value, time, true));
    break;
  }
  scheduleUpdate(time);
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

void Chanend::release(ticks_t time)
{
  if (queue.empty()) {
    source = 0;
    return;
  }
  source = queue.front();
  queue.pop();
  source->notifyDestClaimed(time);
}

bool Chanend::openRoute()
{
  if (inPacket)
    return true;
  if (!dest) {
    // TODO if dest in unset should give a link error exception.
    junkPacket = true;
  } else if (!dest->claim(this, junkPacket)) {
    return false;
  }
  inPacket = true;
  return true;
}

bool Chanend::setData(ThreadState &thread, uint32_t value, ticks_t time)
{
  updateOwner(thread);
  if (inPacket)
    return false;
  ResourceID destID(value);
  if (destID.type() != RES_TYPE_CHANEND &&
      destID.type() != RES_TYPE_CONFIG)
    return false;
  Resource *res = thread.getParent().getChanendDest(destID);
  if (res) {
    assert(res->getType() == RES_TYPE_CHANEND);
    dest = static_cast<Chanend*>(res);
  } else {
    dest = 0;
  }
  return true;
}

Resource::ResOpResult Chanend::
outt(ThreadState &thread, uint8_t value, ticks_t time)
{
  updateOwner(thread);
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
out(ThreadState &thread, uint32_t value, ticks_t time)
{
  updateOwner(thread);
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
    value >> 24,
    value >> 16,
    value >> 8,
    value
  };
  dest->receiveDataTokens(time, tokens, 4);
  return CONTINUE;
}

Resource::ResOpResult Chanend::
outct(ThreadState &thread, uint8_t value, ticks_t time)
{
  updateOwner(thread);
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
  }
  return CONTINUE;
}

bool Chanend::
testct(ThreadState &thread, ticks_t time, bool &isCt)
{
  updateOwner(thread);
  if (buf.empty()) {
    setPausedIn(thread, false);
    return false;
  }
  ticks_t tokenTime = buf.front().getTime();
  if (tokenTime > time) {
    setPausedIn(thread, false);
    scheduleUpdate(tokenTime);
    return false;
  }
  isCt = buf.front().isControl();
  return true;
}

bool Chanend::
testwct(ThreadState &thread, ticks_t time, unsigned &position)
{
  updateOwner(thread);
  if (buf.size() < 4) {
    setPausedIn(thread, true);
    return false;
  }
  ticks_t tokenTime = buf[3].getTime();
  if (tokenTime > time) {
    setPausedIn(thread, true);
    scheduleUpdate(time);
    return false;
  }
  position = 0;
  for (unsigned i = 0; i < 4; i++) {
    if (buf[i].isControl()) {
      position = i + 1;
      break;
    }
  }
  return true;
}

uint8_t Chanend::poptoken(ticks_t time)
{
  assert(!buf.empty() && "poptoken on empty buf");
  uint8_t value = buf.front().getValue();
  buf.pop_front();
  if (source) {
    source->notifyDestCanAcceptTokens(time, buf.remaining());
  }
  return value;
}

void Chanend::setPausedIn(ThreadState &t, bool wordInput)
{
  pausedIn = &t;
  waitForWord = wordInput;
}

Resource::ResOpResult Chanend::
intoken(ThreadState &thread, ticks_t time, uint32_t &val)
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

Resource::ResOpResult Chanend::
inct(ThreadState &thread, ticks_t time, uint32_t &val)
{
  bool isCt;
  if (!testct(thread, time, isCt)) {
    return DESCHEDULE;
  }
  if (!isCt)
    return ILLEGAL;
  val = poptoken(time);
  return CONTINUE;
}

Resource::ResOpResult Chanend::
chkct(ThreadState &thread, ticks_t time, uint32_t value)
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
in(ThreadState &thread, ticks_t time, uint32_t &value)
{
  unsigned Position;
  if (!testwct(thread, time, Position))
    return DESCHEDULE;
  if (Position != 0)
    return ILLEGAL;
  value = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
  buf.pop_front(4);
  if (source) {
    source->notifyDestCanAcceptTokens(time, buf.remaining());
  }
  return CONTINUE;
}

void Chanend::update(ticks_t time)
{
  if (buf.empty() || buf.front().getTime() > time)
    return;
  if (eventsPermitted()) {
    event(time);
    return;
  }
  if (!pausedIn)
    return;
  if (waitForWord && (buf.size() < 4 || buf[3].getTime() > time))
    return;
  pausedIn->time = time;
  pausedIn->schedule();
  pausedIn = 0;
}

void Chanend::run(ticks_t time)
{
  update(time);
}

bool Chanend::seeEventEnable(ticks_t time)
{
  if (buf.empty())
    return false;
  assert(buf.front().getTime() <= time);
  event(time);
  return true;
}
