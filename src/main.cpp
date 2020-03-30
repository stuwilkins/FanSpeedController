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
#include "wifi.h"
#include "bluetooth.h"

#define PIN_MAINS_CLOCK       6
#define PIN_FAN_1             5
#define PIN_FAN_2             9
#define SERIAL_TIMEOUT        15000


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

// TimerHandle_t softtimer;
TimerClass timer1(4);
// TimerClass timer2(4, 0);

volatile int zero_cross_clock = 0;
unsigned long zero_cross_last_clock = 0;
volatile bool zero_cross_trigger_1 = false;
volatile bool zero_cross_trigger_2 = false;
volatile long int zero_cross_micros = 0;
unsigned long fan1_delay = 0;
unsigned long fan2_delay = 0;
unsigned long hardtimer_count = 0;

void zero_crossing_isr(void) {
  zero_cross_clock++;
  zero_cross_trigger_1 = true;
  zero_cross_trigger_2 = true;
  zero_cross_micros = micros();
}

void hardtimer_callback(void) {
  if (fan1_delay > 0) {
    if (zero_cross_trigger_1) {
      if ((micros() - zero_cross_micros) > fan1_delay) {
        digitalWrite(PIN_FAN_1, HIGH);
        delayMicroseconds(50);
        digitalWrite(PIN_FAN_1, LOW);
        zero_cross_trigger_1 = false;
      }
    }
  } else {
    zero_cross_trigger_1 = false;
  }

  if (zero_cross_trigger_2) {
    if ((micros() - zero_cross_micros) > fan2_delay) {
      digitalWrite(PIN_FAN_2, HIGH);
      delayMicroseconds(50);
      digitalWrite(PIN_FAN_2, LOW);
      zero_cross_trigger_2 = false;
    }
  }

  hardtimer_count++;
}

float calc_mains_freq(void) {
  float _freq = zero_cross_clock;
  float _diff = (static_cast<float>(millis())
    - static_cast<float>(zero_cross_last_clock));
  if (_diff == 0) {
    return 0;
  }

  // Catch divide by zero
  _freq /= _diff;
  _freq *= 500;  // Convert to seconds (and 2 per cycle)

  zero_cross_last_clock = millis();
  zero_cross_clock = 0;

  return _freq;
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
  // Setup timer

  timer1.setCallback(hardtimer_callback);
  timer1.init(5);
  timer1.start();
  digitalWrite(LED_BUILTIN, LOW);

  // Setup Bluetooth
  setup_bluetooth();

  // Setup WiFi
  setup_wifi();

  indicator.setPixelColor(0, INDICATOR_OK);
  indicator.show();

  fan1_delay = 0;
  fan2_delay = 1;

  attachInterrupt(digitalPinToInterrupt(PIN_MAINS_CLOCK),
    zero_crossing_isr, FALLING);
}

void loop() {
  Serial.print("Mains Frequency = ");
  Serial.println(calc_mains_freq());
  delay(250);
  Serial.print("Fan 1 period = ");
  Serial.println(fan1_delay);
  delay(250);
  Serial.print("Fan 2 period = ");
  Serial.println(fan2_delay);
  delay(250);
  Serial.print("Hardtimer count = ");
  Serial.println(hardtimer_count);
  delay(250);
  calculate_bluetooth();
  // if ( Bluefruit.Central.connected() )
  // {
  //   // Not discovered yet
  //   if ( clientSandC.discovered() )
  //   {
  //     // Discovered means in working state
  //     // Get Serial input and send to Peripheral
  //     //if ( Serial.available() )
  //     //{
  //     //  delay(2); // delay a bit for all characters to arrive

  //     //  char str[20+1] = { 0 };
  //     //  Serial.readBytes(str, 20);

  //     //  clientUart.print( str );
  //     //}
  //   }
  // }
  // // put your main code here, to run repeatedly:

  delay(2000);
}
