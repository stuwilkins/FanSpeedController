//
// MIT License
//
// Copyright (c) 2020 Stuart Wilkins
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef SRC_INDICATOR_H_
#define SRC_INDICATOR_H_

#include <Adafruit_NeoPixel.h>
#include "inttimer.h"

class NeoPixelIndicator {
 public:
  enum {
    BOOT         = 0xFFFF00,
    OK           = 0x00FF00,
    OFF          = 0x000000,
    BT_CONNECTED = 0x0000FF,
  };
  NeoPixelIndicator(void);
  ~NeoPixelIndicator(void);
  void begin(void);
  void setStatus(int status, int flash = 1);
  void setLevel(int display, uint8_t level);
  void startupEffect(void);
  void timerTick(void);
  static void callback(void* ctx) {
    static_cast<NeoPixelIndicator*>(ctx)->timerTick();
  }

 private:
  TimerClass *timer;
  Adafruit_NeoPixel *neopixel;
  Adafruit_NeoPixel *strip;
  unsigned long ticktock;
  int neopixelStatus;
  int neopixelFlash;
  int neopixelCurrentStatus;
};

extern NeoPixelIndicator indicator;

#endif  // SRC_INDICATOR_H_
