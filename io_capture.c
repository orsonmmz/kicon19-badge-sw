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

static volatile int busy = 0;

static void dummy_handler(int buf) {}

static void (*finish_handler)(int) = dummy_handler;
static int pio_pll_prescaler = 0;

static ioc_buffer_t *ioc_buffers;
static int ioc_buffers_cnt;

static volatile int ioc_buffer_idx;
static volatile int ioc_buffer_current;


/* PIOA interrupt priority */
#define PIO_IRQ_PRI                    (4)

void ioc_init()
{
    p_pdc = pio_capture_get_pdc_base(PIOA);
    pmc_enable_periph_clk(ID_PIOA);

    /* Initialize PIO Parallel Capture function. */
    pio_capture_set_mode(PIOA, PIO_PCMR_ALWYS);
    pio_capture_enable(PIOA);

    /* Disable all PIOA I/O line interrupt. */
    pio_disable_interrupt(PIOA, 0xFFFFFFFF);

    /* Configure the sampling clock generator */
    ioc_set_clock(F1MHZ);
    pio_configure(PIOA, PIO_PERIPH_B, PIO_PA6B_PCK0, PIO_DEFAULT);

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


static inline int ioc_set_next_buffer(void) {
    /* save the index of the currently acquired buffer */
    ioc_buffer_current = ioc_buffer_idx;

    if (ioc_buffers[ioc_buffer_idx].last) {
        /* the last buffer has already been requested, stop here */
        return 0;
    }

    /* move to the next buffer, wrap the index if needed */
    if (++ioc_buffer_idx >= ioc_buffers_cnt) {
        ioc_buffer_idx = 0;
    }

    /* set the next buffer */
    p_pdc->PERIPH_RNPR = (uint32_t) ioc_buffers[ioc_buffer_idx].addr;
    p_pdc->PERIPH_RNCR = ioc_buffers[ioc_buffer_idx].size;

    return 1;
}


void ioc_start(ioc_buffer_t *buffers, int count)
{
    busy = 1;
    ioc_buffer_idx = 0;
    ioc_buffers = buffers;
    ioc_buffers_cnt = count;

    /* Set up PDC receive buffer */
    p_pdc->PERIPH_RPR = (uint32_t) ioc_buffers[0].addr;
    p_pdc->PERIPH_RCR = ioc_buffers[0].size;

    ioc_set_next_buffer();

    /* Configure the PIO capture interrupt mask. */
    pio_capture_enable_interrupt(PIOA, (PIO_PCIER_ENDRX | PIO_PCIER_RXBUFF));

    /* Enable PDC transfer. */
    pdc_enable_transfer(p_pdc, PERIPH_PTCR_RXTEN);

    /* Enable the sampling clock */
    pmc_enable_pck(PMC_PCK_0);
}


int ioc_busy(void)
{
    return busy;
}


void ioc_set_handler(void (*func)(int))
{
    finish_handler = func;
}


void PIOA_Handler(void)
{
    int cur_buf = ioc_buffer_current;
    ioc_set_next_buffer();

    (*finish_handler)(cur_buf);

    /* RXBUFF is set when there are no more buffers configured for acquisition */
    if ((pio_capture_get_interrupt_status(PIOA) & PIO_PCISR_RXBUFF)) {
        pmc_disable_pck(PMC_PCK_0);
        pdc_disable_transfer(p_pdc, PERIPH_PTCR_RXTDIS);
        pio_capture_disable_interrupt(PIOA, (PIO_PCIDR_ENDRX | PIO_PCIDR_RXBUFF));

        /* Clear any unwanted data */
        uint32_t dummy_data;
        pio_capture_read(PIOA, &dummy_data);
        busy = 0;
    }
}
