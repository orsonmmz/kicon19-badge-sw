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

#include "commands.h"
#include <stddef.h>

#define CMD_BUF_SIZE    257
static uint8_t cmd_buf[CMD_BUF_SIZE] = {0,};
static uint8_t cmd_buf_ptr = 0;
static uint8_t cmd_resp_buf[CMD_BUF_SIZE];
static uint8_t cmd_overflow = 0;

void cmd_new_data(uint8_t data)
{
    cmd_buf[cmd_buf_ptr] = data;
    ++cmd_buf_ptr;

    if (cmd_buf_ptr == CMD_BUF_SIZE) {
        cmd_overflow = 1;
        cmd_buf_ptr = 0;
    }
}


static inline void cmd_set_response(uint8_t *buf, uint8_t response)
{
    buf[0] = 1;    /* response length */
    buf[1] = response;
}


static inline void cmd_buf_reset(void)
{
    cmd_overflow = 0;
    cmd_buf_ptr = 0;
}


static uint8_t cmd_crc(uint8_t *buf, unsigned int len)
{
    uint8_t crc = 0;

    for(unsigned int i = 0; i < len; ++i) {
        crc ^= buf[i];
    }

    return crc;
}


static int cmd_valid_crc(void)
{
    unsigned int cmd_len = cmd_buf[0];
    uint8_t crc = cmd_crc(&cmd_buf[1], cmd_len);
    return (crc == cmd_buf[cmd_len + 1]);
}


static void cmd_fix_resp_crc(void)
{
    unsigned int cmd_len = cmd_resp_buf[0];
    uint8_t crc = cmd_crc(&cmd_resp_buf[1], cmd_len);
    cmd_resp_buf[cmd_len + 1] = crc;
}


const uint8_t* cmd_try_execute()
{
    unsigned int cmd_len = cmd_buf[0];

    /* Check for overflows */
    if (cmd_overflow) {
        cmd_set_response(cmd_resp_buf, CMD_RESP_OVERFLOW);
    }

    /* Handle reset request */
    else if (cmd_buf_ptr == 1 && cmd_len == CMD_TYPE_RESET) {
        cmd_set_response(cmd_resp_buf, CMD_RESP_RESET);
    }

    /* Is the command complete? (+2 stands for the cmd_len and crc fields) */
    else if (cmd_len + 2 > cmd_buf_ptr) {
        return NULL;
    }

    else {
        if (!cmd_valid_crc()) {
            cmd_buf_reset();
            cmd_set_response(cmd_resp_buf, CMD_RESP_CRC_ERR);

        } else {
            /* Handle usual commands */
            switch(cmd_buf[1]) {
                case CMD_TYPE_UART:
                    cmd_uart(&cmd_buf[2], cmd_len - 1, cmd_resp_buf);
                    break;

                default:
                    cmd_set_response(cmd_resp_buf, CMD_RESP_INVALID_CMD);
                    break;
            }

        }
    }

    cmd_buf_reset();
    cmd_fix_resp_crc();
    return cmd_resp_buf;
}


// TODO move
#include "uart.h"
void cmd_uart(const uint8_t* data_in, unsigned int input_len, uint8_t *data_out)
{
    for (unsigned int i = 0; i < input_len; ++i) {
        while(!uart_is_tx_buf_empty(UART0));
        uart_write(UART0, data_in[i]);
    }

    cmd_set_response(data_out, CMD_RESP_OK);
}
