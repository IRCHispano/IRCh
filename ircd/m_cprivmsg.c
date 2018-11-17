/*
 * IRC-Hispano IRC Daemon, ircd/m_cprivmsg.c
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
/** @file
 * @brief Handlers for CPRIVMSG command.
 */
#include "config.h"

#include "client.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "s_user.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * m_cprivmsg - generic message handler
 *
 * parv[0] = sender prefix
 * parv[1] = nick
 * parv[2] = #channel
 * parv[3] = Private message text
 */
int m_cprivmsg(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  assert(0 != cptr);
  assert(cptr == sptr);

  if (parc < 4 || EmptyString(parv[3]))
    return need_more_params(sptr, "CPRIVMSG");

  return whisper(sptr, parv[1], parv[2], parv[3], 0);
}

/*
 * m_cnotice - generic message handler
 *
 * parv[0] = sender prefix
 * parv[1] = nick
 * parv[2] = #channel
 * parv[3] = Private message text
 */
int m_cnotice(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  assert(0 != cptr);
  assert(cptr == sptr);

  if (parc < 4 || EmptyString(parv[3]))
    return need_more_params(sptr, "CNOTICE");

  return whisper(sptr, parv[1], parv[2], parv[3], 1);
}


