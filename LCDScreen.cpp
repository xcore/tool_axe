// Copyright (c) 2012, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "LCDScreen.h"
#include "Peripheral.h"
#include "PortInterface.h"
#include "PortConnectionManager.h"
#include "PortHandleClockProxy.h"
#include "PeripheralDescriptor.h"
#include "Signal.h"
#include "SystemState.h"
#include "SDL.h"
#include <iostream>
#include <cstdlib>

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

public:
  SDLScreen(unsigned w, unsigned h, Uint32 r, Uint32 g, Uint32 b) :
    width(w), heigth(h), Rmask(r), Gmask(g), Bmask(b), screen(0), buffer(0) {}
  ~SDLScreen();

  bool init();
  bool update();
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
  
  screen = SDL_SetVideoMode(width, heigth, 32, SDL_SWSURFACE | SDL_ANYFORMAT);
  if (!screen)
    return false;
  buffer =
    SDL_CreateRGBSurface(SDL_SWSURFACE, width, heigth, 16, Rmask, Gmask, Bmask,
                         0);
  return true;
}

/// Handle pending SDL events. Returns whether a SDL_QUIT event was received.
bool SDLScreen::update()
{
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT)
      return true;
  }
  return false;
}

void SDLScreen::flip()
{
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

class LCDScreen : public Peripheral {
  SDLScreen screen;
  PortInterfaceMemberFuncDelegate<LCDScreen> CLKProxy;
  PortInterfaceMemberFuncDelegate<LCDScreen> DataProxy;
  PortInterfaceMemberFuncDelegate<LCDScreen> DEProxy;
  PortInterfaceMemberFuncDelegate<LCDScreen> HSYNCProxy;
  PortInterfaceMemberFuncDelegate<LCDScreen> VSYNCProxy;
  PortHandleClockProxy CLKHandleClock;
  Signal DataSignal;
  Signal DESignal;
  Signal VSYNCSignal;
  Signal HSYNCSignal;
  unsigned x;
  unsigned y;
  unsigned edgeCounter;
  ticks_t lastDEHighEdge;

  void seeCLKChange(const Signal &value, ticks_t time);
  void seeDataChange(const Signal &value, ticks_t time);
  void seeDEChange(const Signal &value, ticks_t time);
  void seeHSYNCChange(const Signal &value, ticks_t time);
  void seeVSYNCChange(const Signal &value, ticks_t time);
public:
  LCDScreen(RunnableQueue &s, PortConnectionWrapper clk,
            PortConnectionWrapper data, PortConnectionWrapper de,
            PortConnectionWrapper hsync, PortConnectionWrapper vsync,
            unsigned width, unsigned height) :
    screen(width, height, 0x1f, (0x3f << 5), (0x1f << 11)),
    CLKProxy(*this, &LCDScreen::seeCLKChange),
    DataProxy(*this, &LCDScreen::seeDataChange),
    DEProxy(*this, &LCDScreen::seeDEChange),
    HSYNCProxy(*this, &LCDScreen::seeHSYNCChange),
    VSYNCProxy(*this, &LCDScreen::seeVSYNCChange),
    CLKHandleClock(s, CLKProxy),
    DataSignal(0),
    DESignal(0),
    x(0),
    y(0),
    edgeCounter(0),
    lastDEHighEdge(0)
  {
    clk.attach(&CLKHandleClock);
    data.attach(&DataProxy);
    de.attach(&DEProxy);
    hsync.attach(&HSYNCProxy);
    vsync.attach(&VSYNCProxy);
    if (!screen.init()) {
      std::cerr << "Failed to initialize SDL screen\n";
      std::exit(1);
    }
  }
};

void LCDScreen::seeCLKChange(const Signal &value, ticks_t time)
{
  assert(!value.isClock());
  if (screen.update()) {
    throw ExitException(0);
  }
  if (value.getValue(time)) {
    // Rising edge.
    ++edgeCounter;
    // TODO According to the AT043TN24 datasheet data should be sampled on the
    // falling edge. However the sc_lcd code drives on the falling edge so for
    // the moment sample on the rising to match that.
    if (DESignal.getValue(time)) {
      // thf + thp + thb
      const unsigned minHorizontalClks = 45;
      // tvf + tvp + tvb
      const unsigned minVerticalMultiplier = 6;
      ticks_t lowEdges = edgeCounter - (lastDEHighEdge + 1);
      if (lowEdges >= minVerticalMultiplier * minHorizontalClks) {
        // VSync
        x = y = 0;
        screen.flip();
      } else if (lowEdges >= minHorizontalClks) {
        // HSync
        x = 0;
        ++y;
      }
      if (x < screen.getWidth() && y < screen.getHeigth()) {
        screen.writePixel(x++, y, DataSignal.getValue(time));
      }
      lastDEHighEdge = edgeCounter;
    }
  }
}

void LCDScreen::seeDataChange(const Signal &value, ticks_t time)
{
  DataSignal = value;
}

void LCDScreen::seeDEChange(const Signal &value, ticks_t time)
{
  DESignal = value;
}

void LCDScreen::seeHSYNCChange(const Signal &value, ticks_t time)
{
  HSYNCSignal = value;
}

void LCDScreen::seeVSYNCChange(const Signal &value, ticks_t time)
{
  VSYNCSignal = value;
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

std::auto_ptr<PeripheralDescriptor> getPeripheralDescriptorLCDScreen()
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
