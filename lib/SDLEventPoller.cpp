// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "SDLEventPoller.h"
#include "RunnableQueue.h"
#include "SystemState.h"
#include <SDL.h>
#include <SDL_events.h>

using namespace axe;

SDLEventPoller::SDLEventPoller(RunnableQueue &scheduler) :
scheduler(scheduler),
lastPollEvent(0)
{
  scheduler.push(*this, 0);
}

void SDLEventPoller::run(ticks_t time)
{
  // Don't poll for events more than 25 times a second.
  Uint32 hostTime = SDL_GetTicks();
  if (hostTime - lastPollEvent < 1000/25)
    return;
  lastPollEvent = time;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    for (auto listener : listeners) {
      listener(&event, time);
    }
    if (event.type == SDL_QUIT)
      throw ExitException(time, 0);
  }
  if (scheduler.empty())
    return;
  const ticks_t updateTicks = 20000;
  scheduler.push(*this, scheduler.front().wakeUpTime + updateTicks);
}
