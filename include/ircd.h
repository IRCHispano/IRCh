/*
 * IRC-Hispano IRC Daemon, include/ircd.h
 *
 * Copyright (C) 1997-2017 IRC-Hispano Development Team <devel@irc-hispano.es>
 * Copyright (C) 1990 Jarkko Oikarinen
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
/** @file ircd.h
 * @brief Global data for the daemon.
 */
#ifndef INCLUDED_ircd_h
#define INCLUDED_ircd_h

#ifndef INCLUDED_struct_h
#include "struct.h"           /* struct Client */
#endif
#ifndef INCLUDED_sys_types_h
#include <sys/types.h>        /* size_t, time_t */
#endif

/** Describes status for a daemon. */
struct Daemon
{
  int          argc;        /**< Number of command-line arguments. */
  char**       argv;        /**< Array of command-line arguments. */
  pid_t        pid;         /**< %Daemon's process id. */
  uid_t        uid;         /**< %Daemon's user id. */
  uid_t        euid;        /**< %Daemon's effective user id. */
  unsigned int bootopt;     /**< Boot option flags. */
  int          pid_fd;      /**< File descriptor for process id file. */
};

/*
 * Macros
 */
#define TStime() (CurrentTime + TSoffset) /**< Current network time*/
#define OLDEST_TS 780000000	/**< Any TS older than this is bogus */
#define BadPtr(x) (!(x) || (*(x) == '\0')) /**< Is \a x a bad string? */

/* Miscellaneous defines */

#define UDP_PORT        "7007"  /**< Default port for server-to-server pings. */
#define MINOR_PROTOCOL  "09"    /**< Minimum protocol version supported. */
#define MAJOR_PROTOCOL  "10"    /**< Current protocol version. */
#define BASE_VERSION    "u2.10" /**< Base name of IRC daemon version. */

/*
 * Proto types
 */
extern void server_die(const char* message);
extern void server_panic(const char* message);
extern void server_restart(const char* message);

extern struct Client  me;
extern time_t         CurrentTime;
extern struct Client* GlobalClientList;
extern time_t         TSoffset;
extern int            GlobalRehashFlag;      /* 1 if SIGHUP is received */
extern int            GlobalRestartFlag;     /* 1 if SIGINT is received */
extern char*          configfile;
extern int            debuglevel;
extern char*          debugmode;
extern int	      running;

#endif /* INCLUDED_ircd_h */

