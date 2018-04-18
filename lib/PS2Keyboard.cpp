// Copyright (c) 2013, Richard Osborne, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#include "PS2Keyboard.h"
#include "Peripheral.h"
#include "PeripheralDescriptor.h"
#include "PortConnectionManager.h"
#include "PortInterface.h"
#include "Runnable.h"
#include "RunnableQueue.h"
#include "SystemState.h"
#include "Signal.h"
#include "SDLEventPoller.h"
#include "BitManip.h"
#include <deque>

using namespace axe;

class PS2Device : Runnable {
private:
  RunnableQueue &scheduler;
  bool initialized;
  PortInterface *CLK;
  PortInterface *DATA;
  uint16_t data;
  unsigned bitsRemaining;
  Signal clkSignal;
  void startTransmittingByte(uint8_t byte, ticks_t time);
  void tryToTransmitNextByte(ticks_t time);
protected:
  virtual bool getMoreData(uint8_t &) = 0;
  void notifyDataAvailable(ticks_t time);
public:
  PS2Device(RunnableQueue &s, PortInterface *CLK, PortInterface *DATA);

  void run(ticks_t time) override;
};

PS2Device::PS2Device(RunnableQueue &s, PortInterface *CLK,
                     PortInterface *DATA) :
  scheduler(s),
  initialized(false),
  CLK(CLK),
  DATA(DATA),
  bitsRemaining(0)
{
  // Clock should be between 10KHz and 16.7KHz.
  const unsigned CLK_Hz = 15000;
  clkSignal = Signal(0, (CYCLES_PER_TICK * 100000000) / (CLK_Hz * 2));
  scheduler.push(*this, 0);
}

void PS2Device::startTransmittingByte(uint8_t byte, ticks_t time)
{
  data = 0;
  data |= 0; // Start bit.
  data |= byte << 1; // Data.
  data |= parity(byte) << 9; // Parity.
  data |= 1 << 10; // Stop bit.
  bitsRemaining = 11;
  ticks_t transitionTime = clkSignal.getNextEdge(time, Edge::RISING).time;
  transitionTime += clkSignal.getHalfPeriod() / 2;
  scheduler.push(*this, transitionTime);
}

void PS2Device::notifyDataAvailable(ticks_t time)
{
  if (!initialized || bitsRemaining)
    return;
  tryToTransmitNextByte(time);
}

void PS2Device::run(ticks_t time)
{
  if (!initialized) {
    initialized = true;
    CLK->seePinsChange(clkSignal, time);
    DATA->seePinsChange(Signal(1), time);
    tryToTransmitNextByte(time);
  } else {
    assert(bitsRemaining != 0);
    DATA->seePinsChange(Signal(data & 1), time);
    if (--bitsRemaining) {
      data >>= 1;
      scheduler.push(*this, time + clkSignal.getPeriod());
    } else {
      tryToTransmitNextByte(time);
    }
  }
}

void PS2Device::tryToTransmitNextByte(ticks_t time)
{
  uint8_t byte;
  if (!getMoreData(byte))
    return;
  startTransmittingByte(byte, time);
}

class PS2Keyboard : public Peripheral, PS2Device {
  std::deque<uint8_t> pendingBytes;
  void eventCallback(SDL_Event *event, ticks_t time);
protected:
  bool getMoreData(uint8_t &) override;
public:
  PS2Keyboard(SystemState &system, PortInterface *CLK,
              PortInterface *DATA);
};

PS2Keyboard::PS2Keyboard(SystemState &system, PortInterface *CLK,
                         PortInterface *DATA) :
  PS2Device(system.getScheduler(), CLK, DATA)
{
  SDLEventPoller *poller = system.initSDL();
  poller->addListener([=](SDL_Event *event, ticks_t time) {
    eventCallback(event, time);
  });
}

