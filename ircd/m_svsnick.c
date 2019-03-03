/*
 * IRC-Hispano IRC Daemon, ircd/m_svsnick.c
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
 * @brief Handlers for SVSNICK command.
 */
#include "config.h"

#include "client.h"
#include "ddb.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_snprintf.h"
#include "ircd_string.h"
#include "ircd_tea.h"
#include "msg.h"
#include "numnicks.h"
#include "s_conf.h"
#include "s_user.h"
#include "send.h"
#include "sys.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdio.h>
#include <string.h>


/** Handle a SVSNICK command from a server.
 * See @ref m_functions for general discussion of parameters.
 *
 * \a parv[1] is a nick
 * \a parv[2] is a new nick (* random nick)
 *
 * @param[in] cptr Client that sent us the message.
 * @param[in] sptr Original source of message.
 * @param[in] parc Number of arguments.
 * @param[in] parv Argument vector.
 */
int ms_svsnick(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;
  struct Client *bcptr;
  char newnick[NICKLEN + 2];
  char *arg;
  int flags = 0;

  assert(0 != IsServer(cptr));

  if (parc == 2)
  {
    parv[2] = "*";
    parc = 3;
  }
  else if (parc < 2)
    return 0;

  if (!find_conf_byhost(cli_confs(cptr), cli_name(sptr), CONF_UWORLD))
  {
    sendcmdto_serv_butone(&me, CMD_DESYNCH, 0,
                   ":HACK(2): Fail SVSNICK for %s. From %C", parv[1], sptr);
    sendto_opmask_butone(0, SNO_HACK2,
                  "Fail SVSNICK for %s. From %C", parv[1], sptr);
    return 0;
  }

  acptr = findNUser(parv[1]);
  if (!acptr)
    acptr = FindUser(parv[1]);
  if (!acptr)
    return 0;

  if (!MyUser(acptr)) {
      sendcmdto_one(sptr, CMD_SVSNICK, cptr, "%s :%s", parv[1], parv[2]);
    return 0;
  }

  if (ircd_strcmp(parv[2], "*"))
  {
    /*
     * Don't let them send make us send back a really long string of
     * garbage
     */
    arg = parv[2];
    if (strlen(arg) > IRCD_MIN(NICKLEN, feature_int(FEAT_NICKLEN)))
      arg[IRCD_MIN(NICKLEN, feature_int(FEAT_NICKLEN))] = '\0';

    strcpy(newnick, arg);

    if (0 == do_nick_name(newnick))
      return 0;

    if (FindUser(newnick))
      return 0;
  }
  else
    strcpy(newnick, get_random_nick(acptr));

  sendto_opmask_butone(0, SNO_HACK4,
       "SVSNICK for %C, new nick %s. From %C", acptr, newnick, sptr);

  if (!MyUser(acptr))
    SetRenamed(flags);

  parv[0] = cli_name(acptr);
  parv[1] = newnick;
  {
     char tmp[100];
     ircd_snprintf(0, tmp, sizeof(tmp), "%T", CurrentTime);
     parv[2] = tmp;
  }

  return set_nick_name(acptr, acptr, newnick, 3, parv, flags);

}
