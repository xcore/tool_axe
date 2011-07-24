// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SSwitch_h_
#define _SSwitch_h_

#include "Token.h"
#include "ChanEndpoint.h"

class Node;

class SSwitchCtrlRegs {
private:
  uint32_t scratchReg;
public:
  SSwitchCtrlRegs();
  bool read(uint16_t num, uint32_t &result);
  bool write(uint16_t num, uint32_t value);
};

class SSwitch : public ChanEndpoint {
private:
  struct Request {
    bool write;
    uint16_t returnNode;
    uint8_t returnNum;
    uint32_t regNum;
    uint32_t data;
  };
  Node *parent;
  SSwitchCtrlRegs regs;
  /// Length of a write request (excluding CT_END).
  static const unsigned writeRequestLength = 1 + 3 + 2 + 4;
  /// Length of a read request (excluding CT_END).
  static const unsigned readRequestLength = 1 + 3 + 2;
  /// Number of tokens in the buffer.
  unsigned recievedTokens;
  Token buf[writeRequestLength];
  bool junkIncomingTokens;
  /// Are we in the middle of sending a response?
  bool sendingResponse;
  unsigned sentTokens;
  /// Length of response (including CT_END).
  unsigned responseLength;
  bool parseRequest(ticks_t time, Request &request) const;
  void handleRequest(ticks_t time, const Request &request);
public:
  SSwitch(Node *parent);
  virtual void notifyDestClaimed(ticks_t time);

  virtual void notifyDestCanAcceptTokens(ticks_t time, unsigned tokens);

  virtual bool canAcceptToken();
  virtual bool canAcceptTokens(unsigned tokens);

  virtual void receiveDataToken(ticks_t time, uint8_t value);

  virtual void receiveDataTokens(ticks_t time, uint8_t *values, unsigned num);

  virtual void receiveCtrlToken(ticks_t time, uint8_t value);
};

#endif _SSwitch_h_

