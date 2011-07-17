// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Chanend.h"
#include "Core.h"

bool Chanend::claim(Chanend *newSource)
{
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

void Chanend::release(ticks_t time)
{
  if (queue.empty()) {
    source = 0;
    return;
  }
  source = queue.front();
  queue.pop();
  source->pausedOut->time = time;
  source->pausedOut->schedule();
  source->pausedOut = 0;
}

bool Chanend::setData(ThreadState &thread, uint32_t value, ticks_t time)
{
  updateOwner(thread);
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
  if (!dest || !dest->isInUse()) {
    // Junk data
    return CONTINUE;
  }
  if (!dest->claim(this) || dest->buf.full()) {
    pausedOut = &thread;
    return DESCHEDULE;
  }
  dest->buf.push_back(Token(value, time));
  dest->scheduleUpdate(time);
  return CONTINUE;
}

Resource::ResOpResult Chanend::
out(ThreadState &thread, uint32_t value, ticks_t time)
{
  updateOwner(thread);
  if (!dest || !dest->isInUse()) {
    // Junk data
    return CONTINUE;
  }
  if (!dest->claim(this) || dest->buf.remaining() < 4) {
    pausedOut = &thread;
    return DESCHEDULE;
  }
  // Channels are big endian
  dest->buf.push_back(Token(value >> 24, time));
  dest->buf.push_back(Token(value >> 16, time));
  dest->buf.push_back(Token(value >> 8, time));
  dest->buf.push_back(Token(value, time));
  dest->scheduleUpdate(time);
  return CONTINUE;
}

Resource::ResOpResult Chanend::
outct(ThreadState &thread, uint8_t value, ticks_t time)
{
  updateOwner(thread);
  if (!dest || !dest->isInUse()) {
    // Junk data
    return CONTINUE;
  }
  if (!dest->claim(this) || dest->buf.full()) {
    pausedOut = &thread;
    return DESCHEDULE;
  }
  switch (value) {
  case CT_END:
    {
      dest->buf.push_back(Token(CT_END, time, true));
      dest->scheduleUpdate(time);
      dest->release(time);
      break;
    }
  case CT_PAUSE:
    dest->release(time);
    break;
  default:
    dest->buf.push_back(Token(value, time, true));
    dest->scheduleUpdate(time);
    break;
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
  if (source && source->pausedOut) {
    source->pausedOut->time = time;
    source->pausedOut->schedule();
    source->pausedOut = 0;
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
  if (source && source->pausedOut) {
    source->pausedOut->time = time;
    source->pausedOut->schedule();
    source->pausedOut = 0;
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
