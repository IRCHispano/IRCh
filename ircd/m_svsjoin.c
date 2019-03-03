/*
 * IRC-Hispano IRC Daemon, ircd/m_svsjoin.c
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
 * @brief Handlers for SVSJOIN command.
 */
#include "config.h"

#include "channel.h"
#include "client.h"
#include "ddb.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_conf.h"
#include "s_user.h"
#include "send.h"
#include "sys.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdio.h>
#include <string.h>


/** Handle a SVSJOIN command from a server.
 * See @ref m_functions for general discussion of parameters.
 *
 * \a parv has the following elements when \a sptr is a server:
 * \li parv[1] is a nick
 * \li \a parv[2] is a comma-separated list of channel names
 *
 * @param[in] cptr Client that sent us the message.
 * @param[in] sptr Original source of message.
 * @param[in] parc Number of arguments.
 * @param[in] parv Argument vector.
 */
int ms_svsjoin(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Channel *chptr;
  struct JoinBuf join;
  struct JoinBuf create;
  struct Client *acptr;
  char *p = 0;
  char *chanlist;
  char *name;

  assert(0 != IsServer(cptr));

  if (parc < 3)
    return 0;

  if (!find_conf_byhost(cli_confs(cptr), cli_name(sptr), CONF_UWORLD)) {
    sendcmdto_serv_butone(&me, CMD_DESYNCH, 0,
                   ":HACK(2): Fail SVSJOIN for %s. From %C", parv[1], sptr);
    sendto_opmask_butone(0, SNO_HACK2,
                  "Fail SVSJOIN for %s. From %C", parv[1], sptr);
    return 0;
  }

  acptr = findNUser(parv[1]);
  if (!acptr)
    acptr = FindUser(parv[1]);
  if (!acptr)
    return 0;

  if (!MyUser(acptr)) {
    if (parc > 3)
      sendcmdto_one(acptr, CMD_SVSJOIN, sptr, "%s %s %s", parv[1], parv[2], parv[3]);
    else
      sendcmdto_one(acptr, CMD_SVSJOIN, sptr, "%s %s", parv[1], parv[2]);
    return 0;
  }

  sendto_opmask_butone(0, SNO_HACK4,
       "SVSJOIN for %C, channels %s. From %C", acptr, parv[2], sptr);

  joinbuf_init(&join, acptr, &me, JOINBUF_TYPE_JOIN, 0, 0);
  joinbuf_init(&create, acptr, &me, JOINBUF_TYPE_CREATE, 0, TStime());

  chanlist = last0(&me, acptr, parv[2]); /* find last "JOIN 0" */

  for (name = ircd_strtok(&p, chanlist, ","); name;
       name = ircd_strtok(&p, 0, ",")) {

    if (!IsChannelName(name) || !strIsIrcCh(name))
    {
      /* bad channel name */
      continue;
    }

    if (!(chptr = FindChannel(name))) {
      if (((name[0] == '&') && !feature_bool(FEAT_LOCAL_CHANNELS))
    || strlen(name) >= IRCD_MIN(CHANNELLEN, feature_int(FEAT_CHANNELLEN))) {
        continue;
      }

      if (!(chptr = get_channel(acptr, name, CGT_CREATE)))
         continue;

      joinbuf_join(&create, chptr, CHFL_CHANOP | CHFL_CHANNEL_MANAGER);

    } else if (find_member_link(chptr, acptr)) {
      continue; /* already on channel */
    } else {
      int flags = 0;

      /* Si no puede entrar y el tercer parametro empieza por C que no entre */
      if (parc > 3 && *parv[3]=='C') {
        int err = 0;

        if ((chptr->mode.mode & MODE_SSLONLY) && !IsSSL(acptr))
          err = ERR_SSLONLYCHAN;
        else if (IsInvited(acptr, chptr)) {
          /* Invites bypass these other checks. */
        } else if (chptr->mode.mode & MODE_INVITEONLY)
          err = ERR_INVITEONLYCHAN;
        else if (chptr->mode.limit && (chptr->users >= chptr->mode.limit))
          err = ERR_CHANNELISFULL;
        else if ((chptr->mode.mode & MODE_REGONLY) && !IsAccount(acptr))
          err = ERR_NEEDREGGEDNICK;
        else if ((chptr->mode.mode & MODE_OPERONLY) && !IsAnOper(acptr))
          err = ERR_OPERONLYCHAN;
        else if (find_ban(acptr, chptr->banlist))
          err = ERR_BANNEDFROMCHAN;
        else if (*chptr->mode.key)
          err = ERR_BADCHANNELKEY;

        if (err) {
          switch(err) {
            case ERR_NEEDREGGEDNICK:
              send_reply(acptr,
                         ERR_NEEDREGGEDNICK,
                         chptr->chname,
                         feature_str(FEAT_URLREG));
              break;
            default:
              send_reply(acptr, err, chptr->chname);
              break;
          }
        continue;
        }
      }

#if defined(DDB_TODO)
      /* If is an Owner, sets mode +q */
      if (RegisteredChannel(chptr) && IsAccount(acptr)
          && chptr->owner && !ircd_strcmp(chptr->owner, cli_name(acptr)))
        flags = CHFL_OWNER;
#endif

      joinbuf_join(&join, chptr, flags);

      if (flags & CHFL_CHANOP) {
        struct ModeBuf mbuf;
        /* Always let the server op him: this is needed on a net with older servers
           because they 'destruct' channels immediately when they become empty without
           sending out a DESTRUCT message. As a result, they would always bounce a mode
           (as HACK(2)) when the user ops himself.
           (There is also no particularly good reason to have the user op himself.)
         */
        modebuf_init(&mbuf, &me, acptr, chptr, MODEBUF_DEST_SERVER);
        modebuf_mode_client(&mbuf, MODE_ADD | MODE_CHANOP, acptr, (flags & CHFL_CHANNEL_MANAGER) ? 0 : 1);
        modebuf_flush(&mbuf);
      }
    }

    del_invite(acptr, chptr);

    if (chptr->topic[0]) {
      send_reply(acptr, RPL_TOPIC, chptr->chname, chptr->topic);
      send_reply(acptr, RPL_TOPICWHOTIME, chptr->chname, chptr->topic_nick,
                 chptr->topic_time);
    }

    do_names(acptr, chptr, NAMES_ALL|NAMES_EON); /* send /names list */
  }

  joinbuf_flush(&join); /* must be first, if there's a JOIN 0 */
  joinbuf_flush(&create);

  return 0;

}