static std::map<SDL_Scancode, const char *> scanCodeMap = {
    {SDL_SCANCODE_A, "\x1c"},
    {SDL_SCANCODE_B, "\x32"},
    {SDL_SCANCODE_C, "\x21"},
    {SDL_SCANCODE_D, "\x23"},
    {SDL_SCANCODE_E, "\x24"},
    {SDL_SCANCODE_F, "\x2b"},
    {SDL_SCANCODE_G, "\x34"},
    {SDL_SCANCODE_H, "\x33"},
    {SDL_SCANCODE_I, "\x43"},
    {SDL_SCANCODE_J, "\x3b"},
    {SDL_SCANCODE_K, "\x42"},
    {SDL_SCANCODE_L, "\x4b"},
    {SDL_SCANCODE_M, "\x3a"},
    {SDL_SCANCODE_N, "\x31"},
    {SDL_SCANCODE_O, "\x44"},
    {SDL_SCANCODE_P, "\x4d"},
    {SDL_SCANCODE_Q, "\x15"},
    {SDL_SCANCODE_R, "\x2d"},
    {SDL_SCANCODE_S, "\x1b"},
    {SDL_SCANCODE_T, "\x2c"},
    {SDL_SCANCODE_U, "\x3c"},
    {SDL_SCANCODE_V, "\x2a"},
    {SDL_SCANCODE_W, "\x1d"},
    {SDL_SCANCODE_X, "\x22"},
    {SDL_SCANCODE_Y, "\x35"},
    {SDL_SCANCODE_Z, "\x1a"},
    {SDL_SCANCODE_0, "\x45"},
    {SDL_SCANCODE_1, "\x16"},
    {SDL_SCANCODE_2, "\x1e"},
    {SDL_SCANCODE_3, "\x26"},
    {SDL_SCANCODE_4, "\x25"},
    {SDL_SCANCODE_5, "\x2e"},
    {SDL_SCANCODE_6, "\x36"},
    {SDL_SCANCODE_7, "\x3d"},
    {SDL_SCANCODE_8, "\x3e"},
    {SDL_SCANCODE_9, "\x46"},
    {SDL_SCANCODE_GRAVE, "\x0e"},
    {SDL_SCANCODE_MINUS, "\x4e"},
    {SDL_SCANCODE_EQUALS, "\x55"},
    {SDL_SCANCODE_BACKSLASH, "\x5d"},
    {SDL_SCANCODE_SPACE, "\x66"},
    {SDL_SCANCODE_TAB, "\x0d"},
    {SDL_SCANCODE_CAPSLOCK, "\x58"},
    {SDL_SCANCODE_LSHIFT, "\x12"},
    {SDL_SCANCODE_LCTRL, "\x14"},
    {SDL_SCANCODE_LGUI, "\xe0\x1f"},
    {SDL_SCANCODE_LALT, "\x11"},
    {SDL_SCANCODE_RSHIFT, "\x59"},
    {SDL_SCANCODE_RCTRL, "\xe0\x14"},
    {SDL_SCANCODE_RGUI, "\xe0\x27"},
    {SDL_SCANCODE_RALT, "\xe0\x11"},
    {SDL_SCANCODE_APPLICATION, "\xe0\x2f"},
    {SDL_SCANCODE_RETURN, "\x5a"},
    {SDL_SCANCODE_ESCAPE, "\x76"},
    {SDL_SCANCODE_F1, "\x05"},
    {SDL_SCANCODE_F2, "\x06"},
    {SDL_SCANCODE_F3, "\x04"},
    {SDL_SCANCODE_F4, "\x0c"},
    {SDL_SCANCODE_F5, "\x03"},
    {SDL_SCANCODE_F6, "\x0b"},
    {SDL_SCANCODE_F7, "\x83"},
    {SDL_SCANCODE_F8, "\x0a"},
    {SDL_SCANCODE_F9, "\x01"},
    {SDL_SCANCODE_F10, "\x09"},
    {SDL_SCANCODE_F11, "\x78"},
    {SDL_SCANCODE_F12, "\x07"},
    {SDL_SCANCODE_PRINTSCREEN, "\x0e\x12\xE0\x7C"},
    {SDL_SCANCODE_SCROLLLOCK, "\x7e"},
    {SDL_SCANCODE_PAUSE, "\x0e\x14\x77\xE1\xF0\x14\xF0\x77"},
    {SDL_SCANCODE_LEFTBRACKET, "\x54"},
    {SDL_SCANCODE_INSERT, "\xe0\x70"},
    {SDL_SCANCODE_HOME, "\xe0\x6c"},
    {SDL_SCANCODE_PAGEUP, "\xe0\x7d"},
    {SDL_SCANCODE_DELETE, "\xe0\x71"},
    {SDL_SCANCODE_END, "\xe0\x69"},
    {SDL_SCANCODE_PAGEDOWN, "\xe0\x7a"},
    {SDL_SCANCODE_UP, "\xe0\x75"},
    {SDL_SCANCODE_LEFT, "\xe0\x6b"},
    {SDL_SCANCODE_DOWN, "\xe0\x72"},
    {SDL_SCANCODE_RIGHT, "\xe0\x74"},
    {SDL_SCANCODE_NUMLOCKCLEAR, "\x77"},
    {SDL_SCANCODE_KP_DIVIDE, "\xe0\x4a"},
    {SDL_SCANCODE_KP_MULTIPLY, "\x7c"},
    {SDL_SCANCODE_KP_MINUS, "\x7b"},
    {SDL_SCANCODE_KP_PLUS, "\x79"},
    {SDL_SCANCODE_KP_ENTER, "\xe0\x5a"},
    {SDL_SCANCODE_KP_PERIOD, "\x71"},
    {SDL_SCANCODE_KP_0, "\x70"},
    {SDL_SCANCODE_KP_1, "\x69"},
    {SDL_SCANCODE_KP_2, "\x72"},
    {SDL_SCANCODE_KP_3, "\x7a"},
    {SDL_SCANCODE_KP_4, "\x6b"},
    {SDL_SCANCODE_KP_5, "\x73"},
    {SDL_SCANCODE_KP_6, "\x74"},
    {SDL_SCANCODE_KP_7, "\x6c"},
    {SDL_SCANCODE_KP_8, "\x75"},
    {SDL_SCANCODE_KP_9, "\x7d"},
    {SDL_SCANCODE_RIGHTBRACKET, "\x5b"},
    {SDL_SCANCODE_SEMICOLON, "\x4c"},
    {SDL_SCANCODE_APOSTROPHE, "\x52"},
    {SDL_SCANCODE_COMMA, "\x41"},
    {SDL_SCANCODE_PERIOD, "\x49"},
    {SDL_SCANCODE_SLASH, "\x4a"}}

