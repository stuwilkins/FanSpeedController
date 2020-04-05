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
#include "inttimer.h"
#include "wiring.h"
#include "debug.h"
#include "triac.h"

TimerClass triac_inttimer(2);

volatile int zero_cross_clock = 0;
unsigned long zero_cross_last_clock = 0;
volatile bool zero_cross_trigger_1 = false;
volatile bool zero_cross_trigger_2 = false;
volatile long int zero_cross_micros = 0;
unsigned long fan1_delay = 0;
unsigned long fan2_delay = 0;
unsigned long hardtimer_count = 0;
unsigned long zero_cross_pulse1 = 0;
unsigned long zero_cross_pulse2 = 0;
unsigned long zero_cross_positive = 0;
unsigned long zero_cross_negative = 0;

void zero_crossing_isr(void) {
  if (!digitalRead(PIN_MAINS_CLOCK)) {
    zero_cross_clock++;
    zero_cross_trigger_1 = true;
    zero_cross_trigger_2 = true;
    zero_cross_micros = micros();
    zero_cross_negative = micros();
    zero_cross_pulse2 = micros() - zero_cross_positive;
  } else {
    zero_cross_positive = micros();
    zero_cross_pulse1 = micros() - zero_cross_negative;
  }
}

void hardtimer_callback(void* ptr) {
  (void) ptr;

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

  if (fan2_delay > 0) {
    if (zero_cross_trigger_2) {
      if ((micros() - zero_cross_micros) > fan2_delay) {
        digitalWrite(PIN_FAN_2, HIGH);
        delayMicroseconds(50);
        digitalWrite(PIN_FAN_2, LOW);
        zero_cross_trigger_2 = false;
      }
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

void triac_setup(void) {
  triac_inttimer.setCallback(hardtimer_callback);
  triac_inttimer.init(5);
  triac_inttimer.start();
}

void triac_set_output(uint8_t op1, uint8_t op2) {
  // Here we map the OP to values
  // 1 = full on, 6000 = full off
  if (op1 == 0) {
    fan1_delay = 0;
  } else {
    fan1_delay = 6000L - (6000L * op1 / 255L);
    if (fan1_delay == 0) {
      fan1_delay = 1;
    }
  }

  if (op2 == 0) {
    fan2_delay = 0;
  } else {
    fan2_delay = 6000L - (6000L * op2 / 255L);
    if (fan2_delay == 0) {
      fan2_delay = 1;
    }
  }

  DEBUG_PRINT("op1 = %d, op2 = %d\n", op1, op2);
  DEBUG_PRINT("fan1_delay = %ld, fan2_delay = %ld\n",
              fan1_delay, fan2_delay);
}
