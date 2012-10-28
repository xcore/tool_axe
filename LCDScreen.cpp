// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "LCDScreen.h"
#include "Peripheral.h"
#include "PortInterface.h"
#include "PortConnectionManager.h"
#include "PortHandleClockProxy.h"
#include "PortSignalTracker.h"
#include "PeripheralDescriptor.h"
#include "Signal.h"
#include "SystemState.h"
#include "SDL.h"
#include <iostream>
#include <cstdlib>

using namespace axe;

const bool showFPS = false;

static int filterSDLEvents(const SDL_Event *event)
{
  return event->type == SDL_QUIT;
}

class SDLScopedSurfaceLock {
  SDL_Surface *surface;
public:
  SDLScopedSurfaceLock(SDL_Surface *s) : surface(0) {
    if (SDL_MUSTLOCK(s)) {
      SDL_LockSurface(s);
      surface = s;
    }
  }
  ~SDLScopedSurfaceLock() {
    if (surface) {
      SDL_UnlockSurface(surface);
    }
  }
};

class SDLScreen {
  unsigned width;
  unsigned heigth;
  Uint32 Rmask;
  Uint32 Gmask;
  Uint32 Bmask;
  SDL_Surface *screen;
  SDL_Surface *buffer;
  Uint32 lastPollEvent;
  Uint32 lastFrameTime;
  unsigned frameCount;

public:
  SDLScreen(unsigned w, unsigned h, Uint32 r, Uint32 g, Uint32 b) :
    width(w), heigth(h), Rmask(r), Gmask(g), Bmask(b), screen(0), buffer(0),
    lastPollEvent(0) {}
  ~SDLScreen();

  bool init();
  bool pollForEvents();
  void writePixel(unsigned x, unsigned, uint16_t);
  unsigned getWidth() const { return width; }
  unsigned getHeigth() const { return heigth; }
  void flip();
};

bool SDLScreen::init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    return false;
  SDL_SetEventFilter(&filterSDLEvents);
  SDL_WM_SetCaption("AXE", "AXE");

  screen = SDL_SetVideoMode(width, heigth, 0, SDL_SWSURFACE | SDL_ANYFORMAT);
  if (!screen)
    return false;
  buffer =
    SDL_CreateRGBSurface(SDL_SWSURFACE, width, heigth, 16, Rmask, Gmask, Bmask,
                         0);
  if (showFPS) {
    lastFrameTime = SDL_GetTicks();
    frameCount = 0;
  }
  return true;
}

/// Handle pending SDL events. Returns whether a SDL_QUIT event was received.
bool SDLScreen::pollForEvents()
{
  // Don't poll for events more than 25 times a second.
  Uint32 time = SDL_GetTicks();
  if (time - lastPollEvent < 1000/25)
    return false;
  lastPollEvent = time;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT)
      return true;
  }
  return false;
}

void SDLScreen::flip()
{
  if (showFPS && ++frameCount == 10) {
    Uint32 newTime = SDL_GetTicks();
    double fps = 10000.0 / (newTime - lastFrameTime);
    std::cerr << fps << '\n';
    frameCount = 0;
    lastFrameTime = newTime;
  }
  SDL_BlitSurface(buffer, NULL, screen, NULL);
  SDL_Flip(screen);
}

void SDLScreen::writePixel(unsigned x, unsigned y, Uint16 value)
{
  SDLScopedSurfaceLock lock(buffer);
  Uint16 *pixels = reinterpret_cast<Uint16*>(buffer->pixels);
  pixels[x + y * width] = value;
}

SDLScreen::~SDLScreen() {
  SDL_Quit();
}

const ticks_t minUpdateTicks = 20000;

class LCDScreen : public Peripheral, Runnable {
  SDLScreen screen;
  enum ScheduleReason {
    SCHEDULE_SAMPLING_EDGE,
    SCHEDULE_POLL_FOR_EVENTS
  } scheduleReason;
  RunnableQueue &scheduler;
  Signal CLKSignal;
  uint32_t CLKValue;
  PortInterfaceMemberFuncDelegate<LCDScreen> CLKProxy;
  PortSignalTracker DataTracker;
  PortSignalTracker DETracker;
  PortSignalTracker HSYNCTracker;
  PortSignalTracker VSYNCTracker;
  unsigned x;
  unsigned y;
  unsigned edgeCounter;
  ticks_t lastDEHighEdge;
  ticks_t lastPollForEvents;

  void seeSamplingEdge(ticks_t time);
  void seeCLKChange(const Signal &value, ticks_t time);
  void schedule(ScheduleReason reason, ticks_t time) {
    scheduleReason = reason;
    scheduler.push(*this, time);
  }
  void scheduleSamplingEdge(ticks_t time) {
    schedule(SCHEDULE_SAMPLING_EDGE, time);
  }
  void schedulePollForEvents(ticks_t time) {
    schedule(SCHEDULE_POLL_FOR_EVENTS, time);
  }
public:
  LCDScreen(RunnableQueue &s, PortConnectionWrapper clk,
            PortConnectionWrapper data, PortConnectionWrapper de,
            PortConnectionWrapper hsync, PortConnectionWrapper vsync,
            unsigned width, unsigned height);

