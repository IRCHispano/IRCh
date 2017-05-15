/*
 * IRC-Hispano IRC Daemon, ircd/m_wallops.c
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
/** @file
 * @brief Handlers for WALLOPS command.
 */
#include "config.h"

#include "client.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */


/*
 * ms_wallops - server message handler
 */
int ms_wallops(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char *message;

  message = parc > 1 ? parv[1] : 0;

  if (EmptyString(message))
    return need_more_params(sptr, "WALLOPS");

  sendwallto_group_butone(sptr, WALL_WALLOPS, cptr, "%s", message);
  return 0;
}

/*
 * mo_wallops - oper message handler
 */
int mo_wallops(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char *message;

  message = parc > 1 ? parv[1] : 0;

  if (EmptyString(message))
    return need_more_params(sptr, "WALLOPS");

  sendwallto_group_butone(sptr, WALL_WALLOPS, 0, "%s", message);
  return 0;
}
