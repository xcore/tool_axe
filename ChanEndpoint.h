// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _ChanEndpoint_h_
#define _ChanEndpoint_h_

class ChanEndpoint {
public:
  /// Give notification that a route to the destination has been opened.
  virtual void notifyDestClaimed(ticks_t time) = 0;

  /// Give notification that the destination can accept  in the buffer has
  /// become available.
  virtual void notifyDestCanAcceptTokens(ticks_t time, unsigned tokens) = 0;

  /// Called when trying to open a route to this channel end. If the route
  /// cannot be opened immediately then the source chanend is added to a queue.
  /// The source will be notified when the route becomes available with
  /// notifyDestClaimed().
  /// \param junkPacket Set to false if the packet should be junked, otherwise
  ///                   left unchanged.
  /// \return Whether a route was succesfully opened.
  virtual bool claim(ChanEndpoint *Source, bool &junkPacket) = 0;

  virtual bool canAcceptToken() = 0;
  virtual bool canAcceptTokens(unsigned tokens) = 0;

  /// Recieve data token. The caller must check sufficient room is available
  /// using canAcceptToken().
  virtual void receiveDataToken(ticks_t time, uint8_t value) = 0;

  /// Recieve data tokens. The caller must check sufficient room is available
  /// using canAcceptTokens().
  virtual void receiveDataTokens(ticks_t time, uint8_t *values, unsigned num) = 0;

  /// Recieve control token. The caller must check sufficient room is available
  /// using canAcceptTokens().
  virtual void receiveCtrlToken(ticks_t time, uint8_t value) = 0;
};

#endif // _ChanEndpoint_h_
