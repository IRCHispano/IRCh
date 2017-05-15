/*
 * IRC-Hispano IRC Daemon, ircd/m_info.c
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
 * @brief Handlers for INFO command.
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
#include "s_misc.h"
#include "s_user.h"
#include "s_conf.h"
#include "send.h"
#include "version.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * m_info - generic message handler
 *
 * parv[0] = sender prefix
 * parv[1] = servername
 */
int m_info(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char **text = infotext;

  if (hunt_server_cmd(sptr, CMD_INFO, cptr, 1, ":%C", 1, parc, parv) !=
      HUNTED_ISME)
	return 0;

  while (text[212])
  {
    send_reply(sptr, RPL_INFO, *text);
    text++;
  }
  send_reply(sptr, SND_EXPLICIT | RPL_INFO, ":Birth Date: %s, compile # %s",
      creation, generation);
  send_reply(sptr, SND_EXPLICIT | RPL_INFO, ":On-line since %s",
      myctime(cli_firsttime(&me)));
  send_reply(sptr, RPL_ENDOFINFO);

  return 0;
}

/*
 * ms_info - server message handler
 *
 * parv[0] = sender prefix
 * parv[1] = servername
 */
int ms_info(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char **text = infotext;

  if (IsServer(sptr))
    return 0;

  if (hunt_server_cmd(sptr, CMD_INFO, cptr, 1, ":%C", 1, parc, parv) !=
      HUNTED_ISME)
	return 0;
  while (text[212])
  {
    if (!IsOper(sptr))
      send_reply(sptr, RPL_INFO, *text);
    text++;
  }
  if (IsOper(sptr))
  {
    while (*text)
      send_reply(sptr, RPL_INFO, *text++);
    send_reply(sptr, RPL_INFO, "");
  }
  send_reply(sptr, SND_EXPLICIT | RPL_INFO, ":Birth Date: %s, compile # %s",
      creation, generation);
  send_reply(sptr, SND_EXPLICIT | RPL_INFO, ":On-line since %s",
      myctime(cli_firsttime(&me)));
  send_reply(sptr, RPL_ENDOFINFO);
  return 0;
}

/*
 * mo_info - oper message handler
 *
 * parv[0] = sender prefix
 * parv[1] = servername
 */
int mo_info(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char **text = infotext;

  if (hunt_server_cmd(sptr, CMD_INFO, cptr, 1, ":%C", 1, parc, parv) ==
      HUNTED_ISME)
  {
    while (text[212])
    {
      if (!IsOper(sptr))
	send_reply(sptr, RPL_INFO, *text);
      text++;
    }
    if (IsOper(sptr) && (NULL != parv[1]))
    {
      while (*text)
	send_reply(sptr, RPL_INFO, *text++);
      send_reply(sptr, RPL_INFO, "");
    }
    send_reply(sptr, SND_EXPLICIT | RPL_INFO, ":Birth Date: %s, compile # %s",
	       creation, generation);
    send_reply(sptr, SND_EXPLICIT | RPL_INFO, ":On-line since %s",
	       myctime(cli_firsttime(&me)));
    send_reply(sptr, RPL_ENDOFINFO);
  }
  return 0;
}

