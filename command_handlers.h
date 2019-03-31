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

#ifndef COMMAND_HANDLERS_H
#define COMMAND_HANDLERS_H

#include <stdint.h>
#include "commands_def.h"

/* Function that should be used by the command handlers to return a response */
void cmd_response(const uint8_t* buf, unsigned int len);

/* Starts a response to a command */
void cmd_resp_init(cmd_resp_t status);

/* Writes data to the response buffer */
void cmd_resp_write(uint8_t data);
void cmd_resp_writen(const uint8_t *data, unsigned int len);

/* Logic analyzer SUMP protocol handler */
int cmd_sump(const uint8_t* cmd, unsigned int len);

void cmd_uart(const uint8_t* data_in, unsigned int input_len);
void cmd_i2c(const uint8_t* data_in, unsigned int input_len);
void cmd_spi(const uint8_t* data_in, unsigned int input_len);
void cmd_led(const uint8_t* data_in, unsigned int input_len);
/*void cmd_lcd(const uint8_t* data_in, unsigned int input_len);*/
void cmd_btn(const uint8_t* data_in, unsigned int input_len);
/*void cmd_adc(const uint8_t* data_in, unsigned int input_len);*/
/*void cmd_dac(const uint8_t* data_in, unsigned int input_len);*/

#endif /* COMMAND_HANDLERS_H */
