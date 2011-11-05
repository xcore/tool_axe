// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Chanend_h_
#define _Chanend_h_

#include "Resource.h"
#include "ChanEndpoint.h"
#include "ring_buffer.h"
#include "Token.h"

class Chanend : public EventableResource, public ChanEndpoint {
private:
  /// The destination channel end.
  ChanEndpoint *dest;
  /// Input buffer.
  typedef ring_buffer<Token, CHANEND_BUFFER_SIZE> TokenBuffer;
  TokenBuffer buf;
  /// Thread paused on an output instruction, 0 if none.
  ThreadState *pausedOut;
  /// Thread paused on an input instruction, 0 if none.
  ThreadState *pausedIn;
  /// Is the pausedIn thread waiting for a word? Only valid if pausedIn is set.
  bool waitForWord;
  /// Are we in the middle of sending a packet?
  bool inPacket;
  /// Should be current packet be junked?
  bool junkPacket;

  /// Update the channel end after the data is placed in the buffer.
  void update(ticks_t time);

  /// Try and open a route for a packet. If a route cannot be opened the chanend
  /// is registered with the destination and notifyDestClaimed() will be called
  /// when the route becomes available.
  bool openRoute();

  /// Called when trying to open a route to this channel end. If the route
  /// cannot be opened immediately then the source chanend is added to a queue.
  /// The source will be notified when the route becomes available with
  /// notifyDestClaimed().
  /// \return Whether a route was succesfully opened.
  bool claim(ChanEndpoint *Source, bool &junkPacket);

  bool canAcceptToken();
  bool canAcceptTokens(unsigned tokens);

  /// Recieve data token. The caller must check sufficient room is available
  /// using canAcceptToken().
  void receiveDataToken(ticks_t time, uint8_t value);

  /// Recieve data tokens. The caller must check sufficient room is available
  /// using canAcceptTokens().
  void receiveDataTokens(ticks_t time, uint8_t *values, unsigned num);

  /// Recieve control token. The caller must check sufficient room is available
  /// using canAcceptTokens().
  void receiveCtrlToken(ticks_t time, uint8_t value);

  /// Give notification that a route to the destination has been opened.
  void notifyDestClaimed(ticks_t time);

  /// Give notification that the destination can accept  in the buffer has
  /// become available.
  void notifyDestCanAcceptTokens(ticks_t time, unsigned tokens);

  /// Input a token. You must check beforehand if there is data available.
  uint8_t poptoken(ticks_t time);

  void setPausedIn(ThreadState &t, bool wordInput);

public:
  Chanend() : EventableResource(RES_TYPE_CHANEND) {}

  bool alloc(ThreadState &t)
  {
    assert(!isInUse() && "Trying to allocate in use chanend");
    dest = 0;
    pausedOut = 0;
    pausedIn = 0;
    inPacket = false;
    junkPacket = false;
    eventableSetInUseOn(t);
    setJunkIncoming(false);
    return true;
  }
  
  bool free()
  {
    if (!buf.empty() || getSource() || inPacket) {
      return false;
    }
    eventableSetInUseOff();
    setJunkIncoming(true);
    return true;
  }

  bool setData(ThreadState &thread, uint32_t value, ticks_t time);

  ResOpResult outt(ThreadState &thread, uint8_t value, ticks_t time);
  ResOpResult outct(ThreadState &thread, uint8_t value, ticks_t time);
  
  ResOpResult out(ThreadState &thread, uint32_t value, ticks_t time);
  ResOpResult intoken(ThreadState &thread, ticks_t time, uint32_t &val);
  ResOpResult inct(ThreadState &thread, ticks_t time, uint32_t &val);
  ResOpResult chkct(ThreadState &thread, ticks_t time, uint32_t value);

  /// Check if there is a token available for input. If a token is available
  /// the current thread's time is adjusted to be after the time at which the
  /// token was received and IsCt is set according whether it is a control token.
  bool testct(ThreadState &thread, ticks_t time, bool &isCt);
  /// Check if there is a word available for input. If a word is available
  /// the current thread's time is adjusted to be after the time at which the
  /// last token was received. Position is set to be the index of the first
  /// control token, starting at one, or 0 if there is no control token in the
  /// word.
  bool testwct(ThreadState &thread, ticks_t time, unsigned &position);

  ResOpResult in(ThreadState &thread, ticks_t time, uint32_t &val);

  void run(ticks_t time);
protected:
  bool seeEventEnable(ticks_t time);
};

#endif // _Chanend_h_
