/*
 * IRC-Hispano IRC Daemon, ircd/m_monitor.c
 *
 * Copyright (C) 1997-2018 IRC-Hispano Development Team <devel@irc-hispano.es>
 * Copyright (C) 2017 Toni Garcia (zoltan) <toni@tonigarcia.es>
 * Copyright (C) 2002 Toni Garcia (zoltan) <toni@tonigarcia.es> (WATCH)
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
 * @brief Handlers for MONITOR command.
 */
#include "config.h"

#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_snprintf.h"
#include "ircd_string.h"
#include "list.h"
#include "monitor.h"
#include "msgq.h"
#include "numeric.h"
#include "s_user.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdlib.h>
#include <string.h>

/** Add nick to the MONITOR List.
 *  @param[in] sptr
 *  @param[in] nicks
 */
static void
add_monitor(struct Client *sptr, char *nicks)
{
  char onbuf[BUFSIZE], offbuf[BUFSIZE];
  struct Client *acptr;
  char *nick, *p;
  struct MsgBuf *mbon, *mboff;
  int needs_commaon = 0;
  int needs_commaoff = 0;

  mbon = msgq_make(sptr, rpl_str(RPL_MONONLINE), cli_name(&me), cli_name(sptr), "");
  mboff = msgq_make(sptr, rpl_str(RPL_MONOFFLINE), cli_name(&me), cli_name(sptr), "");

  for (nick = ircd_strtok(&p, nicks, ","); nick;
       nick = ircd_strtok(&p, 0, ","))
  {

    if (EmptyString(nick) || strlen(nick) > NICKLEN-1)
      continue;

    if (cli_user(sptr)->monitors >= feature_int(FEAT_MAXMONITOR))
    {
      char monitorbuf[512];

      if (needs_commaon)
        send_buffer(sptr, mbon, 0);
      if (needs_commaoff)
        send_buffer(sptr, mboff, 0);
      msgq_clean(mbon);
      msgq_clean(mboff);

      if (p)
        ircd_snprintf(0, monitorbuf, sizeof(monitorbuf), "%s,%s", nick, p);
      else
        ircd_snprintf(0, monitorbuf, sizeof(monitorbuf), "%s", nick);

      send_reply(sptr, ERR_MONLISTFULL, nick, feature_int(FEAT_MAXMONITOR), monitorbuf);
      return;
    }

    if (FindMonitor(cli_name(sptr)))
      continue;

    monitor_add_nick(sptr, nick);

    if ((acptr = FindUser(nick)))
    {
      if (msgq_bufleft(mbon) < (strlen(cli_name(acptr)) + strlen(cli_user(acptr)->username) +  strlen(cli_user(acptr)->host) + 3))
      {
        send_buffer(sptr, mbon, 0);
        msgq_clean(mbon);
        mbon = msgq_make(sptr, rpl_str(RPL_MONONLINE), cli_name(&me), cli_name(sptr), "");
        needs_commaon = 0;
      }
      msgq_append(0, mbon, "%s%s!%s@%s", needs_commaon ? "," : "", cli_name(acptr), cli_user(acptr)->username, cli_user(acptr)->host);
      needs_commaon = 1;

    }
    else
    {
      if (msgq_bufleft(mboff) < strlen(nick) + 1)
      {
        send_buffer(sptr, mboff, 0);
        msgq_clean(mboff);
        mboff = msgq_make(sptr, rpl_str(RPL_MONOFFLINE), cli_name(&me), cli_name(sptr), "");
        needs_commaoff = 0;
      }
      msgq_append(0, mboff, "%s%s", needs_commaoff ? "," : "", nick);
      needs_commaoff = 1;
    }
  }

  if (needs_commaon)
    send_buffer(sptr, mbon, 0);
  if (needs_commaoff)
    send_buffer(sptr, mboff, 0);
}

/** Delete nick from  MONITOR List.
 *  @param[in] sptr
 *  @param[in] nicks
 */
static void
del_monitor(struct Client *sptr, char *nicks)
{
  char *nick, *p;

  if (!cli_user(sptr)->monitor)
    return;

  for (nick = ircd_strtok(&p, nicks, ","); nick;
       nick = ircd_strtok(&p, 0, ","))
  {
    if (EmptyString(nick))
      continue;

    monitor_del_nick(sptr, nick);
  }
}

/** List users online.
 *  @param[in] sptr
 */
