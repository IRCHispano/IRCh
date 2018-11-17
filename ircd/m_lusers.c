/*
 * IRC-Hispano IRC Daemon, ircd/m_lusers.c
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
 * @brief Handlers for LUSERS command.
 */
#include "config.h"

#include "client.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "querycmds.h"
#include "s_user.h"
#include "s_serv.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * m_lusers - generic message handler
 *
 * parv[0] = sender
 * parv[1] = ignored
 * parv[2] = server to query
 */
int m_lusers(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  int longoutput = MyUser(sptr) || IsOper(sptr);
  if (parc > 2)
    if (hunt_server_cmd(sptr, CMD_LUSERS, cptr, feature_int(FEAT_HIS_REMOTE),
                        "%s :%C", 2, parc, parv) != HUNTED_ISME)
      return 0;

  assert(UserStats.inv_clients <= UserStats.clients + UserStats.unknowns);

  send_reply(sptr, RPL_LUSERCLIENT, 
	     UserStats.clients - UserStats.inv_clients + UserStats.unknowns,
	     UserStats.inv_clients, UserStats.servers);
  if (longoutput && UserStats.opers)
    send_reply(sptr, RPL_LUSEROP, UserStats.opers);
  if (UserStats.unknowns > 0)
    send_reply(sptr, RPL_LUSERUNKNOWN, UserStats.unknowns);
  if (longoutput && UserStats.channels > 0)
    send_reply(sptr, RPL_LUSERCHANNELS, UserStats.channels);
  send_reply(sptr, RPL_LUSERME, UserStats.local_clients,
	     UserStats.local_servers);

  sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :Highest connection count: "
		"%d (%d clients)", sptr, max_connection_count,
		max_client_count);

  return 0;
}

/*
 * ms_lusers - server message handler
 *
 * parv[0] = sender
 * parv[1] = ignored
 * parv[2] = server to query
 */
int ms_lusers(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  int longoutput = MyUser(sptr) || IsOper(sptr);
  if (parc > 2)
    if (hunt_server_cmd(sptr, CMD_LUSERS, cptr, 0, "%s :%C", 2, parc, parv) !=
        HUNTED_ISME)
      return 0;

  send_reply(sptr, RPL_LUSERCLIENT, UserStats.clients - UserStats.inv_clients,
	     UserStats.inv_clients, UserStats.servers);
  if (longoutput && UserStats.opers)
    send_reply(sptr, RPL_LUSEROP, UserStats.opers);
  if (UserStats.unknowns > 0)
    send_reply(sptr, RPL_LUSERUNKNOWN, UserStats.unknowns);
  if (longoutput && UserStats.channels > 0)
    send_reply(sptr, RPL_LUSERCHANNELS, UserStats.channels);
  send_reply(sptr, RPL_LUSERME, UserStats.local_clients,
	     UserStats.local_servers);

  sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :Highest connection count: "
		"%d (%d clients)", sptr, max_connection_count,
		max_client_count);

  return 0;
}
