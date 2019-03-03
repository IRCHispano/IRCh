/*
 * IRC-Hispano IRC Daemon, ircd/m_svsmode.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2008 Toni Garcia (zoltan) <toni@tonigarcia.es>
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
 * @brief Handlers for SVSMODE command.
 */
#include "config.h"

#include "channel.h"
#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_snprintf.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_conf.h"
#include "s_user.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdio.h>
#include <string.h>


/** Handle a SVSMODE command from a server.
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
int ms_svsmode(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;

  assert(0 != IsServer(cptr));

  if (parc < 3)
    return 0;

  /* Not support for channels */
  if (IsChannelName(parv[1]))
    return 0;

  if (!find_conf_byhost(cli_confs(cptr), cli_name(sptr), CONF_UWORLD))
  {
    sendcmdto_serv_butone(&me, CMD_DESYNCH, 0,
                   ":HACK(2): Fail SVSMODE for %s. From %s",
                   parv[1], cli_name(sptr));
    sendto_opmask_butone(0, SNO_HACK2,
                  "Fail SVSMODE for %s. From %C", parv[1], sptr);
    return 0;
  }

  acptr = findNUser(parv[1]);
  if (!acptr)
    acptr = FindUser(parv[1]);
  if (!acptr)
    return 0;

  if (!MyUser(acptr)) {
    sendcmdto_one(sptr, CMD_SVSMODE, cptr, "%s %s", parv[1], parv[2]);
    return 0;
  }

  sendcmdto_serv_butone(&me, CMD_DESYNCH, 0,
                 ":HACK(4): SVSMODE for %s, mode %s. From %s",
                 cli_name(acptr), parv[2], cli_name(sptr));
  sendto_opmask_butone(0, SNO_HACK4,
                "SVSMODE for %C, mode %s. From %C", acptr, parv[2], sptr);


  parv[0] = cli_name(acptr);
  parv[1] = cli_name(acptr);

  set_user_mode(acptr, acptr, 3, parv, ALLOWMODES_ANY | ALLOWMODES_SVSMODE);

  return 0;
}
