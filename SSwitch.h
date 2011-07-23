// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Token.h"
#include "ChanEndpoint.h"

class SSwitch : public ChanEndpoint {
private:
public:
  SSwitch();
  virtual void notifyDestClaimed(ticks_t time);
  
  virtual void notifyDestCanAcceptTokens(ticks_t time, unsigned tokens);
  
  virtual bool canAcceptToken();
  virtual bool canAcceptTokens(unsigned tokens);
  
  virtual void receiveDataToken(ticks_t time, uint8_t value);
  
  virtual void receiveDataTokens(ticks_t time, uint8_t *values, unsigned num);
  
  virtual void receiveCtrlToken(ticks_t time, uint8_t value);
};
