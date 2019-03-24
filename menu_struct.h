/*
 * Copyright (C) 2014 Julian Lewis
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0-standalone.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @brief Menu data structures.
 */

#ifndef MENU_STRUCT_H
#define MENU_STRUCT_H

//#include "application.h"  // TODO
//#include "settings/settings.h"

/**
 * @brief Type of menu entry.
 */
typedef enum _entry_type_t {
    APP,
    SUBMENU,
    SETTING,
    END         // sentinel, should be put as the last entry
} entry_type_t;

typedef struct _application_t {
    char name[21];
    void (*func)(void);
} application_t;

typedef struct _setting_t {
    char name[21];
    int val;
    struct _menu_list_t *submenu;
} setting_t;

/**
 * @brief Structure that represents a single
 * entry - either application or submenu.
 * Menus and submenus are defined in menu_struct.c
 * Applications are defined in app_list.h and in their respective files
 * Settings are defined in settings/settings.h
 */
typedef struct _menu_entry_t {
    entry_type_t type;

    union {
        application_t *app;
        struct _menu_list_t *submenu;
        const char *setting;
    } data;
} menu_entry_t;

/**
 * @brief Structure that contains a list
 * of menu entries (submenu).
 */
typedef struct _menu_list_t {
    char name[21];
    int val;
    menu_entry_t entries[];
} menu_list_t;

/**
 * @brief Returns the number of entries for a given submenu.
 * @param menu is the menu that length is returned.
 */
int get_menu_size(const menu_list_t *menu);

/**
 * @brief Stores the main menu structure.
 */
extern menu_list_t main_menu;

#endif /* MENU_H */

