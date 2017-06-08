/*
 * IRC-Hispano IRC Daemon, ircd/m_kick.c
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
 * @brief Handlers for KICK command.
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
#include "ircd_features.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * m_kick - generic message handler
 *
 * parv[0] = sender prefix
 * parv[1] = channel
 * parv[2] = client to kick
 * parv[parc-1] = kick comment
 */
int m_kick(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  struct Client *who;
  struct Channel *chptr;
  struct Membership *member = 0;
  struct Membership* member2;
  char *name, *comment;

  ClrFlag(sptr, FLAG_TS8);

  if (parc < 3 || *parv[1] == '\0')
    return need_more_params(sptr, "KICK");

  name = parv[1];

  /* simple checks */
  if (!(chptr = get_channel(sptr, name, CGT_NO_CREATE)))
    return send_reply(sptr, ERR_NOSUCHCHANNEL, name);

  if (!(member2 = find_member_link(chptr, sptr)) || IsZombie(member2)
      || !IsChanOp(member2))
    return send_reply(sptr, ERR_CHANOPRIVSNEEDED, name);

  if (!(who = find_chasing(sptr, parv[2], 0)))
    return 0; /* find_chasing sends the reply for us */

  /* Don't allow the channel service to be kicked */
  if (IsChannelService(who))
    return send_reply(sptr, ERR_ISCHANSERVICE, cli_name(who), chptr->chname);

  /* Prevent kicking opers from local channels -DM- */
  if (IsLocalChannel(chptr->chname) && HasPriv(who, PRIV_DEOP_LCHAN))
    return send_reply(sptr, ERR_ISOPERLCHAN, cli_name(who), chptr->chname);

  /* check if kicked user is actually on the channel */
  if (!(member = find_member_link(chptr, who)) || IsZombie(member))
    return send_reply(sptr, ERR_USERNOTINCHANNEL, cli_name(who), chptr->chname);

  /* Don't allow to kick member with a higher op-level,
   * or members with the same op-level unless both are MAXOPLEVEL.
   */
  if (OpLevel(member) < OpLevel(member2)
      || (OpLevel(member) == OpLevel(member2)
          && OpLevel(member) < MAXOPLEVEL))
    return send_reply(sptr, ERR_NOTLOWEROPLEVEL, cli_name(who), chptr->chname,
	OpLevel(member2), OpLevel(member), "kick",
	OpLevel(member) == OpLevel(member2) ? "the same" : "a higher");

  /* We rely on ircd_snprintf to truncate the comment */
  comment = EmptyString(parv[parc - 1]) ? parv[0] : parv[parc - 1];

  if (!IsLocalChannel(name))
    sendcmdto_serv_butone(sptr, CMD_KICK, cptr, "%H %C :%s", chptr, who,
			  comment);

  if (IsDelayedJoin(member)) {
    /* If it's a delayed join, only send the KICK to the person doing
     * the kicking and the victim */
    if (MyUser(who))
      sendcmdto_one(sptr, CMD_KICK, who, "%H %C :%s", chptr, who, comment);
    if (CapActive(sptr, CAP_EXTJOIN))
      sendcmdto_one(who, CMD_JOIN, sptr, "%H %s :%s", chptr, IsAccount(who) ? cli_account(who) : "*",  cli_info(who));
    else
      sendcmdto_one(who, CMD_JOIN, sptr, "%H", chptr);
    sendcmdto_one(sptr, CMD_KICK, sptr, "%H %C :%s", chptr, who, comment);
    CheckDelayedJoins(chptr);
  } else
    sendcmdto_channel_butserv_butone(sptr, CMD_KICK, chptr, NULL, 0, "%H %C :%s", chptr, who,
                                     comment);

  make_zombie(member, who, cptr, sptr, chptr);

  return 0;
}

/*
 * ms_kick - server message handler
 *
 * parv[0] = sender prefix
 * parv[1] = channel
 * parv[2] = client to kick
 * parv[parc-1] = kick comment
 */
int ms_kick(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  struct Client *who;
  struct Channel *chptr;
  struct Membership *member = 0, *sptr_link = 0;
  char *name, *comment;

  ClrFlag(sptr, FLAG_TS8);

  if (parc < 3 || *parv[1] == '\0')
    return need_more_params(sptr, "KICK");

  name = parv[1];
  comment = parv[parc - 1];

  /* figure out who gets kicked from what */
  if (IsLocalChannel(name) ||
      !(chptr = get_channel(sptr, name, CGT_NO_CREATE)) ||
      !(who = findNUser(parv[2])))
    return 0;

  /* We go ahead and pass on the KICK for users not on the channel */
  member = find_member_link(chptr, who);
  if (member && IsZombie(member))
  {
    /* We might get a KICK from a zombie's own server because the user
     * net-rode during a burst (which always generates a KICK) *and*
     * was kicked via another server.  In that case, we must remove
     * the user from the channel.
     */
    if (sptr == cli_user(who)->server)
    {
      remove_user_from_channel(who, chptr);
    }
    /* Otherwise, we treat zombies like they are not channel members. */
    member = 0;
  }

  /* Send HACK notice, but not for servers in BURST */
  /* 2002-10-17: Don't send HACK if the users local server is kicking them */
  if (IsServer(sptr) &&
      !IsBurstOrBurstAck(sptr) &&
      sptr!=cli_user(who)->server)
    sendto_opmask_butone(0, SNO_HACK4, "HACK: %C KICK %H %C %s", sptr, chptr,
			 who, comment);

  /* Unless someone accepted it downstream (or the user isn't on the channel
   * here), if kicker is not on channel, or if kicker is not a channel
   * operator, bounce the kick
   */
  if (!IsServer(sptr) && member && cli_from(who) != cptr &&
      (!(sptr_link = find_member_link(chptr, sptr)) || !IsChanOp(sptr_link))) {
    sendto_opmask_butone(0, SNO_HACK2, "HACK: %C KICK %H %C %s", sptr, chptr,
			 who, comment);

    sendcmdto_one(who, CMD_JOIN, cptr, "%H", chptr);

    /* Reop/revoice member */
    if (IsChanOp(member) || HasVoice(member)) {
      struct ModeBuf mbuf;

      modebuf_init(&mbuf, sptr, cptr, chptr,
		   (MODEBUF_DEST_SERVER |  /* Send mode to a server */
		    MODEBUF_DEST_DEOP   |  /* Deop the source */
		    MODEBUF_DEST_BOUNCE)); /* And bounce the MODE */

      if (IsChanOp(member))
	modebuf_mode_client(&mbuf, MODE_DEL | MODE_CHANOP, who, OpLevel(member));
      if (HasVoice(member))
	modebuf_mode_client(&mbuf, MODE_DEL | MODE_VOICE, who, MAXOPLEVEL + 1);

      modebuf_flush(&mbuf);
    }
  } else {
    /* Propagate kick... */
    sendcmdto_serv_butone(sptr, CMD_KICK, cptr, "%H %C :%s", chptr, who,
			  comment);

    if (member) { /* and tell the channel about it */
      if (IsDelayedJoin(member)) {
        if (MyUser(who))
          sendcmdto_one(IsServer(sptr) ? &his : sptr, CMD_KICK,
                        who, "%H %C :%s", chptr, who, comment);
      } else {
        sendcmdto_channel_butserv_butone(IsServer(sptr) ? &his : sptr, CMD_KICK,
                                         chptr, NULL, 0, "%H %C :%s", chptr, who,
                                         comment);
      }

      make_zombie(member, who, cptr, sptr, chptr);
    }
  }

  return 0;
}
