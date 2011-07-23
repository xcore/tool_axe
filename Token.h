// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Token_h_
#define _Token_h_

enum ControlTokenValue {
  CT_END = 1,
  CT_PAUSE = 2,
};

class Token {
private:
  uint8_t value;
  bool control;
  
public:
  Token(uint8_t v = 0, bool c = false)
  : value(v), control(c) { }
  
  bool isControl() const
  {
    return control;
  }
  
  uint8_t getValue() const
  {
    return value;
  }
  
  operator uint8_t() const { return value; }
};

#endif // _Token_h_
