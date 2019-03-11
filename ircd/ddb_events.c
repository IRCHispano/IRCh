/*
 * IRC-Hispano IRC Daemon, include/ddb_events.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2004-2007,2018 Toni Garcia (zoltan) <toni@tonigarcia.es>
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
 * @brief Events of Distributed DataBases.
 */
#include "config.h"

#include "ddb.h"
#include "channel.h"
#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_alloc.h"
#include "ircd_chattr.h"
#include "ircd_features.h"
#include "ircd_snprintf.h"
#include "ircd_tea.h"
#include "msg.h"
#include "numnicks.h"
#include "querycmds.h"
#include "s_user.h"
#include "send.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** It indicates events is initialized */
static int events_init = 0;
/** Events table engine */
ddb_events_table_td ddb_events_table[DDB_TABLE_MAX];

static void ddb_events_table_features(char *key, char *content, int update);
static void ddb_events_table_logging(char *key, char *content, int update);
static void ddb_events_table_nicks(char *key, char *content, int update);
static void ddb_events_table_operators(char *key, char *content, int update);

/** Initialize events module of %DDB Distributed DataBases.
 */
void
ddb_events_init(void)
{
  if (events_init)
    return;

  ddb_events_table[DDB_BOTDB] = 0;
  ddb_events_table[DDB_CHANDB] = 0; //ddb_events_table_channels;
  ddb_events_table[DDB_CHANDB2] = 0; //ddb_events_table_channels2;
  ddb_events_table[DDB_EXCEPTIONDB] = 0; //ddb_events_table_exceptions;
  ddb_events_table[DDB_FEATUREDB] = ddb_events_table_features;
  ddb_events_table[DDB_ILINEDB] = 0;
  ddb_events_table[DDB_JUPEDB] = 0; //ddb_events_table_jupes;
  ddb_events_table[DDB_LOGGINGDB] = ddb_events_table_logging;
  ddb_events_table[DDB_MOTDDB] = 0;
  ddb_events_table[DDB_NICKDB] = ddb_events_table_nicks;
  ddb_events_table[DDB_OPERDB] = ddb_events_table_operators;
  ddb_events_table[DDB_PSEUDODB] = 0; //ddb_events_table_pseudo;
  ddb_events_table[DDB_QUARANTINEDB] = 0; //ddb_events_table_quarantines;
  ddb_events_table[DDB_CHANREDIRECTDB] = 0;
  ddb_events_table[DDB_SPAMDB] = 0; //ddb_events_table_spam;
  ddb_events_table[DDB_UWORLDDB] = 0; //ddb_events_table_uworlds;
  ddb_events_table[DDB_VHOSTDB] = 0; //ddb_events_table_vhosts;
  ddb_events_table[DDB_WEBIRCDB] = 0; //ddb_events_table_webircs;
  events_init = 1;
}

/** Handle events on Features Table.
 * @param[in] key Key of registry.
 * @param[in] content Content of registry.
 * @param[in] update Update of registry or no.
 */
static void
ddb_events_table_features(char *key, char *content, int update)
{
  if (content)
  {
    char *tempa[2];
    tempa[0] = key;
    tempa[1] = content;

    feature_set(&me, (const char * const *)tempa, 2);
  }
  else
  {
    char *tempb[1];
    tempb[0] = key;

    feature_set(&me, (const char * const *)tempb, 1);
  }
}

/** Handle events on Logging Table.
 * @param[in] key Key of registry.
 * @param[in] content Content of registry.
 * @param[in] update Update of registry or no.
 */
static void
ddb_events_table_logging(char *key, char *content, int update)
{
#if 0
  static char *keytemp = NULL;

  if (content)
  {
    char *tempa[2];
    tempa[0] = keytemp;
    tempa[1] = content;

    feature_set(&me, (const char * const *)tempa, 2);
  }
  else
  {
    char *tempb[1];
    tempb[0] = keytemp;

    feature_set(&me, (const char * const *)tempb, 1);
  }
#endif
}

/** Handle events on Nick Table.
 * @param[in] key Key of registry.
 * @param[in] content Content of registry.
 * @param[in] update Update of registry or no.
 */
