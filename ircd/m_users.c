/*
 * IRC-Hispano IRC Daemon, ircd/m_users.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2004 Toni Garcia (zoltan) <toni@tonigarcia.es>
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
 * @brief Handlers for USERS command.
 */
#include "config.h"

#include "client.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "msg.h"
#include "numeric.h"
#include "querycmds.h"
#include "s_misc.h"
#include "s_user.h"
#include "s_serv.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/** Handle a USERS message from a local connection.
 *
 * \a parv may either be empty or have the following elements:
 * \li \a parv[1] is ignored
 * \li \a parv[2] is the server to query
 *
 * See @ref m_functions for discussion of the arguments.
 * @param[in] cptr Client that sent us the message.
 * @param[in] sptr Original source of message.
 * @param[in] parc Number of arguments.
 * @param[in] parv Argument vector./*
 */
int m_users(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  if (parc > 2)
    if (hunt_server_cmd(sptr, CMD_USERS, cptr, feature_int(FEAT_HIS_REMOTE),
			"%s :%C", 2, parc, parv) != HUNTED_ISME)
      return 0;

  if (feature_bool(FEAT_HIS_SERVERS) && !IsAnOper(sptr)) {
    send_reply(sptr, SND_EXPLICIT | RPL_CURRENT_LOCAL,
               ":Current local users: %d Max: %d",
               UserStats.clients - UserStats.services, max_global_count - UserStats.services);
    send_reply(sptr, SND_EXPLICIT | RPL_CURRENT_GLOBAL,
               ":Current global users: %d Max: %d",
               UserStats.clients, max_global_count);
  } else {
    send_reply(sptr, RPL_CURRENT_LOCAL, UserStats.local_clients, max_client_count,
               myctime(max_client_count_TS), myctime(cli_since(&me)));

    send_reply(sptr, RPL_CURRENT_GLOBAL, UserStats.clients, max_global_count,
               myctime(max_global_count_TS), myctime(cli_since(&me)));
  }

  return 0;
}

/** Handle a USERS message from a server connection.
 *
 * \a parv has the following elements:
 * \li \a parv[1] is ignored
 * \li \a parv[2] is the server to query
 *
 * See @ref m_functions for discussion of the arguments.
 * @param[in] cptr Client that sent us the message.
 * @param[in] sptr Original source of message.
 * @param[in] parc Number of arguments.
 * @param[in] parv Argument vector./*
 */
int ms_users(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  if (parc > 2)
    if (hunt_server_cmd(sptr, CMD_USERS, cptr, 0, "%s :%C", 2, parc, parv) !=
        HUNTED_ISME)
      return 0;

  if (feature_bool(FEAT_HIS_SERVERS) && !IsAnOper(sptr)) {
    send_reply(sptr, SND_EXPLICIT | RPL_CURRENT_LOCAL,
               ":Current local users: %d Max: %d",
               UserStats.clients - UserStats.services, max_global_count - UserStats.services);
    send_reply(sptr, SND_EXPLICIT | RPL_CURRENT_GLOBAL,
               ":Current global users: %d Max: %d",
               UserStats.clients, max_global_count);
  } else {
    send_reply(sptr, RPL_CURRENT_LOCAL, UserStats.local_clients, max_client_count,
               myctime(max_client_count_TS), myctime(cli_since(&me)));
    send_reply(sptr, RPL_CURRENT_GLOBAL, UserStats.clients, max_global_count,
               myctime(max_global_count_TS), myctime(cli_since(&me)));
  }

  return 0;
}
