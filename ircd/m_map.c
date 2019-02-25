/*
 * IRC-Hispano IRC Daemon, ircd/m_map.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
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
 * @brief Handlers for MAP command.
 */
#include "config.h"

#include "client.h"
#include "ircd.h"
#include "ircd_defs.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_snprintf.h"
#include "ircd_string.h"
#include "list.h"
#include "match.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_user.h"
#include "s_serv.h"
#include "send.h"
#include "querycmds.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdio.h>
#include <string.h>

static void dump_map(struct Client *cptr, struct Client *server, char *mask, int prompt_length)
{
  const char *chr;
  static char prompt[64];
  struct DLink *lp;
  char *p = prompt + prompt_length;
  int cnt = 0;

  *p = '\0';
  if (prompt_length > 60)
    send_reply(cptr, RPL_MAPMORE, prompt, cli_name(server));
  else
  {
    char lag[512];
    char numeric[16];
    unsigned int users;
    unsigned int totalusers;
    unsigned int percentage;

    totalusers = UserStats.clients;
    if (!totalusers)
      totalusers = 1;

    users = (IsMe(server) ? UserStats.local_clients : cli_serv(server)->clients);
    percentage = (10000 * users) / totalusers;

    if (cli_serv(server)->lag>10000)
      lag[0]=0;
    else if (cli_serv(server)->lag<0)
      strcpy(lag,"(0s)");
    else
      sprintf(lag,"(%is)",cli_serv(server)->lag);
    sprintf(numeric, "%s:%d", NumServ(server), base64toint(NumServ(server)));
    if (IsBurst(server))
      chr = "*";
    else if (IsBurstAck(server))
      chr = "!";
    else
      chr = "";
    send_reply(cptr, RPL_MAP, prompt, chr, cli_name(server),
               numeric, lag, users, (users == 1) ? "" : "s",
               (percentage / 100), (percentage % 100));
  }
  if (prompt_length > 0)
  {
    p[-1] = ' ';
    if (p[-2] == '`')
      p[-2] = ' ';
  }
  if (prompt_length > 60)
    return;
  strcpy(p, "|-");
  for (lp = cli_serv(server)->down; lp; lp = lp->next)
    if (match(mask, cli_name(lp->value.cptr)))
      ClrFlag(lp->value.cptr, FLAG_MAP);
    else
    {
      SetFlag(lp->value.cptr, FLAG_MAP);
      cnt++;
    }
  for (lp = cli_serv(server)->down; lp; lp = lp->next)
  {
    if (!HasFlag(lp->value.cptr, FLAG_MAP))
      continue;
    if (--cnt == 0)
      *p = '`';
    dump_map(cptr, lp->value.cptr, mask, prompt_length + 2);
  }
  if (prompt_length > 0)
    p[-1] = '-';
}


/*
 * m_map - generic message handler
 * -- by Run
 *
 * parv[0] = sender prefix
 * parv[1] = server mask
 */
int m_map(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  if (feature_bool(FEAT_HIS_MAP) && !IsAnOper(sptr))
  {
    sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :%s %s", sptr,
                  "/MAP has been disabled.  "
                  "Visit ", feature_str(FEAT_HIS_URLSERVERS));
    return 0;
  }

  if (feature_bool(FEAT_HIS_SERVERS) && !IsAnOper(sptr)) {
    struct Client *acptr;
    unsigned int clients;
    unsigned int percentage;
    int numps = 0;

    /* Special MAP only show services */
    clients = UserStats.clients - UserStats.services;
    percentage = (10000 * clients) / UserStats.clients;

    send_reply(sptr, SND_EXPLICIT | RPL_MAP, ":%s [%u client%s - %u.%u%%]",
               cli_name(&his), clients, (clients == 1) ? "" : "s",
               percentage / 100, percentage % 100);

    for (acptr = GlobalClientList; acptr; acptr = cli_next(acptr))
    {
      if (!IsServer(acptr) && !IsMe(acptr))
        continue;
      if (!IsService(acptr))
        continue;

      numps++;
      clients = cli_serv(acptr)->clients;
      percentage = (10000 * clients) / UserStats.clients;

      if (numps < UserStats.pservers)
        send_reply(sptr, SND_EXPLICIT | RPL_MAP, ":|-%s [%u clients - %u.%u%%]",
                   cli_name(acptr), clients, (percentage / 100), (percentage % 100));
      else
        send_reply(sptr, SND_EXPLICIT | RPL_MAP, ":`-%s [%u clients - %u.%u%%]",
                   cli_name(acptr), clients, (percentage / 100), (percentage % 100));
    }
    send_reply(sptr, RPL_MAPEND);
    return 0;
  }

  if (parc < 2)
    parv[1] = "*";
  dump_map(sptr, &me, parv[1], 0);
  send_reply(sptr, RPL_MAPEND);

  return 0;
}

int mo_map(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  if (parc < 2)
    parv[1] = "*";

  dump_map(sptr, &me, parv[1], 0);
  send_reply(sptr, RPL_MAPEND);

  return 0;
}
