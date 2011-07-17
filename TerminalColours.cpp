// Copyright (c) 2011, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "TerminalColours.h"

TerminalColours TerminalColours::ansi = {
  "\033[0m", // reset
  "\033[30m", // black
  "\033[31m", // red
  "\033[32m", // green
  "\033[33m", // yellow
  "\033[34m", // blue
  "\033[35m", // magenta
  "\033[36m", // cyan
  "\033[37m" // white
};

TerminalColours TerminalColours::null = {
  "", // reset
  "", // black
  "", // red
  "", // green
  "", // yellow
  "", // blue
  "", // magenta
  "", // cyan
  "" // white
};
