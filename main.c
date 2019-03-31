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

#include "asf.h"
#include "buttons.h"
#include "io_capture.h"
#include "logic_analyzer.h"
#include "udc.h"
#include "udi_cdc.h"
#include "commands.h"
#include "lcd.h"
#include "gfx.h"
#include "SSD1306_commands.h"
#include "scope.h"
#include "serial.h"
#include "menu.h"


/** Set default LED blink period to 250ms*3 */
#define DEFAULT_LED_FREQ   4

/** LED blink period */
#define LED_BLINK_PERIOD    3

/** LED blink period */
static volatile uint32_t led_blink_period = 0;
volatile bool g_interrupt_enabled = true;


/**
 *  \brief Handler for System Tick interrupt.
 *
 *  Process System Tick Event
 */
void SysTick_Handler(void)
{
}

/**
 * \brief Interrupt handler for TC0 interrupt. Toggles the state of LEDs.
 */
void TC0_Handler(void)
{
    /* Clear status bit to acknowledge interrupt */
    tc_get_status(TC0, 0);

    led_blink_period++;

    if (led_blink_period == LED_BLINK_PERIOD) {
        pio_toggle_pin(PIO_PA8_IDX);
        led_blink_period = 0;
    }
}

/**
 * \brief Configure Timer Counter 0 to generate an interrupt with the specific
 * frequency.
 *
 * \param freq Timer counter frequency.
 */
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

/**
 * \brief Configure timer ISR to fire regularly.
 */
static void init_timer_isr(void)
{
    SysTick_Config((sysclk_get_cpu_hz() / 1000) * 100); // 100 ms
}

/**
 * \brief initialize pins, watchdog, etc.
 */
static void init_system(void)
{
    /* Disable the watchdog */
    wdt_disable(WDT);

    irq_initialize_vectors();
    cpu_irq_enable();

    /* Initialize the system clock */
    sysclk_init();

    /* Configure LED pins */
    pio_configure(PIOA, PIO_OUTPUT_0, (PIO_PA7 | PIO_PA8), PIO_DEFAULT);

    /* Enable PMC clock for key/slider PIOs  */
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOB);

    /* Configure PMC */
    pmc_enable_periph_clk(ID_TC0);

    /* Configure the default TC frequency */
    configure_tc(DEFAULT_LED_FREQ);

    /* Enable the serial console */
    serial_init(115200);

    btn_init();
    SSD1306_init();
    ioc_init();
    la_init();
    udc_start();
}


int main(void)
{
    init_system();

    /* Configure timer ISR to fire regularly */
    init_timer_isr();

    SSD1306_drawBitmap(0, 0, kicon_logo, 128, 32);
    SSD1306_drawBufferDMA();
    SSD1306_setString(20, 7, "press a button", 14, WHITE);

    //while(!btn_state()); // wait for a button press   // TODO uncomment

    while(1) {
        menu();
    }
}
