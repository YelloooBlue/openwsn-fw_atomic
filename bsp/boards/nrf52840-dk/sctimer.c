/**
 * brief A timer module with only a single compare value. 
 *
 * Authors: Tamas Harczos (1, tamas.harczos@imms.de) and Adam Sedmak (2, adam.sedmak@gmail.com)
 * Company: (1) Institut fuer Mikroelektronik- und Mechatronik-Systeme gemeinnuetzige GmbH (IMMS GmbH)
 *          (2) Faculty of Electronics and Computing, Zagreb, Croatia
 *    Date: May 2018
 *
 *    Note: We use RTC0 peripheral with its CC0 register.
*/

#include "nrf52840.h"
#include "nrf52840.h"
#include "sctimer.h"

// ========================== define ==========================================

#define MINIMUM_ISR_ADVANCE         16         ///< number of ticks to set CC ahead to make sure the RTC will fire (should this be equal to TIMERTHRESHOLD of opentimers?)
#define TIMERLOOP_THRESHOLD         0x20000   ///< 3s, if sctimer_setCompare() is late by max that many ticks, we still issue the ISR 
#define MAX_RTC_TASKS_DELAY         47        ///< maximum delay in us until an RTC config task is executed


// ========================== variable ========================================

typedef struct
{
  sctimer_cbt         cb;
  uint32_t            last_counter;
  uint32_t            counter_MSB;      ///< the first 8 bits of the 32 bit counter (which do not exist in the physical timer)
  uint32_t            cc32bit_MSB;      ///< the first 8 bits of the 32 bit CC (capture and compare) value, set
  bool                RTC_enabled;
} sctimer_vars_t;

sctimer_vars_t sctimer_vars= {0};

// ========================== prototypes========================================

// ========================== protocol =========================================

/**
\brief Initialization sctimer.
*/
void sctimer_init(void) {
}

void sctimer_set_callback(sctimer_cbt cb) {
    sctimer_vars.cb= cb;
}

/**
\brief set compare interrupt
*/
void sctimer_setCompare(PORT_TIMER_WIDTH val) {

    uint32_t counter_current;
    
    counter_current = NRF_RTC0->COUNTER;

    if (counter_current - val < TIMERLOOP_THRESHOLD) {
        // the timer is already late, schedule the ISR right now manually 
        NRF_RTC0->EVENTS_COMPARE[0] = 1; // need to check if this works
    } else {
        if (val - counter_current < MINIMUM_ISR_ADVANCE)  {
            // there is hardware limitation to schedule the timer within TIMERTHRESHOLD ticks
            // schedule ISR right now manually
            NRF_RTC0->EVENTS_COMPARE[0] = 1; // need to check if this works
        } else {
            // schedule the timer at val
            NRF_RTC0->CC[0] = val;
        }
    }
}

/**
\brief Return the current value of the timer's counter.

 \returns The current value of the timer's counter.
*/
PORT_TIMER_WIDTH sctimer_readCounter(void) {
    return NRF_RTC0->COUNTER;
}

void sctimer_enable(void) {

    NRF_RTC0->INTENSET = (uint32_t)(1<<16);
}

void sctimer_disable(void) {

    NRF_RTC0->INTENCLR = (uint32_t)(1<<16);
}


//=========================== interrupt handler ===============================
