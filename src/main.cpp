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
#include "wiring.h"
#include "debug.h"
#include "triac.h"
#include "inttimer.h"
#include "bluetooth.h"
#include "uart_cmd.h"
#include "indicator.h"

//
// User Interface
//

Adafruit_NeoPixel indicator(1, PIN_NEOPIXEL, NEO_GRB);
#define INDICATOR_BOOT        indicator.Color(255, 255, 0)
#define INDICATOR_OK          indicator.Color(0, 255, 0)
#define INDICATOR_CONNECTED   indicator.Color(0, 0, 255)

//
// ISR / Timer
//

void bluetooth_rx_callback(const char* cmd, int cmd_len, void* ctx) {
  DEBUG_PRINT("Recieved data [%s]\n", cmd);
  if (!strncmp(CMD_FAN1_ON, cmd, cmd_len)) {
    fan1_delay = 1;
  } else if (!strncmp(CMD_FAN1_OFF, cmd, cmd_len)) {
    fan1_delay = 0;
  } else if (!strncmp(CMD_FAN2_ON, cmd, cmd_len)) {
    fan2_delay = 1;
  } else if (!strncmp(CMD_FAN2_OFF, cmd, cmd_len)) {
    fan2_delay = 0;
  } else {
    DEBUG_COMMENT("Unknown command\n");
  }
}

void setup() {
  // Setup Input / Output

  pinMode(LED_BUILTIN,      OUTPUT);
  pinMode(PIN_NEOPIXEL,     INPUT_PULLUP);
  pinMode(PIN_MAINS_CLOCK,  INPUT_PULLUP);
  pinMode(PIN_FAN_1,        OUTPUT);
  pinMode(PIN_FAN_2,        OUTPUT);
  digitalWrite(PIN_FAN_1,   LOW);
  digitalWrite(PIN_FAN_2,   LOW);

  // Start, set indicator to RED

  indicator_start();
  indicator.begin();
  indicator.setBrightness(10);
  indicator.setPixelColor(0, INDICATOR_BOOT);
  indicator.show();

  // Setup Serial Monitor

  Serial.begin(115200);
  unsigned long _start = millis();
  while (!Serial && ((millis() - _start < SERIAL_TIMEOUT))) {
    delay(10);   // for nrf52840 with native usb
  }
  DEBUG_COMMENT("Started FanSpeedController.\n");
  // Setup timer

  DEBUG_COMMENT("Setting up hardware timer.\n");
  triac_setup();

  indicator.setPixelColor(0, INDICATOR_OK);
  indicator.show();

  fan1_delay = 0;
  fan2_delay = 0;

  // Setup Bluetooth
  DEBUG_COMMENT("Setting up bluetooth.\n");
  bluetooth_setup();
  bluetooth_set_rx_callback(bluetooth_rx_callback, NULL);

  attachInterrupt(digitalPinToInterrupt(PIN_MAINS_CLOCK),
    zero_crossing_isr, CHANGE);

  DEBUG_COMMENT("Finished setup.\n");
}

unsigned long last_loop_millis = 0;

void loop() {
  if ((millis() - last_loop_millis) > 5000) {
    DEBUG_PRINT("Mains Frequency       = %f\n", calc_mains_freq());
    DEBUG_PRINT("Fan 1 delay           = %ld\n", fan1_delay);
    DEBUG_PRINT("Fan 2 delay           = %ld\n", fan2_delay);
    DEBUG_PRINT("Hardtimer count       = %ld\n", hardtimer_count);
    DEBUG_PRINT("Zerocross pulse width = %ld\n", zero_cross_pulse);

    float speed = calculate_bluetooth_speed();
    DEBUG_PRINT("Speed                 = %f\n", speed);

    if (speed > 5) {
      fan1_delay = 1;
      fan2_delay = 1;
    } else {
      fan1_delay = 0;
      fan2_delay = 0;
    }

    last_loop_millis = millis();
  }
}
