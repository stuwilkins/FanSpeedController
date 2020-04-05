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

#include "inttimer.h"
#include <Arduino.h>
#include "nrf_timer.h"

static void defaultFunc(void *ptr) {
  (void)ptr;
}

NRF_TIMER_Type *nrf_timers[] = {NRF_TIMER0, NRF_TIMER1, NRF_TIMER2,
                                NRF_TIMER3, NRF_TIMER4};

TimerClass::TimerClass(int timer) {
  nrf_timer = nrf_timers[timer];

  callback_ptr = defaultFunc;
  Timers[timer] = this;
}

void TimerClass::setNVIC(IRQn_Type IRQn) {
  NVIC_SetPriority(IRQn, 5);
  NVIC_ClearPendingIRQ(IRQn);
  NVIC_EnableIRQ(IRQn);
}

void TimerClass::init(int microsec) {
  // Setup NVIC IRQ
  if (nrf_timer == nrf_timers[1])
    setNVIC(TIMER1_IRQn);
  if (nrf_timer == nrf_timers[2])
    setNVIC(TIMER2_IRQn);
  if (nrf_timer == nrf_timers[3])
    setNVIC(TIMER3_IRQn);
  if (nrf_timer == nrf_timers[4])
    setNVIC(TIMER4_IRQn);

  // Timer mode with 32bit width
  nrf_timer_mode_set(nrf_timer, NRF_TIMER_MODE_TIMER);
  nrf_timer_bit_width_set(nrf_timer, NRF_TIMER_BIT_WIDTH_32);
  nrf_timer_frequency_set(nrf_timer, NRF_TIMER_FREQ_16MHz);

  if (microsec < 0)
    microsec = 0;
  if (microsec >= 1 << 28)
    microsec = (1 << 28) - 1;
  uint32_t ticks = nrf_timer_us_to_ticks(microsec, NRF_TIMER_FREQ_16MHz);
  nrf_timer_cc_write(nrf_timer, NRF_TIMER_CC_CHANNEL0, ticks);
}

void TimerClass::setCallback(funcPtr_t callback, void *ptr) {
  callback_ptr = callback;
  usrptr = ptr;
}

void TimerClass::start(void) {
  nrf_timer_int_enable(nrf_timer, NRF_TIMER_INT_COMPARE0_MASK);
  nrf_timer_task_trigger(nrf_timer, NRF_TIMER_TASK_START);
}

void TimerClass::process(void) {
  nrf_timer_event_clear(nrf_timer, NRF_TIMER_EVENT_COMPARE0);
  nrf_timer_task_trigger(nrf_timer, NRF_TIMER_TASK_CLEAR);
  (*callback_ptr)(usrptr);
}

// Timer 0 is used by the soft device but Timer 1, 2, 3 and 4 are available
extern "C" void TIMER1_IRQHandler(void) {
  TimerClass::trigger(Timers[1]);
}

extern "C" void TIMER2_IRQHandler(void) {
  TimerClass::trigger(Timers[2]);
}

extern "C" void TIMER3_IRQHandler(void) {
  TimerClass::trigger(Timers[3]);
}

extern "C" void TIMER4_IRQHandler(void) {
  TimerClass::trigger(Timers[4]);
}

TimerClass *Timers[5] = {0, 0, 0, 0, 0};
