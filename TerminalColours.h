// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef _TerminalColours_h_
#define _TerminalColours_h_

struct TerminalColours {
  const char *reset;
  const char *black;
  const char *red;
  const char *green;
  const char *yellow;
  const char *blue;
  const char *magenta;
  const char *cyan;
  const char *white;

  static TerminalColours ansi;
  static TerminalColours null;
};

#endif //_TerminalColours_h_
