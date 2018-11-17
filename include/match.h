/*
 * IRC-Hispano IRC Daemon, include/match.h
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
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
/** @file match.h
 * @brief Interface for matching strings to IRC masks.
 */
#ifndef INCLUDED_match_h
#define INCLUDED_match_h

#ifndef INCLUDED_sys_types_h
#include <sys/types.h>         /* XXX - broken BSD system headers */
#define INCLUDED_sys_types_h
#endif
#ifndef INCLUDED_res_h
#include "res.h"
#endif

/*
 * Prototypes
 */

/*
 * XXX - match returns 0 if a match is found. Smelly interface
 * needs to be fixed. --Bleep
 */
extern int mmatch(const char *old_mask, const char *new_mask);
extern int match(const char *ma, const char *na);
extern char *collapse(char *pattern);

extern int matchcomp(char *cmask, int *minlen, int *charset, const char *mask);
extern int matchexec(const char *string, const char *cmask, int minlen);
extern int matchdecomp(char *mask, const char *cmask);
extern int mmexec(const char *wcm, int wminlen, const char *rcm, int rminlen);

extern int ipmask_check(const struct irc_in_addr *addr, const struct irc_in_addr *mask, unsigned char bits);

#endif /* INCLUDED_match_h */
