/*
 * IRC - Internet Relay Chat, ircd/m_info.c
 * Copyright (C) 1990 Jarkko Oikarinen and
 *                    University of Oulu, Computing Center
 *
 * See file AUTHORS in IRC package for additional names of
 * the programmers.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

/*
 * m_functions execute protocol messages on this server:
 *
 *    cptr    is always NON-NULL, pointing to a *LOCAL* client
 *            structure (with an open socket connected!). This
 *            identifies the physical socket where the message
 *            originated (or which caused the m_function to be
 *            executed--some m_functions may call others...).
 *
 *    sptr    is the source of the message, defined by the
 *            prefix part of the message if present. If not
 *            or prefix not found, then sptr==cptr.
 *
 *            (!IsServer(cptr)) => (cptr == sptr), because
 *            prefixes are taken *only* from servers...
 *
 *            (IsServer(cptr))
 *                    (sptr == cptr) => the message didn't
 *                    have the prefix.
 *
 *                    (sptr != cptr && IsServer(sptr) means
 *                    the prefix specified servername. (?)
 *
 *                    (sptr != cptr && !IsServer(sptr) means
 *                    that message originated from a remote
 *                    user (not local).
 *
 *            combining
 *
 *            (!IsServer(sptr)) means that, sptr can safely
 *            taken as defining the target structure of the
 *            message in this server.
 *
 *    *Always* true (if 'parse' and others are working correct):
 *
 *    1)      sptr->from == cptr  (note: cptr->from == cptr)
 *
 *    2)      MyConnect(sptr) <=> sptr == cptr (e.g. sptr
 *            *cannot* be a local connection, unless it's
 *            actually cptr!). [MyConnect(x) should probably
 *            be defined as (x == x->from) --msa ]
 *
 *    parc    number of variable parameter strings (if zero,
 *            parv is allowed to be NULL)
 *
 *    parv    a NULL terminated list of parameter pointers,
 *
 *                    parv[0], sender (prefix string), if not present
 *                            this points to an empty string.
 *                    parv[1]...parv[parc-1]
 *                            pointers to additional parameters
 *                    parv[parc] == NULL, *always*
 *
 *            note:   it is guaranteed that parv[0]..parv[parc-1] are all
 *                    non-NULL pointers.
 */
#if 0
/*
 * No need to include handlers.h here the signatures must match
 * and we don't need to force a rebuild of all the handlers everytime
 * we add a new one to the list. --Bleep
 */
#include "handlers.h"
#endif /* 0 */
#include "client.h"
#include "ircd.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_misc.h"
#include "s_user.h"
#include "send.h"
#include "version.h"

#include <assert.h>

/*
 * m_info - generic message handler
 *
 * parv[0] = sender prefix
 * parv[1] = servername
 */
int m_info(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char **text = infotext;

  if (hunt_server(1, cptr, sptr, "%s%s " TOK_INFO " :%s", 1, parc, parv) == HUNTED_ISME)
  {
    while (text[2])
    {
      if (!IsOper(sptr))
        sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], *text);
      text++;
    }
    if (IsOper(sptr))
    {
      while (*text)
        sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], *text++);
      sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], "");
    }
    sendto_one(sptr, ":%s %d %s :Birth Date: %s, compile # %s",
        me.name, RPL_INFO, parv[0], creation, generation);
    sendto_one(sptr, ":%s %d %s :On-line since %s",
        me.name, RPL_INFO, parv[0], myctime(me.firsttime));
    sendto_one(sptr, rpl_str(RPL_ENDOFINFO), me.name, parv[0]);
  }
  return 0;
}

/*
 * ms_info - server message handler
 *
 * parv[0] = sender prefix
 * parv[1] = servername
 */
int ms_info(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char **text = infotext;

  if (IsServer(sptr))
    return 0;

  if (hunt_server(1, cptr, sptr, "%s%s " TOK_INFO " :%s", 1, parc, parv) == HUNTED_ISME)
  {
    while (text[2])
    {
      if (!IsOper(sptr))
        sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], *text);
      text++;
    }
    if (IsOper(sptr))
    {
      while (*text)
        sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], *text++);
      sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], "");
    }
    sendto_one(sptr, ":%s %d %s :Birth Date: %s, compile # %s",
        me.name, RPL_INFO, parv[0], creation, generation);
    sendto_one(sptr, ":%s %d %s :On-line since %s",
        me.name, RPL_INFO, parv[0], myctime(me.firsttime));
    sendto_one(sptr, rpl_str(RPL_ENDOFINFO), me.name, parv[0]);
  }
  return 0;
}

/*
 * mo_info - oper message handler
 *
 * parv[0] = sender prefix
 * parv[1] = servername
 */
int mo_info(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char **text = infotext;

  if (hunt_server(1, cptr, sptr, "%s%s " TOK_INFO " :%s", 1, parc, parv) == HUNTED_ISME)
  {
    while (text[2])
    {
      if (!IsOper(sptr))
        sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], *text);
      text++;
    }
    if (IsOper(sptr))
    {
      while (*text)
        sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], *text++);
      sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], "");
    }
    sendto_one(sptr, ":%s %d %s :Birth Date: %s, compile # %s",
        me.name, RPL_INFO, parv[0], creation, generation);
    sendto_one(sptr, ":%s %d %s :On-line since %s",
        me.name, RPL_INFO, parv[0], myctime(me.firsttime));
    sendto_one(sptr, rpl_str(RPL_ENDOFINFO), me.name, parv[0]);
  }
  return 0;
}

  
#if 0
/*
 * m_info
 *
 * parv[0] = sender prefix
 * parv[1] = servername
 */
int m_info(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  const char **text = infotext;

  if (hunt_server(1, cptr, sptr, "%s%s " TOK_INFO " :%s", 1, parc, parv) == HUNTED_ISME)
  {
    while (text[2])
    {
      if (!IsOper(sptr))
        sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], *text);
      text++;
    }
    if (IsOper(sptr))
    {
      while (*text)
        sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], *text++);
      sendto_one(sptr, rpl_str(RPL_INFO), me.name, parv[0], "");
    }
    sendto_one(sptr, ":%s %d %s :Birth Date: %s, compile # %s",
        me.name, RPL_INFO, parv[0], creation, generation);
    sendto_one(sptr, ":%s %d %s :On-line since %s",
        me.name, RPL_INFO, parv[0], myctime(me.firsttime));
    sendto_one(sptr, rpl_str(RPL_ENDOFINFO), me.name, parv[0]);
  }
  return 0;
}
#endif /* 0 */

