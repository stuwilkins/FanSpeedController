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

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "inttimer.h"
#include "wiring.h"
#include "debug.h"
#include "indicator.h"
#include "colormap.h"

NeoPixelIndicator::NeoPixelIndicator(void) {
  neopixel = new Adafruit_NeoPixel(1, PIN_NEOPIXEL, NEO_GRB);
  strip = new Adafruit_NeoPixel(12, PIN_STRIP, NEO_RGB);
  timer = new TimerClass(3);

  timer->init(100000);
  timer->setCallback(NeoPixelIndicator::callback, this);

  ticktock = 0;
  neopixelFlash = 1;
}

NeoPixelIndicator::~NeoPixelIndicator(void) {
  delete neopixel;
  delete strip;
  delete timer;
}

void NeoPixelIndicator::begin(void) {
  timer->start();
  neopixel->begin();
  neopixel->show();
  strip->begin();
  strip->show();
}

void NeoPixelIndicator::timerTick(void) {
  if (!neopixelFlash) {
    // no flash
    if (neopixelCurrentStatus != neopixelStatus) {
      neopixelCurrentStatus = neopixelStatus;
      neopixel->setBrightness(20);
      neopixel->setPixelColor(0, neopixelStatus);
      neopixel->show();
    }
  } else {
    if (!(ticktock % neopixelFlash)) {
      if (neopixelCurrentStatus) {
        neopixelCurrentStatus = 0;
        neopixel->setBrightness(20);
        neopixel->setPixelColor(0, 0);
      } else {
        neopixelCurrentStatus = neopixelStatus;
        neopixel->setBrightness(20);
        neopixel->setPixelColor(0, neopixelStatus);
      }
      neopixel->show();
    }
  }
  ticktock++;
}

void NeoPixelIndicator::startupEffect(void) {
  for (int i = 0; i < 256; i++) {
    setLevel(0, i);
    setLevel(1, i);
    delay(2);
  }
  for (int i = 0; i < 256; i++) {
    setLevel(0, 255 - i);
    setLevel(1, 255 - i);
    delay(2);
  }
}

void NeoPixelIndicator::setStatus(int status, int flash) {
  neopixelStatus = status;
  neopixelFlash = flash;
}

void NeoPixelIndicator::setLevel(int display, uint8_t level) {
  // Level is from 0 to 255.

  int h = (level * 6 / 256 + 1);

  // strip->setBrightness(255);
  if (display == 0) {
    for (int i = 0; i < h; i++) {
      strip->setPixelColor(i, colormap[level]);
    }
    for (int i = h; i < 6; i++) {
      strip->setPixelColor(i, 0);
    }
  } else if (display == 1) {
    for (int i = 0; i < h; i++) {
      strip->setPixelColor(11 - i, colormap[level]);
    }
    for (int i = h; i < 6; i++) {
      strip->setPixelColor(11 - i, 0);
    }
  }
  strip->show();
}

NeoPixelIndicator indicator;