  void run(ticks_t time);
};


LCDScreen::
LCDScreen(RunnableQueue &s, PortConnectionWrapper clk,
          PortConnectionWrapper data, PortConnectionWrapper de,
          PortConnectionWrapper hsync, PortConnectionWrapper vsync,
          unsigned width, unsigned height) :
  screen(width, height, 0x1f, (0x3f << 5), (0x1f << 11)),
  scheduler(s),
  CLKSignal(0),
  CLKProxy(*this, &LCDScreen::seeCLKChange),
  x(0),
  y(0),
  edgeCounter(0),
  lastDEHighEdge(0),
  lastPollForEvents(0)
{
  clk.attach(&CLKProxy);
  data.attach(&DataTracker);
  de.attach(&DETracker);
  hsync.attach(&HSYNCTracker);
  vsync.attach(&VSYNCTracker);
  if (!screen.init()) {
    std::cerr << "Failed to initialize SDL screen\n";
    std::exit(1);
  }
  schedulePollForEvents(minUpdateTicks);
}

#include <iostream>

void LCDScreen::seeSamplingEdge(ticks_t time)
{
  ++edgeCounter;
  // TODO According to the AT043TN24 datasheet data should be sampled on the
  // falling edge. However the sc_lcd code drives on the falling edge so for
  // the moment sample on the rising to match that.
  if (DETracker.getSignal().getValue(time)) {
    // Thb
    const unsigned minHorizontalClks = 40;
    // Th * tvb
    const unsigned minVerticalClks = 5 * 520;
    ticks_t lowEdges = edgeCounter - (lastDEHighEdge + 1);
    if (lowEdges >= minVerticalClks) {
      // VSync
      x = y = 0;
      screen.flip();
    } else if (lowEdges >= minHorizontalClks) {
      // HSync
      x = 0;
      ++y;
    }
    if (x < screen.getWidth() && y < screen.getHeigth()) {
      screen.writePixel(x++, y, DataTracker.getSignal().getValue(time));
    }
    lastDEHighEdge = edgeCounter;
  }
}

void LCDScreen::seeCLKChange(const Signal &newSignal, ticks_t time)
{
  CLKSignal = newSignal;
  uint32_t newValue = CLKSignal.getValue(time);
  if (newValue != CLKValue) {
    CLKValue = newValue;
    if (CLKValue)
      seeSamplingEdge(time);
  }
  if (CLKSignal.isClock()) {
    EdgeIterator nextEdge = CLKSignal.getEdgeIterator(time);
    if (nextEdge->type != Edge::RISING)
      ++nextEdge;
    scheduleSamplingEdge(nextEdge->time);
  } else if (scheduleReason != SCHEDULE_POLL_FOR_EVENTS) {
    schedulePollForEvents(time + minUpdateTicks);
  }
}

void LCDScreen::run(ticks_t time)
{
  switch (scheduleReason) {
  case SCHEDULE_SAMPLING_EDGE:
    assert(CLKSignal.isClock());
    seeSamplingEdge(time);
    if (time - lastPollForEvents >= minUpdateTicks) {
      if (screen.pollForEvents()) {
        throw ExitException(0);
      }
      lastPollForEvents = time;
    }
    scheduleSamplingEdge(time + CLKSignal.getPeriod());
    break;
  case SCHEDULE_POLL_FOR_EVENTS:
    if (screen.pollForEvents()) {
      throw ExitException(0);
    }
    schedulePollForEvents(time + minUpdateTicks);
    lastPollForEvents = time;
    break;
  }
}

static Peripheral *
createLCDScreen(SystemState &system, PortConnectionManager &connectionManager,
                  const Properties &properties)
{
  PortConnectionWrapper CLK = connectionManager.get(properties, "clk");
  PortConnectionWrapper DATA = connectionManager.get(properties, "data");
  PortConnectionWrapper DE = connectionManager.get(properties, "de");
  PortConnectionWrapper HSYNC = connectionManager.get(properties, "vsync");
  PortConnectionWrapper VSYNC = connectionManager.get(properties, "hsync");
  LCDScreen *p =
    new LCDScreen(system.getScheduler(), CLK, DATA, DE, HSYNC, VSYNC, 480, 272);
  return p;
}

std::auto_ptr<PeripheralDescriptor> axe::getPeripheralDescriptorLCDScreen()
{
  std::auto_ptr<PeripheralDescriptor> p(
    new PeripheralDescriptor("lcd-screen", &createLCDScreen));
  p->addProperty(PropertyDescriptor::portProperty("clk")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("data")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("de")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("hsync")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("vsync")).setRequired(true);
  return p;
}