static void
list_monitor(struct Client *sptr)
{
  struct Client *acptr;
  struct SLink *lp;
  struct MsgBuf *mb;
  int needs_comma = 0;

  lp = cli_user(sptr)->monitor;

  if (!lp)
  {
    send_reply(sptr, RPL_ENDOFMONLIST);
    return;
  }

  mb = msgq_make(sptr, rpl_str(RPL_MONLIST), cli_name(&me), cli_name(sptr), "");

  while (lp)
  {
    if ((acptr = FindUser(lp->value.mptr->mo_nick)))
    {
      if (msgq_bufleft(mb) < strlen(cli_name(acptr)) + 1) {
        send_buffer(sptr, mb, 0);
        msgq_clean(mb);
        mb = msgq_make(sptr, rpl_str(RPL_MONLIST), cli_name(&me), cli_name(sptr), "");
        needs_comma = 0;
      }
      msgq_append(0, mb, "%s%s", needs_comma ? "," : "", cli_name(acptr));
      needs_comma = 1;
    }
    lp = lp->next;
  }

  if (needs_comma)
    send_buffer(sptr, mb, 0);
  send_reply(sptr, RPL_ENDOFMONLIST);
}

/** Show status the MONITOR List.
 *  @param[in] sptr
 */
static void
show_monitor_status(struct Client *sptr)
{
  struct Client *acptr;
  struct SLink *lp;
  struct MsgBuf *mbon, *mboff;
  int needs_commaon = 0;
  int needs_commaoff = 0;

  mbon = msgq_make(sptr, rpl_str(RPL_MONONLINE), cli_name(&me), cli_name(sptr), "");
  mboff = msgq_make(sptr, rpl_str(RPL_MONOFFLINE), cli_name(&me), cli_name(sptr), "");

  lp = cli_user(sptr)->monitor;
  while (lp)
  {
    if ((acptr = FindUser(lp->value.mptr->mo_nick)))
    {
      if (msgq_bufleft(mbon) < (strlen(cli_name(acptr)) + strlen(cli_user(acptr)->username) +  strlen(cli_user(acptr)->host) + 3))
      {
        send_buffer(sptr, mbon, 0);
        msgq_clean(mbon);
        mbon = msgq_make(sptr, rpl_str(RPL_MONONLINE), cli_name(&me), cli_name(sptr), "");
        needs_commaon = 0;
      }
      msgq_append(0, mbon, "%s%s!%s@%s", needs_commaon ? "," : "", cli_name(acptr), cli_user(acptr)->username, cli_user(acptr)->host);
      needs_commaon = 1;

    }
    else
    {
      if (msgq_bufleft(mboff) < strlen(lp->value.mptr->mo_nick) + 1) {
        send_buffer(sptr, mboff, 0);
        msgq_clean(mboff);
        mboff = msgq_make(sptr, rpl_str(RPL_MONOFFLINE), cli_name(&me), cli_name(sptr), "");
        needs_commaoff = 0;
      }
      msgq_append(0, mboff, "%s%s", needs_commaoff ? "," : "", lp->value.mptr->mo_nick);
      needs_commaoff = 1;
    }
    lp = lp->next;
  }

  if (needs_commaon)
    send_buffer(sptr, mbon, 0);
  if (needs_commaoff)
    send_buffer(sptr, mboff, 0);
}


/** Handle a message handler for local clients
 * @param[in] cptr Client that sent us the message.
 * @param[in] sptr Original source of message.
 * @param[in] parc Number of arguments.
 * @param[in] parv Argument vector.
 *
 * parv[0] = sender prefix
 * parv[1] = parametres
 * parv[2] = nicks (optional)
 *
 * If a parameter begins by '+', adds a nick or nicks list separated with ",".
 * And if a parameter begins by '-' deletes a nick or nicks list separated with ",".
 * If to a 'C' or 'c' is sent, deletes all the monitor list.
 * A 'S' or 's' gives the notify status.
 * The parameter 'l' or 'L' list nicks on-line.
 *
 * 2002/05/20 zoltan <toni@tonigarcia.es>
 * 2017/12/01 Change WATCH to MONITOR
 */
int m_monitor(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  char *s, *p = 0;

  if (parc < 2)
    return need_more_params(sptr, "MONITOR");

  switch (parv[1][0])
  {
    case '+':
      if (parc < 3 || EmptyString(parv[2]))
        return need_more_params(sptr, "MONITOR");

      add_monitor(sptr, parv[2]);
      break;


    case '-':
      if (parc < 3 || EmptyString(parv[2]))
        return need_more_params(sptr, "MONITOR");

      del_monitor(sptr, parv[2]);
      break;


    /*
     * Parameter C/c
     *
     * Deletes all the MONITOR list.
     */
    case 'C':
    case 'c':
      monitor_list_clean(sptr);
      break;


    /*
     * Parameter L/l
     *
     * List users online.
     */
    case 'L':
    case 'l':
      list_monitor(sptr);
      break;

    /*
     * Parameter S/s
     *
     * Status the MONITOR List.
     */
    case 'S':
    case 's':
      show_monitor_status(sptr);
      break;

    default:
      break;
  }

  return 0;
}
