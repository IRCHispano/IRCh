/*
 * IRC-Hispano IRC Daemon, ircd/m_wallchops.c
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
 * @brief Handlers for WALLCHOPS command.
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
#include "s_user.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * m_wallchops - local generic message handler
 */
int m_wallchops(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Channel *chptr;

  assert(0 != cptr);
  assert(cptr == sptr);

  ClrFlag(sptr, FLAG_TS8);

  if (parc < 2 || EmptyString(parv[1]))
    return send_reply(sptr, ERR_NORECIPIENT, "WALLCHOPS");

  if (parc < 3 || EmptyString(parv[parc - 1]))
    return send_reply(sptr, ERR_NOTEXTTOSEND);

  if (IsChannelName(parv[1]) && (chptr = FindChannel(parv[1]))) {
    if (client_can_send_to_channel(sptr, chptr, 0)) {
      if ((chptr->mode.mode & MODE_NOPRIVMSGS) &&
          check_target_limit(sptr, chptr, chptr->chname, 0))
        return 0;
      RevealDelayedJoinIfNeeded(sptr, chptr);
      sendcmdto_channel_butone(sptr, CMD_WALLCHOPS, chptr, cptr,
			       SKIP_DEAF | SKIP_BURST | SKIP_NONOPS,
			       "%H :@ %s", chptr, parv[parc - 1]);
    }
    else
      send_reply(sptr, ERR_CANNOTSENDTOCHAN, parv[1]);
  }
  else
    send_reply(sptr, ERR_NOSUCHCHANNEL, parv[1]);

  return 0;
}

/*
 * ms_wallchops - server message handler
 */
int ms_wallchops(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Channel *chptr;
  assert(0 != cptr);
  assert(0 != sptr);

  if (parc < 3 || !IsUser(sptr))
    return 0;

  if (!IsLocalChannel(parv[1]) && (chptr = FindChannel(parv[1]))) {
    if (client_can_send_to_channel(sptr, chptr, 1)) {
      sendcmdto_channel_butone(sptr, CMD_WALLCHOPS, chptr, cptr,
			       SKIP_DEAF | SKIP_BURST | SKIP_NONOPS,
			       "%H :%s", chptr, parv[parc - 1]);
    } else
      send_reply(sptr, ERR_CANNOTSENDTOCHAN, parv[1]);
  }
  return 0;
}
