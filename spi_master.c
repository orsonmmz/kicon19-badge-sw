/*
 * Copyright (c) 2019 Katarzyna Stachyra <kas.stachyra@gmail.com>
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

#include "spi_master.h"
#include "asf.h"

/* Chip select. */
#define SPI_CHIP_SEL 1
#define SPI_CHIP_PCS spi_get_pcs(SPI_CHIP_SEL)

/* Clock polarity. */
#define SPI_CLK_POLARITY 0

/* Clock phase. */
#define SPI_CLK_PHASE 0

/* Delay before SPCK. */
#define SPI_DLYBS 0x40

/* Delay between consecutive transfers. */
#define SPI_DLYBCT 0x10


/* SPI clock setting (Hz). */
static uint32_t gs_ul_spi_clock = 500000;

void spi_init(void)
{
    pio_configure(PIOA, PIO_PERIPH_A,
            (PIO_PA12A_MISO | PIO_PA13A_MOSI | PIO_PA14A_SPCK),
            PIO_OPENDRAIN | PIO_PULLUP);

    pio_configure(PIOB, PIO_PERIPH_A, PIO_PB14A_NPCS1,
            PIO_OPENDRAIN | PIO_PULLUP);

    spi_enable_clock(SPI);
    spi_disable(SPI);
    spi_reset(SPI);
    spi_set_master_mode(SPI);
    spi_disable_mode_fault_detect(SPI);
    spi_disable_loopback(SPI);
    spi_set_peripheral_chip_select_value(SPI, SPI_CHIP_PCS);
    spi_set_clock_polarity(SPI, SPI_CHIP_SEL, SPI_CLK_POLARITY);
    spi_set_clock_phase(SPI, SPI_CHIP_SEL, SPI_CLK_PHASE);
    spi_set_bits_per_transfer(SPI, SPI_CHIP_SEL,SPI_CSR_BITS_8_BIT);
    spi_set_baudrate_div(SPI, SPI_CHIP_SEL,
            (sysclk_get_peripheral_hz() / gs_ul_spi_clock));
    spi_set_transfer_delay(SPI, SPI_CHIP_SEL, SPI_DLYBS, SPI_DLYBCT);
    spi_enable(SPI);
}

void spi_master_transfer(const uint8_t *buf, uint32_t size)
{
    uint8_t uc_pcs;
    static uint16_t data;

    for (int i = 0; i < size; i++) {
        spi_write(SPI, buf[i], 0, 0);
        while ((spi_read_status(SPI) & SPI_SR_RDRF) == 0);
//        spi_read(SPI, &data, &uc_pcs);
//        p_buffer[i] = data;
    }
}
