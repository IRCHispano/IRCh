/************************************************************************
 *   IRC - Internet Relay Chat, ircd/ircd_alloc.c
 *   Copyright (C) 1999 Thomas Helvey (BleepSoft)
 *                     
 *   See file AUTHORS in IRC package for additional names of
 *   the programmers. 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   $Id$
 */
#include "config.h"

#include "ircd_alloc.h"
#include "ircd_string.h"
#include "s_debug.h"

#include <assert.h>

static void nomem_handler(void);

/* Those ugly globals... */
OutOfMemoryHandler noMemHandler = nomem_handler;
void *malloc_tmp;

static void
nomem_handler(void)
{
#ifdef MDEBUG
  assert(0);
#else
  Debug((DEBUG_FATAL, "Out of memory, exiting"));
  exit(2);
#endif
}

void
set_nomem_handler(OutOfMemoryHandler handler)
{
  noMemHandler = handler;
}
