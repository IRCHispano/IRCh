/*
 * IRC-Hispano IRC Daemon, ircd/m_endburst.c
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
 * @brief Handlers for END OF BURST command.
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
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * ms_end_of_burst - server message handler
 * - Added Xorath 6-14-96, rewritten by Run 24-7-96
 * - and fixed by record and Kev 8/1/96
 * - and really fixed by Run 15/8/96 :p
 * This the last message in a net.burst.
 * It clears a flag for the server sending the burst.
 *
 * As of 10.11, to fix a bug in the way BURST is processed, it also
 * makes sure empty channels are deleted
 *
 * parv[0] - sender prefix
 */
int ms_end_of_burst(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Channel *chan, *next_chan;

  assert(0 != cptr);
  assert(0 != sptr);

  sendto_opmask_butone(0, SNO_NETWORK, "Completed net.burst from %C.", 
  	sptr);
  sendcmdto_serv_butone(sptr, CMD_END_OF_BURST, cptr, "");
  ClearBurst(sptr);
  SetBurstAck(sptr);
  if (MyConnect(sptr))
    sendcmdto_one(&me, CMD_END_OF_BURST_ACK, sptr, "");

  /* Count through channels... */
  for (chan = GlobalChannelList; chan; chan = next_chan) {
    next_chan = chan->next;
    if (!chan->members && (chan->mode.mode & MODE_BURSTADDED)) {
      /* Newly empty channel, schedule it for removal. */
      chan->mode.mode &= ~MODE_BURSTADDED;
      sub1_from_channel(chan);
   } else
      chan->mode.mode &= ~MODE_BURSTADDED;
  }

  return 0;
}

/*
 * ms_end_of_burst_ack - server message handler
 *
 * This the acknowledge message of the `END_OF_BURST' message.
 * It clears a flag for the server receiving the burst.
 *
 * parv[0] - sender prefix
 */
int ms_end_of_burst_ack(struct Client *cptr, struct Client *sptr, int parc, char **parv)
{
  if (!IsServer(sptr))
    return 0;

  sendto_opmask_butone(0, SNO_NETWORK, "%C acknowledged end of net.burst.",
		       sptr);
  sendcmdto_serv_butone(sptr, CMD_END_OF_BURST_ACK, cptr, "");
  ClearBurstAck(sptr);

  return 0;
}
