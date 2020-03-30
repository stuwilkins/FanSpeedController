#include <Arduino.h>
#include "nrf_timer.h"
#include "inttimer.h"

static void defaultFunc() {};

NRF_TIMER_Type* nrf_timers[] = {NRF_TIMER0, NRF_TIMER1, NRF_TIMER2,
                                NRF_TIMER3, NRF_TIMER4};

TimerClass::TimerClass(int timer, int channel) {
    nrf_timer = nrf_timers[timer];
    cc_channel = nrf_timer_cc_channel_t(channel);

    callback_ptr = defaultFunc;
    Timers[timer] = this;

}

void TimerClass::init(void) {
    // Timer mode with 32bit width
    nrf_timer_mode_set(nrf_timer, NRF_TIMER_MODE_TIMER);
    nrf_timer_bit_width_set(nrf_timer, NRF_TIMER_BIT_WIDTH_32);
    nrf_timer_frequency_set(nrf_timer, NRF_TIMER_FREQ_16MHz);
}

void TimerClass::setCallback(funcPtr_t callback) {
    callback_ptr = callback;
}

void TimerClass::setNVIC(IRQn_Type IRQn) {
    NVIC_SetPriority(IRQn, 3);
    NVIC_ClearPendingIRQ(IRQn);
    NVIC_EnableIRQ(IRQn);
}

void TimerClass::setup(int microsec) {

}

void TimerClass::attachInterrupt(int microsec) {
    // Start if not already running (and reset?)
    nrf_timer_task_trigger(nrf_timer, NRF_TIMER_TASK_START);
    nrf_timer_task_trigger(nrf_timer, NRF_TIMER_TASK_CLEAR);

    // Clear and enable compare interrupt
    nrf_timer_int_mask_t chanel_mask = NRF_TIMER_INT_COMPARE0_MASK; // nrf_timer_compare_int_get(cc_channel);
    nrf_timer_int_enable(nrf_timer, chanel_mask);

    if (nrf_timer == nrf_timers[1]) setNVIC(TIMER1_IRQn);
    if (nrf_timer == nrf_timers[2]) setNVIC(TIMER2_IRQn);
    if (nrf_timer == nrf_timers[3]) setNVIC(TIMER3_IRQn);
    if (nrf_timer == nrf_timers[4]) setNVIC(TIMER4_IRQn);

    // Program compare register between 0 & 2**28 (max on 32 bits @16 ticks/us)
    if (microsec < 0)           microsec = 0;
    if (microsec >= 1 << 28)    microsec = (1 << 28) - 1;
    uint32_t ticks = nrf_timer_us_to_ticks(microsec, NRF_TIMER_FREQ_16MHz);
    nrf_timer_cc_write(nrf_timer, cc_channel, ticks);
}

inline void TimerClass::detachInterrupt() {
    // Stop timer
    nrf_timer_task_trigger(nrf_timer, NRF_TIMER_TASK_STOP);

    // Disable timer compare interrupt
    nrf_timer_int_mask_t chanel_mask = nrf_timer_compare_int_get(cc_channel);
    nrf_timer_int_disable(nrf_timer, chanel_mask);

    // Clear event - TODO?
    nrf_timer_event_t event = nrf_timer_compare_event_get(chanel_mask);
    nrf_timer_event_clear(nrf_timer, event);
}

void TimerClass::process(void) {
    detachInterrupt();
    (*callback_ptr)();
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

TimerClass* Timers[5] = {0, 0, 0, 0, 0};