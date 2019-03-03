/*
 * IRC-Hispano IRC Daemon, ircd/m_svspart.c
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
 * @brief Handlers for SVSPART command.
 */
#include "config.h"

#include "channel.h"
#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
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


/** Handle a SVSPART command from a server.
 * See @ref m_functions for general discussion of parameters.
 *
 * \a parv has the following elements when \a sptr is a server:
 * \li parv[1] is a nick
 * \li \a parv[2] is a comma-separated list of channel names
 * \li \a parv[\a parc - 1] is the parting comment
 *
 * @param[in] cptr Client that sent us the message.
 * @param[in] sptr Original source of message.
 * @param[in] parc Number of arguments.
 * @param[in] parv Argument vector.
 */
int ms_svspart(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Channel *chptr;
  struct Membership *member;
  struct JoinBuf parts;
  struct Client *acptr;
  unsigned int flags = 0;
  char *p = 0;
  char *name;

  assert(0 != IsServer(cptr));

  if (parc < 3)
    return 0;

  if (!find_conf_byhost(cli_confs(cptr), cli_name(sptr), CONF_UWORLD))
  {
    sendcmdto_serv_butone(&me, CMD_DESYNCH, 0,
                   ":HACK(2): Fail SVSPART for %s. From %s",
                   parv[1], cli_name(sptr));
    sendto_opmask_butone(0, SNO_HACK2,
                  "Fail SVSPART for %s. From %C", parv[1], sptr);
    return 0;
  }

  acptr = findNUser(parv[1]);
  if (!acptr)
    acptr = FindUser(parv[1]);
  if (!acptr)
    return 0;

  if (!MyUser(acptr)) {
    if (parc > 3)
      sendcmdto_one(acptr, CMD_SVSPART, sptr, "%s %s :%s", parv[1], parv[2], parv[3]);
    else
      sendcmdto_one(acptr, CMD_SVSPART, sptr, "%s %s", parv[1], parv[2]);
    return 0;
  }

  sendcmdto_serv_butone(&me, CMD_DESYNCH, 0,
                 ":HACK(4): SVSPART for %s, channels %s. From %s",
                 cli_name(acptr), parv[2], cli_name(sptr));
  sendto_opmask_butone(0, SNO_HACK4,
       "SVSPART for %C, channels %s. From %C", acptr, parv[2], sptr);

  /* init join/part buffer */
  joinbuf_init(&parts, acptr, &me, JOINBUF_TYPE_PART,
         (parc > 3 && !EmptyString(parv[parc - 1])) ? parv[parc - 1] : 0,
         0);

  /* scan through channel list */
  for (name = ircd_strtok(&p, parv[2], ","); name;
       name = ircd_strtok(&p, 0, ",")) {

    chptr = get_channel(acptr, name, CGT_NO_CREATE); /* look up channel */

    if (!chptr) { /* complain if there isn't such a channel */
      send_reply(acptr, ERR_NOSUCHCHANNEL, name);
      continue;
    }

    if (!(member = find_member_link(chptr, acptr))) { /* complain if not on */
      send_reply(acptr, ERR_NOTONCHANNEL, chptr->chname);
      continue;
    }

    assert(!IsZombie(member)); /* Local users should never zombie */

    if (!member_can_send_to_channel(member, 0))
    {
      flags |= CHFL_BANNED;
      /* Remote clients don't want to see a comment either. */
      parts.jb_comment = 0;
    }

    if (IsDelayedJoin(member))
      flags |= CHFL_DELAYED;

    joinbuf_join(&parts, chptr, flags); /* part client from channel */

  }

  return joinbuf_flush(&parts); /* flush channel parts */
}
