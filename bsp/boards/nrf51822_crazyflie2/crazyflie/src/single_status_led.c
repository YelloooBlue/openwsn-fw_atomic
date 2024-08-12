/**
 * 
 * @file single_status_led.c
 * @brief A simple interface for controlling the status LED on the nRF51822 Crazyflie 2.X.
 *          The status LED is used to indicate the status of the Crazyflie 2.X and the OpenWSN stack.
 * @author Lan HUANG (YelloooBlue@outlook.com)
 * @date Aug 2024
 *
 */

#include "single_status_led.h"
#include "cf_systick.h"

//=========================== defines =========================================

//=========================== variables =======================================

static led_state_t led_state = LED_OFF;

//=========================== prototypes ======================================

//=========================== public ==========================================

void status_led_init() {
    //leds_init();
}

void status_led_handle() {

    unsigned int tick_ms = systickGetTick();

    switch (led_state) {
        case LED_OFF:
            leds_all_off();
            break;
        case LED_ON:
            leds_all_on();
            break;
        case LED_BLINK:
            if (tick_ms % 2000 < 1000) {
                leds_all_on();
            } else {
                leds_all_off();
            }
            break;
        case LED_BLINK_FAST:
            if (tick_ms % 1000 < 500) {
                leds_all_on();
            } else {
                leds_all_off();
            }
            break;
        case LED_BLINK_SLOW:
            if (tick_ms % 4000 < 2000) {
                leds_all_on();
            } else {
                leds_all_off();
            }
            break;
        case LED_BLINK_LSS:
            if (tick_ms % 2000 < 1000) {
                leds_all_on();
            } else if (tick_ms % 2000 < 1250) {
                leds_all_off();
            } else if (tick_ms % 2000 < 1500) {
                leds_all_on();
            } else if (tick_ms % 2000 < 1750) {
                leds_all_off();
            } else {
                leds_all_on();
            }
            break;
        default:
            break;
    }
}

void status_led_set(led_state_t state) {
    led_state = state;
}

void status_led_clear() {
    led_state = LED_OFF;
    leds_all_off();
}

//=========================== private =========================================

//=========================== interrup handlers ===============================