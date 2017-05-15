/*
 * IRC-Hispano IRC Daemon, ircd/m_asll.c
 *
 * Copyright (C) 1997-2017 IRC-Hispano Development Team <devel@irc-hispano.es>
 * Copyright (C) 2002 Alex Badea <vampire@p16.pub.ro>
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
 * @brief Handlers for ASLL command.
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
#include "match.h"
#include "msg.h"
#include "send.h"
#include "s_bsd.h"
#include "s_user.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdlib.h>

static int send_asll_reply(struct Client *from, struct Client *to, char *server,
			   int rtt, int up, int down)
{
  sendcmdto_one(from, CMD_NOTICE, to,
    (up || down) ? "%C :AsLL for %s -- RTT: %ims Upstream: %ims Downstream: %ims" :
    rtt ? "%C :AsLL for %s -- RTT: %ims [no asymm info]" :
    "%C :AsLL for %s -- [unknown]",
    to, server, rtt, up, down);
  return 0;
}

/*
 * ms_asll - server message handler
 */
int ms_asll(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char *mask;
  struct Client *acptr;
  int hits;
  int i;

  if (parc < 2)
    return need_more_params(sptr, "ASLL");

  if (parc > 5) {
    if (!(acptr = findNUser(parv[1])))
      return 0;
    if (MyUser(acptr))
      send_asll_reply(sptr, acptr, parv[2], atoi(parv[3]), atoi(parv[4]), atoi(parv[5]));
    else
      sendcmdto_prio_one(sptr, CMD_ASLL, acptr, "%C %s %s %s %s",
        acptr, parv[2], parv[3], parv[4], parv[5]);
    return 0;
  }

  if (hunt_server_prio_cmd(sptr, CMD_ASLL, cptr, 1, "%s %C", 2, parc, parv) != HUNTED_ISME)
    return 0;
  mask = parv[1];

  for (i = hits = 0; i <= HighestFd; i++) {
    acptr = LocalClientArray[i];
    if (!acptr || !IsServer(acptr) || !MyConnect(acptr) || match(mask, cli_name(acptr)))
      continue;
    sendcmdto_prio_one(&me, CMD_ASLL, sptr, "%C %s %i %i %i", sptr,
      cli_name(acptr), cli_serv(acptr)->asll_rtt,
      cli_serv(acptr)->asll_to, cli_serv(acptr)->asll_from);
    hits++;
  }
  sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :AsLL for %s: %d local servers matched", sptr, mask, hits);
  return 0;
}

/*
 * mo_asll - oper message handler
 */
int mo_asll(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char *mask;
  struct Client *acptr;
  int hits;
  int i;

  if (parc < 2)
    return need_more_params(sptr, "ASLL");

  if (parc == 2 && MyUser(sptr))
    parv[parc++] = cli_name(&me);

  if (hunt_server_prio_cmd(sptr, CMD_ASLL, cptr, 1, "%s %C", 2, parc, parv) != HUNTED_ISME)
    return 0;
  mask = parv[1];

  for (i = hits = 0; i <= HighestFd; i++) {
    acptr = LocalClientArray[i];
    if (!acptr || !IsServer(acptr) || !MyConnect(acptr) || match(mask, cli_name(acptr)))
      continue;
    send_asll_reply(&me, sptr, cli_name(acptr), cli_serv(acptr)->asll_rtt,
      cli_serv(acptr)->asll_to, cli_serv(acptr)->asll_from);
    hits++;
  }
  sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :AsLL for %s: %d local servers matched", sptr, mask, hits);
  return 0;
}
