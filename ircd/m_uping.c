/*
 * IRC-Hispano IRC Daemon, ircd/m_uping.c
 *
 * Copyright (C) 1997-2017 IRC-Hispano Development Team <devel@irc-hispano.es>
 * Copyright (C) 1996 Carlo Wood <carlo@runaway.xs4all.nl>
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
 * @brief Handlers for the UPING command.
 */
#include "config.h"

#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "match.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_conf.h"
#include "s_user.h"
#include "send.h"
#include "uping.h"


/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdlib.h>
#include <string.h>

/*
 * ms_uping - server message handler
 *
 * m_uping  -- by Run
 *
 * parv[0] = sender prefix
 * parv[1] = pinged server
 * parv[2] = port
 * parv[3] = hunted server
 * parv[4] = number of requested pings
 */
int ms_uping(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct ConfItem *aconf;
  int port;
  int count;

  assert(0 != cptr);
  assert(0 != sptr);

  if (!IsAnOper(sptr)) {
    send_reply(sptr, ERR_NOPRIVILEGES);
    return 0;
  }

  if (parc < 5) {
    send_reply(sptr, ERR_NEEDMOREPARAMS, "UPING");
    return 0;
  }

  if (hunt_server_cmd(sptr, CMD_UPING, cptr, 1, "%s %s %C %s", 3, parc, parv)
      != HUNTED_ISME)
    return 0;
  /*
   * Determine port: First user supplied, then default : 7007
   */
  if (EmptyString(parv[2]) || (port = atoi(parv[2])) <= 0)
    port = atoi(UDP_PORT);

  if (EmptyString(parv[4]) || (count = atoi(parv[4])) <= 0)
  {
    sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :UPING : Illegal number of "
		  "packets: %s", sptr, parv[4]);
    return 0;
  }
  /* 
   * Check if a CONNECT would be possible at all (adapted from m_connect)
   */
  if ((aconf = conf_find_server(parv[1])))
    uping_server(sptr, aconf, port, count);
  else
    sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :UPING: Host %s not listed in "
		  "ircd.conf", sptr, parv[1]);

  return 0;
}

/*
 * mo_uping - oper message handler
 *
 * m_uping  -- by Run
 *
 * parv[0] = sender prefix
 * parv[1] = pinged server
 * parv[2] = port
 * parv[3] = hunted server
 * parv[4] = number of requested pings
 */
int mo_uping(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct ConfItem *aconf;
  int port;
  int count;

  assert(0 != cptr);
  assert(0 != sptr);
  assert(cptr == sptr);

  assert(IsAnOper(sptr));

  if (parc < 2) {
    send_reply(sptr, ERR_NEEDMOREPARAMS, "UPING");
    return 0;
  }

  if (parc == 2) {
    parv[parc++] = UDP_PORT;
    parv[parc++] = cli_name(&me);
    parv[parc++] = "5";
  }
  else if (parc == 3) {
    if (IsDigit(*parv[2]))
      parv[parc++] = cli_name(&me);
    else {
      parv[parc++] = parv[2];
      parv[2] = UDP_PORT;
    }
    parv[parc++] = "5";
  }
  else if (parc == 4) {
    if (IsDigit(*parv[2])) {
      if (IsDigit(*parv[3])) {
        parv[parc++] = parv[3];
        parv[3] = cli_name(&me);
      }
      else
        parv[parc++] = "5";
    }
    else {
      parv[parc++] = parv[3];
      parv[3] = parv[2];
      parv[2] = UDP_PORT;
    }
  }
  if (hunt_server_cmd(sptr, CMD_UPING, sptr, 1, "%s %s %C %s", 3, parc, parv)
      != HUNTED_ISME)
    return 0;
  /*
   * Determine port: First user supplied, then default : 7007
   */
  if (EmptyString(parv[2]) || (port = atoi(parv[2])) <= 0)
    port = atoi(UDP_PORT);

  if (EmptyString(parv[4]) || (count = atoi(parv[4])) <= 0)
  {
    sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :UPING: Illegal number of "
		  "packets: %s", sptr, parv[4]);
    return 0;
  }
  /* 
   * Check if a CONNECT would be possible at all (adapted from m_connect)
   */
  if ((aconf = conf_find_server(parv[1])))
    uping_server(sptr, aconf, port, count);
  else {
    sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :UPING: Host %s not listed in "
		  "ircd.conf", sptr, parv[1]);
  }
  return 0;
}
