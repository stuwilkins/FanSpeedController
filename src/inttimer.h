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

#ifndef SRC_INTTIMER_H_
#define SRC_INTTIMER_H_

#include <nrf_timer.h>

typedef void (*funcPtr_t)();

class TimerClass {
 public:
  explicit TimerClass(int timer = 1);
  void init(int microsecs);
  void setCallback(funcPtr_t callback);
  void start(void);
  static void trigger(TimerClass* ctx) {
    if (ctx) ctx->process();
  }
 private:
  void process(void);
  void setNVIC(IRQn_Type IRQn);
  NRF_TIMER_Type*        nrf_timer;
  funcPtr_t callback_ptr;
};

extern NRF_TIMER_Type* nrf_timers[5];
extern TimerClass* Timers[5];

#endif  // SRC_INTTIMER_H_
