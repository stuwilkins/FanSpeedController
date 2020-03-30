#ifndef _INTTIMER_H_
#define _INTTIMER_H_

#include <nrf_timer.h>

typedef void (*funcPtr_t)();

class TimerClass {
    public:
        TimerClass(int timer = 1, int channel = 0);
        void init(int microsecs);
        void setCallback(funcPtr_t callback);
        void start(void);
        static void trigger(TimerClass* ctx) {
            if(ctx) ctx->process();
        }
    private:
        void process(void);
        void setNVIC(IRQn_Type IRQn);
        NRF_TIMER_Type*        nrf_timer;
        nrf_timer_cc_channel_t cc_channel;
        funcPtr_t callback_ptr;
};

extern NRF_TIMER_Type* nrf_timers[5];
extern TimerClass* Timers[5];

#endif // _INTTIMER_H_

