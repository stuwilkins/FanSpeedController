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
#include <SdFat.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SleepyDog.h>
#include <Adafruit_SPIFlash.h>

#include "wiring.h"
#include "debug.h"
#include "triac.h"
#include "inttimer.h"
#include "bluetooth.h"
#include "uart_cmd.h"
#include "indicator.h"

#define SERIAL_TIMEOUT        10000

Adafruit_FlashTransport_QSPI flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;

void bluetooth_rx_callback(const char* cmd, int cmd_len, void* ctx) {
  DEBUG_PRINT("Recieved data [%s]\n", cmd);
}

void setup() {
  // Setup Input / Output

  pinMode(LED_BUILTIN,      OUTPUT);
  pinMode(PIN_MAINS_CLOCK,  INPUT_PULLUP);
  pinMode(PIN_FAN_1,        OUTPUT);
  pinMode(PIN_FAN_2,        OUTPUT);
  digitalWrite(PIN_FAN_1,   LOW);
  digitalWrite(PIN_FAN_2,   LOW);

  // Start indicator

  indicator.begin();
  indicator.setStatus(NeoPixelIndicator::BOOT);

  // Setup Serial Monitor

  Serial.begin(115200);
  unsigned long _start = millis();
  while (!Serial && ((millis() - _start < SERIAL_TIMEOUT))) {
    delay(10);
  }

  DEBUG_COMMENT("Started FanSpeedController.\n");

  int countdownMS = Watchdog.enable(8000);
  DEBUG_PRINT("Enabled watchdog with max countdown of %d\n", countdownMS);

  triac_setup();
  Watchdog.reset();

  // Setup Bluetooth
  DEBUG_COMMENT("Setting up bluetooth.\n");
  bluetooth_setup();
  bluetooth_set_rx_callback(bluetooth_rx_callback, NULL);
  Watchdog.reset();

  attachInterrupt(digitalPinToInterrupt(PIN_MAINS_CLOCK),
    zero_crossing_isr, CHANGE);

  DEBUG_COMMENT("Setting up flash\n");
  flash.begin();
  fatfs.begin(&flash);
  DEBUG_PRINT("JEDEC ID: 0x%X\n", flash.getJEDECID());
  DEBUG_PRINT("Flash size: %d\n", flash.size());

  indicator.startupEffect();
  indicator.setStatus(NeoPixelIndicator::OK, 10);
  Watchdog.reset();
  DEBUG_COMMENT("Finished setup.\n");

  File myFile = fatfs.open("test.txt", FILE_WRITE);
  if (myFile) {
    DEBUG_COMMENT("Writing to test.txt...\n");
    myFile.println("testing 1, 2, 3.");
    myFile.close();
    DEBUG_COMMENT("DONE");
  } else {
    DEBUG_COMMENT("Error opening test.txt\n");
  }
}

unsigned long last_loop_millis = 0;

void loop() {
  Watchdog.reset();  // Pet the dog!

  if (bluetooth_get_connections()) {
    // We have active connections
    indicator.setStatus(NeoPixelIndicator::BT_CONNECTED, 10);
  } else {
    indicator.setStatus(NeoPixelIndicator::OK, 0);
  }


  if ((millis() - last_loop_millis) > 3000) {
    float speed = bluetooth_calculate_speed();
    uint8_t op;
    if (speed < 30.0) {
      op = static_cast<uint8_t>(255 * (speed / 30.0));
    } else {
      op = 255;
    }

    indicator.setLevel(0, op);
    indicator.setLevel(1, op);

    if (speed > 5) {
      triac_set_output(op, op);
    } else {
      triac_set_output(0, 0);
    }

    DEBUG_PRINT("Mains Frequency          = %f\n", calc_mains_freq());
    DEBUG_PRINT("Hardtimer count          = %ld\n", hardtimer_count);
    DEBUG_PRINT("Zerocross pulse positive = %ld\n", zero_cross_pulse1);
    DEBUG_PRINT("Zerocross pulse negative = %ld\n", zero_cross_pulse2);
    DEBUG_PRINT("Connections              = %d\n", bluetooth_get_connections());
    DEBUG_PRINT("Speed                    = %f\n", speed);

    last_loop_millis = millis();
  }
}
