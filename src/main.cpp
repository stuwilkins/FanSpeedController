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
#include <Adafruit_SleepyDog.h>

#include "wiring.h"
#include "debug.h"
#include "triac.h"
#include "inttimer.h"
#include "bluetooth.h"
#include "uart_cmd.h"
#include "indicator.h"
#include "file.h"
#include "config.h"

// Global variables

config_data config;

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

  config_set_defaults();

  // Start indicator

  indicator.begin();
  indicator.setStatus(NeoPixelIndicator::BOOT);

  // Flash drive must be setup before Serial Monitor
  file_setup();

  // Setup Serial Monitor

  Serial.begin(115200);
  unsigned long _start = millis();
  while (!Serial && ((millis() - _start < SERIAL_TIMEOUT))) {
    delay(10);
  }

  DEBUG_COMMENT("Started FanSpeedController.\n");

  // Setup watchdog

  int countdownMS = Watchdog.enable(WATCHDOG_TIMEOUT);
  DEBUG_PRINT("Enabled watchdog with max countdown of %d\n", countdownMS);

  // Setup TRIACs

  triac_setup();
  Watchdog.reset();

  // Setup Bluetooth
  DEBUG_COMMENT("Setting up bluetooth.\n");
  bluetooth_setup();
  bluetooth_set_rx_callback(bluetooth_rx_callback, NULL);
  Watchdog.reset();

  // Setup NeoPixel Indicators

  indicator.startupEffect();
  indicator.setStatus(NeoPixelIndicator::OK, 10);
  Watchdog.reset();

  DEBUG_COMMENT("Config:\n");
  config_print();

  DEBUG_COMMENT("Finished setup.\n");
}

void loop() {
  static unsigned long off_timer = 0;
  static unsigned long last_loop_millis = 0;
  static uint8_t op = 0;

  Watchdog.reset();  // Pet the dog!

  if (bluetooth_get_connections()) {
    // We have active connections
    indicator.setStatus(NeoPixelIndicator::BT_CONNECTED, 10);
  } else {
    indicator.setStatus(NeoPixelIndicator::OK, 0);
  }

  if ((millis() - last_loop_millis) > 3000) {
    // First check for new settings
    file_loop();

    float speed = bluetooth_calculate_speed();
    if (speed >= config.speed_max) {
      op = 255;
      off_timer = millis();  // Reset each cycle
    } else if ((speed >= config.speed_min) && (speed < config.speed_max)) {
      op = static_cast<uint8_t>(255 * (
          (speed - config.speed_min) / config.speed_max));
      off_timer = millis();  // Reset each cycle
    } else if (speed >= config.speed_threshold) {
      op = 1;
      off_timer = millis();  // Reset each cycle
    }

    // Check for off timer

    DEBUG_PRINT("off_timer = %ld\n", off_timer);
    if (((millis() - off_timer) > 30000L) && (speed < 1.5)) {
      DEBUG_PRINT("off_timer countdown = %ld\n", millis() - off_timer);
      op = 0;
    }

    // Set indicators

    indicator.setLevel(0, op);
    indicator.setLevel(1, op);

    // Set the fan output
    triac_set_output(op, op);

    DEBUG_PRINT("Mains Frequency          = %f\n", calc_mains_freq());
    DEBUG_PRINT("Hardtimer count          = %ld\n", hardtimer_count);
    DEBUG_PRINT("Zerocross pulse positive = %ld\n", zero_cross_pulse1);
    DEBUG_PRINT("Zerocross pulse negative = %ld\n", zero_cross_pulse2);
    DEBUG_PRINT("Connections              = %d\n", bluetooth_get_connections());
    DEBUG_PRINT("Speed                    = %f\n", speed);

    last_loop_millis = millis();
  }
}
