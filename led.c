/*
 * Copyright (c) 2019 Maciej Suminski <orson@orson.net.pl>
 *
 * This source code is free software; you can redistribute it
 * and/or modify it in source code form under the terms of the GNU
 * General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "led.h"
#include "command_handlers.h"

#include <sysclk.h>
#include <tc.h>
#include <pio.h>

#define LED1_IDX PIO_PA7_IDX
#define LED2_IDX PIO_PA8_IDX

/** Set default LED blink period to 250ms*3 */
#define DEFAULT_LED_FREQ   4

static unsigned int led_blink_period1, led_blink_period2;
static volatile uint32_t led_blink_cnt1, led_blink_cnt2;

static void configure_tc(uint32_t freq)
{
    uint32_t ul_div;
    uint32_t ul_tcclks;
    uint32_t ul_sysclk = sysclk_get_cpu_hz();

    /* Disable TC first */
    tc_stop(TC0, 0);
    tc_disable_interrupt(TC0, 0, TC_IER_CPCS);

    /** Configure TC with the frequency and trigger on RC compare. */
    tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
    tc_init(TC0, 0, ul_tcclks | TC_CMR_CPCTRG);
    tc_write_rc(TC0, 0, (ul_sysclk / ul_div) / 4);

    /* Configure and enable interrupt on RC compare */
    NVIC_EnableIRQ((IRQn_Type)ID_TC0);
    tc_enable_interrupt(TC0, 0, TC_IER_CPCS);

    /** Start the counter. */
    tc_start(TC0, 0);
}


void led_init(void) {
    led_blink_period1 = 0;
    led_blink_period2 = 0;

    pio_configure(PIOA, PIO_OUTPUT_0, (PIO_PA7 | PIO_PA8), PIO_DEFAULT);
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_TC0);
    configure_tc(DEFAULT_LED_FREQ);
}


void led_blink(led_t led, unsigned int period) {
    switch(led) {
        case LED1:
            led_blink_period1 = period;
            led_blink_cnt1 = 0;
            break;

        case LED2:
            led_blink_period2 = period;
            led_blink_cnt2 = 0;
            break;
    }
}


void led_set(led_t led, unsigned int state) {
    switch(led) {
        case LED1:
            led_blink_period1 = 0;

            if (state) {
                pio_set_pin_high(LED1_IDX);
            } else {
                pio_set_pin_low(LED1_IDX);
            }
            break;

        case LED2:
            led_blink_period2 = 0;

            if (state) {
                pio_set_pin_high(LED1_IDX);
            } else {
                pio_set_pin_low(LED1_IDX);
            }
            break;
    }
}


/**
 * \brief Interrupt handler for TC0 interrupt. Toggles the state of LEDs.
 */
void TC0_Handler(void) {
    /* Clear status bit to acknowledge interrupt */
    tc_get_status(TC0, 0);

    if (led_blink_period1 > 0 && ++led_blink_cnt1 == led_blink_period1) {
        led_blink_cnt1 = 0;
        pio_toggle_pin(LED1_IDX);
    }

    if (led_blink_period2 > 0 && ++led_blink_cnt2 == led_blink_period2) {
        led_blink_cnt2 = 0;
        pio_toggle_pin(LED2_IDX);
    }
}


void cmd_led(const uint8_t* data_in, unsigned int input_len) {
    switch(data_in[0]) {
        case CMD_LED_SET: led_set(data_in[1], data_in[2]); break;
        case CMD_LED_BLINK: led_blink(data_in[1], data_in[2]); break;
        default: cmd_resp_init(CMD_RESP_INVALID_CMD); return;
    }

    cmd_resp_init(CMD_RESP_OK);
}
