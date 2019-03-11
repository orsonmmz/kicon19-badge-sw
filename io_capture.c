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

#include "io_capture.h"

#include <pdc.h>
#include <pio.h>
#include <pio_handler.h>
#include <pmc.h>

/** Pointer to PDC register base. */
static Pdc *p_pdc;

static int busy = 0;

static void (*finish_handler)(void*) = NULL;
static void* finish_handler_param = NULL;
static int pio_pll_prescaler = 0;

/** PIOA interrupt priority. */
#define PIO_IRQ_PRI                    (4)

static void capture_handler(Pio *p_pio)
{
    uint32_t dummy_data;

    pmc_disable_pck(PMC_PCK_0);
    pio_capture_disable_interrupt(p_pio, (PIO_PCIDR_ENDRX | PIO_PCIDR_RXBUFF));
    pdc_disable_transfer(p_pdc, PERIPH_PTCR_RXTEN);

    /* Clear any unwanted data */
    pio_capture_read(PIOA, &dummy_data);

    if(finish_handler)
        (*finish_handler)(finish_handler_param);

    busy = 0;
}


void ioc_init()
{
    pmc_enable_periph_clk(ID_PIOA);
    pio_capture_handler_set(capture_handler);

    /* Initialize PIO Parallel Capture function. */
    pio_capture_set_mode(PIOA, PIO_PCMR_ALWYS);
    pio_capture_enable(PIOA);

    /* Disable all PIOA I/O line interrupt. */
    pio_disable_interrupt(PIOA, 0xFFFFFFFF);

    /* Configure the sampling clock generator */
    ioc_set_clock(F1MHZ);
    pio_configure(PIOA, PIO_PERIPH_B, PIO_PA6B_PCK0, PIO_DEFAULT);

    p_pdc = pio_capture_get_pdc_base(PIOA);

    /* Configure and enable interrupt of PIO. */
    NVIC_DisableIRQ(PIOA_IRQn);
    NVIC_ClearPendingIRQ(PIOA_IRQn);
    NVIC_SetPriority(PIOA_IRQn, PIO_IRQ_PRI);
    NVIC_EnableIRQ(PIOA_IRQn);
}


void ioc_set_clock(clock_freq_t freq)
{
    int clk_pre = 0, pll_pre = 0;

    pmc_disable_pck(PMC_PCK_0);

    switch(freq) {
        //case F64MHZ:    pll_pre = 31; clk_pre = PMC_PCK_PRES_CLK_1; break;
        case F32MHZ:    pll_pre = 31; clk_pre = PMC_PCK_PRES_CLK_2; break;
        case F16MHZ:    pll_pre = 31; clk_pre = PMC_PCK_PRES_CLK_4; break;
        case F8MHZ:     pll_pre = 31; clk_pre = PMC_PCK_PRES_CLK_8; break;

        case F50MHZ:    pll_pre = 24; clk_pre = PMC_PCK_PRES_CLK_1; break;
        case F25MHZ:    pll_pre = 24; clk_pre = PMC_PCK_PRES_CLK_2; break;
        case F12_5MHZ:  pll_pre = 24; clk_pre = PMC_PCK_PRES_CLK_4; break;

        case F40MHZ:    pll_pre = 19; clk_pre = PMC_PCK_PRES_CLK_1; break;
        case F20MHZ:    pll_pre = 19; clk_pre = PMC_PCK_PRES_CLK_2; break;
        case F10MHZ:    pll_pre = 19; clk_pre = PMC_PCK_PRES_CLK_4; break;
        case F5MHZ:     pll_pre = 19; clk_pre = PMC_PCK_PRES_CLK_8; break;

        case F6MHZ:     pll_pre = 2;  clk_pre = PMC_PCK_PRES_CLK_1; break;
        case F3MHZ:     pll_pre = 2;  clk_pre = PMC_PCK_PRES_CLK_2; break;

        case F4MHZ:     pll_pre = 1;  clk_pre = PMC_PCK_PRES_CLK_1; break;
        case F2MHZ:     pll_pre = 1;  clk_pre = PMC_PCK_PRES_CLK_2; break;
        case F1MHZ:     pll_pre = 1;  clk_pre = PMC_PCK_PRES_CLK_4; break;
        case F500KHZ:   pll_pre = 1;  clk_pre = PMC_PCK_PRES_CLK_8; break;
        case F250KHZ:   pll_pre = 1;  clk_pre = PMC_PCK_PRES_CLK_16; break;
        case F125KHZ:   pll_pre = 1;  clk_pre = PMC_PCK_PRES_CLK_32; break;
    }

    if (pll_pre != pio_pll_prescaler) {
        pio_pll_prescaler = pll_pre;
        pmc_enable_pllbck(pll_pre, 0x00, 6);
        while(!pmc_is_locked_pllbck());
    }

    pmc_switch_pck_to_pllbck(PMC_PCK_0, clk_pre);
}


void ioc_fetch(uint8_t *destination, uint32_t sample_count)
{
    /** PDC data packet. */
    pdc_packet_t packet_t;

    if (busy)
        return;

    busy = 1;

    /* Set up PDC receive buffer */
    packet_t.ul_addr = (uint32_t) destination;
    packet_t.ul_size = sample_count;
    pdc_rx_init(p_pdc, &packet_t, NULL);

    /* Enable the sampling clock */
    pmc_enable_pck(PMC_PCK_0);

    /* Enable PDC transfer. */
    pdc_enable_transfer(p_pdc, PERIPH_PTCR_RXTEN);

    /* Configure the PIO capture interrupt mask. */
    pio_capture_enable_interrupt(PIOA, (PIO_PCIER_ENDRX | PIO_PCIER_RXBUFF));
}


int ioc_busy(void)
{
    return busy;
}


void ioc_set_handler(void (*func)(void*), void* param)
{
    finish_handler = func;
    finish_handler_param = param;
}
