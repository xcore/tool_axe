// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "Timeout.h"
#include "SystemState.h"

using namespace axe;

void Timeout::run(ticks_t time)
{
  throw TimeoutException(time);
}
