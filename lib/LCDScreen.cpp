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
#include "SDLEventPoller.h"
#include <SDL.h>
#include <SDL_video.h>
#include <SDL_pixels.h>
#include <iostream>
#include <cstdlib>

using namespace axe;

const bool showFPS = false;

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
  unsigned height;
  Uint32 Rmask;
  Uint32 Gmask;
  Uint32 Bmask;
  SDL_Window *screen;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  Uint16 *pixels;
  Uint32 lastFrameTime;
  unsigned frameCount;

public:
  SDLScreen(unsigned w, unsigned h, Uint32 r, Uint32 g, Uint32 b) :
    width(w), height(h), Rmask(r), Gmask(g), Bmask(b), screen(nullptr),
    renderer(nullptr), texture(nullptr), pixels(nullptr) {}
  ~SDLScreen();

  bool init();
  void writePixel(unsigned x, unsigned, uint16_t);
  unsigned getWidth() const { return width; }
  unsigned getHeigth() const { return height; }
  void flip();
};

bool SDLScreen::init() {
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    return false;

  screen = SDL_CreateWindow("AXE", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, width, height,
                            SDL_WINDOW_RESIZABLE);
  if (!screen)
    return false;
  renderer = SDL_CreateRenderer(screen, -1, 0);
  if (!renderer)
    return false;
  auto pixelFormat = SDL_MasksToPixelFormatEnum(16, Rmask, Gmask, Bmask, 0);
  if (pixelFormat == SDL_PIXELFORMAT_UNKNOWN)
    return false;
  texture =
    SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR565,
                      SDL_TEXTUREACCESS_STREAMING, width, height);
  if (!texture)
    return false;
  pixels = new Uint16[width * height];
  if (showFPS) {
    lastFrameTime = SDL_GetTicks();
    frameCount = 0;
  }
  return true;
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
  SDL_UpdateTexture(texture, NULL, pixels, width * sizeof(Uint16));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void SDLScreen::writePixel(unsigned x, unsigned y, Uint16 value)
{
  pixels[x + y * width] = value;
}

SDLScreen::~SDLScreen() {
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(screen);
  SDL_Quit();
  delete[] pixels;
}

const ticks_t minUpdateTicks = 20000;

class LCDScreen : public Peripheral, Runnable {
  SDLScreen screen;
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

  void seeSamplingEdge(ticks_t time);
  void seeCLKChange(const Signal &value, ticks_t time);
public:
  LCDScreen(SystemState &system, PortConnectionWrapper clk,
            PortConnectionWrapper data, PortConnectionWrapper de,
            PortConnectionWrapper hsync, PortConnectionWrapper vsync,
            unsigned width, unsigned height);

  void run(ticks_t time) override;
};


LCDScreen::
LCDScreen(SystemState &system, PortConnectionWrapper clk,
          PortConnectionWrapper data, PortConnectionWrapper de,
          PortConnectionWrapper hsync, PortConnectionWrapper vsync,
          unsigned width, unsigned height) :
  screen(width, height, 0x1f, (0x3f << 5), (0x1f << 11)),
  scheduler(system.getScheduler()),
  CLKSignal(0),
  CLKProxy(*this, &LCDScreen::seeCLKChange),
  x(0),
  y(0),
  edgeCounter(0),
  lastDEHighEdge(0)
{
  clk.attach(&CLKProxy);
  data.attach(&DataTracker);
  de.attach(&DETracker);
  hsync.attach(&HSYNCTracker);
  vsync.attach(&VSYNCTracker);
  system.initSDL();
  if (!screen.init()) {
    std::cerr << "Failed to initialize SDL screen\n";
    std::exit(1);
  }
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
    scheduler.push(*this, nextEdge->time);
  }
}

void LCDScreen::run(ticks_t time)
{
  assert(CLKSignal.isClock());
  seeSamplingEdge(time);
  scheduler.push(*this, time + CLKSignal.getPeriod());
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
    new LCDScreen(system, CLK, DATA, DE, HSYNC, VSYNC, 480, 272);
  return p;
}

std::unique_ptr<PeripheralDescriptor> axe::getPeripheralDescriptorLCDScreen()
{
  std::unique_ptr<PeripheralDescriptor> p(
    new PeripheralDescriptor("lcd-screen", &createLCDScreen));
  p->addProperty(PropertyDescriptor::portProperty("clk")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("data")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("de")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("hsync")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("vsync")).setRequired(true);
  return p;
}
