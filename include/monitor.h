/*
 * IRC-Hispano IRC Daemon, include/monitor.h
 *
 * Copyright (C) 1997-2017 IRC-Hispano Development Team <devel@irc-hispano.es>
 * Copyright (C) 2017 Toni Garcia (zoltan) <toni@tonigarcia.es>
 * Copyright (C) 2002 Toni Garcia (zoltan) <toni@tonigarcia.es> (WATCH)
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
#ifndef INCLUDED_monitor_h
#define INCLUDED_monitor_h

#ifndef INCLUDED_config_h
#include "config.h"
#endif
#ifndef INCLUDED_sys_types_h
#include <sys/types.h>
#define INCLUDED_sys_types_h
#endif


struct Client;

/*
 * Structures
 */
/** Represents a monitor.
 */
struct Monitor {
  struct Monitor *mo_next;     /**< Next monitor with queued data */
  struct SLink   *mo_monitor;  /**< Pointer to monitor list */
  char           *mo_nick;     /**< Nick */
};


/*
 * Macros
 */
/** Get next monitor. */
#define mo_next(mo)		((mo)->mo_next)
/** Get list monitor. */
#define mo_monitor(mo)		((mo)->mo_monitor)
/** Get nick. */
#define mo_nick(mo)		((mo)->mo_nick)


/*
 * Proto types
 */
extern void monitor_notify(struct Client *sptr, int raw);
extern int monitor_add_nick(struct Client *sptr, char *nick);
extern int monitor_del_nick(struct Client *sptr, char *nick);
extern int monitor_list_clean(struct Client *sptr);
extern void monitor_count_memory(int* count_out, size_t* bytes_out);

#endif /* INCLUDED_monitor_h */
