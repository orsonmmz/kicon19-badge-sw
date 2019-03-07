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
    //pmc_enable_pllbck(24, 0x00, 6); // 50 MHz (=12 MHz * (24+1) / 6)
    pmc_enable_pllbck(15, 0x00, 3); // 64 MHz (=12 MHz * (15+1) / 6)
    pio_configure(PIOA, PIO_PERIPH_B, PIO_PA6B_PCK0, PIO_DEFAULT);
    pmc_disable_pck(PMC_PCK_0);
    while(!pmc_is_locked_pllbck());
    ioc_set_clock(F1MHZ);

    p_pdc = pio_capture_get_pdc_base(PIOA);

    /* Configure and enable interrupt of PIO. */
    NVIC_DisableIRQ(PIOA_IRQn);
    NVIC_ClearPendingIRQ(PIOA_IRQn);
    NVIC_SetPriority(PIOA_IRQn, PIO_IRQ_PRI);
    NVIC_EnableIRQ(PIOA_IRQn);
}


void ioc_set_clock(clock_freq_t freq)
{
    // TODO other frequencies might be achieved by the PLLB reconfiguration
    switch(freq) {
        case F32MHZ:  pmc_switch_pck_to_pllbck(PMC_PCK_0, PMC_PCK_PRES_CLK_2); break;
        case F16MHZ:  pmc_switch_pck_to_pllbck(PMC_PCK_0, PMC_PCK_PRES_CLK_4); break;
        case F8MHZ:   pmc_switch_pck_to_pllbck(PMC_PCK_0, PMC_PCK_PRES_CLK_8); break;
        case F4MHZ:   pmc_switch_pck_to_pllbck(PMC_PCK_0, PMC_PCK_PRES_CLK_16); break;
        case F2MHZ:   pmc_switch_pck_to_pllbck(PMC_PCK_0, PMC_PCK_PRES_CLK_32); break;
        case F1MHZ:   pmc_switch_pck_to_pllbck(PMC_PCK_0, PMC_PCK_PRES_CLK_64); break;
    }
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