static void
ddb_events_table_nicks(char *key, char *content, int update)
{
  struct Client *cptr;
  struct DdbNick *ddbnick;
  char *botname;
  int nick_renames = 0;

  /* Only my clients */
  if ((cptr = FindUser(key)) && MyConnect(cptr))
  {
    botname = ddb_get_botname(DDB_NICKSERV);
    /* Droping Key */
    if (!content && (IsAccount(cptr) || IsNickSuspended(cptr)))
    {
      struct Flags oldflags;

      oldflags = cli_flags(cptr);

      // Clear operator modes and privilegies for user.
      nickreg_clear_modeprivs(cptr);

      // Clear user modes.
      nickreg_clear_mode(cptr);

      sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                       "%C :*** Your nick %C is droping.", cptr, cptr);

      send_umode_out(cptr, cptr, &oldflags, IsRegistered(cptr));
    }
    else if (content && ((ddbnick = ddb_nick_get(key, content, 0))))
    {
      /* New Key or Update Key */
      if (ddbnick->flags & DDB_NICK_FORBID)
      {
        /* Forbid Nick */
        sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                         "%C :*** Your nick %C has been forbided, cannot be used. Reason: %s",
                         cptr, cptr, ddbnick->reason ? ddbnick->reason : "<no reason>");
        nick_renames = 1;
      }
      else if (ddbnick->flags & DDB_NICK_SUSPEND && update && IsAccount(cptr))
      {
        struct Flags oldflags;

        oldflags = cli_flags(cptr);

        // Clear operator modes and privilegies for user.
        nickreg_clear_modeprivs(cptr);

        // Clear user modes.
        nickreg_clear_mode(cptr);

        SetNickSuspended(cptr);

        sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                         "%C :*** Your nick %C has been suspended. Reason: %s",
                         cptr, cptr, ddbnick->reason ? ddbnick->reason : "<no reason>");

        send_umode_out(cptr, cptr, &oldflags, IsRegistered(cptr));
      }
      else if ((!ddbnick->flags & DDB_NICK_SUSPEND) && update && IsNickSuspended(cptr))
      {
        struct Flags oldflags;

        oldflags = cli_flags(cptr);

        ClearNickSuspended(cptr);

        nickreg_set_modes(cptr, ddbnick);

        sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                         "%C :*** Your nick %C has been unsuspended.", cptr, cptr);

        send_umode_out(cptr, cptr, &oldflags, IsRegistered(cptr));
      }
      else if (!update)
      {
        sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                         "%C :*** Your nick %C has been registered.", cptr, cptr);
        nick_renames = 1;
      }
      else if (update)
      {
        /* Possible change automodes */
        nickreg_set_modes(cptr, ddbnick);
      }
    }

    if (nick_renames)
    {
      char *newnick;
      char tmp[100];
      char *parv[3];
      int flags = 0;

      newnick = get_random_nick(cptr);

      SetRenamed(flags);

      parv[0] = cli_name(cptr);
      parv[1] = newnick;
      ircd_snprintf(0, tmp, sizeof(tmp), "%T", TStime());
      parv[2] = tmp;

      set_nick_name(cptr, cptr, newnick, 3, parv, flags);
    }
  }
}

/** Handle events on Operators Table.
 * @param[in] key Key of registry.
 * @param[in] content Content of registry.
 * @param[in] update Update of registry or no.
 */
static void
ddb_events_table_operators(char *key, char *content, int update)
{
  struct Client *cptr;
  if ((cptr = FindUser(key)) && MyConnect(cptr))
  {
    /* Droping Key */
    if (!content && (IsAdmin(cptr) || IsCoder(cptr) || IsHelper(cptr) || IsServicesBot(cptr)))
    {
      struct Flags oldflags;

      oldflags = cli_flags(cptr);

      // Clear all modes and privilegies for user.
      nickreg_clear_modeprivs(cptr);

      send_umode_out(cptr, cptr, &oldflags, IsRegistered(cptr));

    }
    else if (content)
    {
      struct DdbOperator *ddboptr;

      /* New Key or Update Key */
      if (IsAccount(cptr) && ((ddboptr = ddb_opers_get(key, content, 0))))
      {
        struct Flags oldflags = cli_flags(cptr);

        ClearAdmin(cptr);
        ClearCoder(cptr);
        ClearHelper(cptr);
        ClearServicesBot(cptr);

        if (!IsOperByCmd(cptr))
        {
          ClearLocOp(cptr);
          if (IsOper(cptr))
            --UserStats.opers;
          ClearOper(cptr);
        }

        // Set modes and privilegies for user.
        nickreg_set_modeprivs(cptr, ddboptr);

        send_umode_out(cptr, cptr, &oldflags, IsRegistered(cptr));
      }
    }
  }
}
