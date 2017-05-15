/*
 * IRC-Hispano IRC Daemon, ircd/m_away.c
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
 * @brief Handlers for AWAY command.
 */
#include "config.h"

#include "client.h"
#include "ircd.h"
#include "ircd_alloc.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_user.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <string.h>

/*
 * user_set_away - set user away state
 * returns 1 if client is away or changed away message, 0 if 
 * client is removing away status.
 * NOTE: this function may modify user and message, so they
 * must be mutable.
 */
static int user_set_away(struct User* user, char* message)
{
  char* away;
  assert(0 != user);

  away = user->away;

  if (EmptyString(message)) {
    /*
     * Marking as not away
     */
    if (away) {
      MyFree(away);
      user->away = 0;
    }
  }
  else {
    /*
     * Marking as away
     */
    unsigned int len = strlen(message);

    if (len > AWAYLEN) {
      message[AWAYLEN] = '\0';
      len = AWAYLEN;
    }
    if (away)
      MyFree(away);
    away = (char*) MyMalloc(len + 1);
    assert(0 != away);

    user->away = away;
    strcpy(away, message);
  }
  return (user->away != 0);
}


/*
 * m_away - generic message handler
 * - Added 14 Dec 1988 by jto.
 *
 * parv[0] = sender prefix
 * parv[1] = away message
 *
 * TODO: Throttle aways - many people have a script which resets the away
 *       message every 10 seconds which really chews the bandwidth.
 */
int m_away(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char* away_message = parv[1];
  int was_away = cli_user(sptr)->away != 0;

  assert(0 != cptr);
  assert(cptr == sptr);

  if (user_set_away(cli_user(sptr), away_message))
  {
    if (!was_away)    
      sendcmdto_serv_butone(sptr, CMD_AWAY, cptr, ":%s", away_message);
    send_reply(sptr, RPL_NOWAWAY);
  }
  else {
    sendcmdto_serv_butone(sptr, CMD_AWAY, cptr, "");
    send_reply(sptr, RPL_UNAWAY);
  }
  return 0;
}

/*
 * ms_away - server message handler
 * - Added 14 Dec 1988 by jto.
 *
 * parv[0] = sender prefix
 * parv[1] = away message
 */
int ms_away(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char* away_message = parv[1];

  assert(0 != cptr);
  assert(0 != sptr);
  /*
   * servers can't set away
   */
  if (IsServer(sptr))
    return protocol_violation(sptr,"Server trying to set itself away");

  if (user_set_away(cli_user(sptr), away_message))
    sendcmdto_serv_butone(sptr, CMD_AWAY, cptr, ":%s", away_message);
  else
    sendcmdto_serv_butone(sptr, CMD_AWAY, cptr, "");
  return 0;
}


