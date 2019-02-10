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

#ifndef USB_HANDLERS_H
#define USB_HANDLERS_H

#include <stdint.h>
#include <stdbool.h>

void usb_handler_suspend_action(void);
void usb_handler_resume_action(void);
void usb_handler_sof_action(void);
void usb_handler_remotewakeup_enable(void);
void usb_handler_remotewakeup_disable(void);
bool usb_handler_cdc_enable(uint8_t port);
void usb_handler_cdc_disable(uint8_t port);

#endif /* USB_HANDLERS_H */
