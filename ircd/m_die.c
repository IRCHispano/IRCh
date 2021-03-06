/*
 * IRC-Hispano IRC Daemon, ircd/m_die.c
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
 * @brief Handlers for DIE command.
 */
#include "config.h"

#include "client.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_bsd.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */


/*
 * mo_die - oper message handler
 */
int mo_die(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;
  int i;

  if (!HasPriv(sptr, PRIV_DIE))
    return send_reply(sptr, ERR_NOPRIVILEGES);

  for (i = 0; i <= HighestFd; i++)
  {
    if (!(acptr = LocalClientArray[i]))
      continue;
    if (IsUser(acptr))
      sendcmdto_one(&me, CMD_NOTICE, acptr, "%C :Server Terminating. %s",
		    acptr, get_client_name(sptr, HIDE_IP));
    else if (IsServer(acptr))
      sendcmdto_one(&me, CMD_ERROR, acptr, ":Terminated by %s",
		    get_client_name(sptr, HIDE_IP));
  }
  server_die("received DIE");

  return 0;
}
