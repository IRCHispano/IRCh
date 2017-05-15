/*
 * IRC-Hispano IRC Daemon, ircd/m_tmpl.c
 *
 * Copyright (C) 1997-2017 IRC-Hispano Development Team <devel@irc-hispano.es>
 * Copyright (C) 2000 Kevin L. Mitchell <klmitch@mit.edu>
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
 * @brief Handlers for the TMPL command.
 */
#include "config.h"

#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "numeric.h"
#include "numnicks.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * m_tmpl - generic message handler
 */
int m_tmpl(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  return 0;
}

/*
 * ms_tmpl - server message handler
 */
int ms_tmpl(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  return 0;
}

/*
 * mo_tmpl - oper message handler
 */
int mo_tmpl(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  return 0;
}

  
/*
 * mv_tmpl - service message handler
 */
int mv_tmpl(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  return 0;
}

