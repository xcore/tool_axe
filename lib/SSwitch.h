// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _SSwitch_h_
#define _SSwitch_h_

#include "Token.h"
#include "ChanEndpoint.h"
#include "SSwitchCtrlRegs.h"

namespace axe {

class Node;

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
  void initRegisters() { regs.initRegisters(); }
  void notifyDestClaimed(ticks_t time) override;

  void notifyDestCanAcceptTokens(ticks_t time, unsigned tokens) override;

  bool canAcceptToken() override;
  bool canAcceptTokens(unsigned tokens) override;

  void receiveDataToken(ticks_t time, uint8_t value) override;

  void receiveDataTokens(ticks_t time, uint8_t *values, unsigned num) override;

  void receiveCtrlToken(ticks_t time, uint8_t value) override;
};
  
} // End axe namespace

#endif //_SSwitch_h_

