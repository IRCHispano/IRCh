/*
 * IRC-Hispano IRC Daemon, include/ircd_crypt_native.h
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2002 hikari
 * Copyright (C) 1990-1991 Armin Gruner
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/** @file
 * @brief Native crypt() function declarations.
 */
#ifndef INCLUDED_ircd_crypt_native_h
#define INCLUDED_ircd_crypt_native_h

extern const char* ircd_crypt_native(const char* key, const char* salt);
extern void ircd_register_crypt_native(void);

#endif /* INCLUDED_ircd_crypt_native_h */

