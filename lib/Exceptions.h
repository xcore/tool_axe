// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _Exceptions_h_
#define _Exceptions_h_

namespace axe {

enum ExceptionType {
  ET_LINK_ERROR = 1,
  ET_ILLEGAL_PC = 2,
  ET_ILLEGAL_INSTRUCTION = 3,
  ET_ILLEGAL_RESOURCE = 4,
  ET_LOAD_STORE = 5,
  ET_ILLEGAL_PS = 6,
  ET_ARITHMETIC = 7,
  ET_ECALL = 8,
  ET_RESOURCE_DEP = 9,
  ET_KCALL = 15
};

class Exceptions {
public:
  static const char *getExceptionName(int type);
};
  
} // End axe namespace

#endif // _Exceptions_h_