void PS2Keyboard::eventCallback(SDL_Event *event, ticks_t time)
{
  if (event->type != SDL_KEYDOWN && event->type != SDL_KEYUP)
    return;
  SDL_Scancode scanCode = event->key.keysym.scancode;
  auto match = scanCodeMap.find(scanCode);
  if (match == scanCodeMap.end())
    return;
  const char *bytes = match->second;
  if (event->type == SDL_KEYUP) {
    if (scanCode == SDL_SCANCODE_PAUSE) {
      // No break code.
      return;
    } else if (scanCode == SDL_SCANCODE_PRINTSCREEN) {
      bytes = "\xe0\xf0\x7c\xe0\xf0\x12";
    } else if (*bytes == 0xe) {
      pendingBytes.push_back(*bytes++);
      pendingBytes.push_back(0xf0);
    } else {
      pendingBytes.push_back(0xf0);
    }
  }
  while (*bytes) {
    pendingBytes.push_back(*bytes++);
  }
  notifyDataAvailable(time);
}

bool PS2Keyboard::getMoreData(uint8_t &data)
{
  if (pendingBytes.empty())
    return false;
  data = pendingBytes.front();
  pendingBytes.pop_front();
  return true;
}

static Peripheral *
createPS2Keyboard(SystemState &system, PortConnectionManager &connectionManager,
                const Properties &properties)
{
  PortConnectionWrapper CLK = connectionManager.get(properties, "clk");
  PortConnectionWrapper DATA = connectionManager.get(properties, "data");
  PS2Keyboard *p =
    new PS2Keyboard(system, CLK.getInterface(), DATA.getInterface());
  return p;
}

std::unique_ptr<PeripheralDescriptor> axe::getPeripheralDescriptorPS2Keyboard()
{
  std::unique_ptr<PeripheralDescriptor> p(
    new PeripheralDescriptor("ps2-keyboard", &createPS2Keyboard));
  p->addProperty(PropertyDescriptor::portProperty("clk")).setRequired(true);
  p->addProperty(PropertyDescriptor::portProperty("data")).setRequired(true);
  return p;
}
