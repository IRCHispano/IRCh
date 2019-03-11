/*
 * IRC-Hispano IRC Daemon, include/ddb_tools.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2019 Toni Garcia (zoltan) <toni@tonigarcia.es>
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
 * @brief Tools of Distributed DataBases.
 */
#include "config.h"

#include "ddb.h"
#include "ircd_alloc.h"
#include "ircd_log.h"
#include "ircd_string.h"

#include <json-c/json.h>
#include <json-c/json_object.h>
/*
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
#include "s_user.h"
#include "send.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
*/

static struct DdbNick *ddbnptr = NULL;
static struct DdbOperator *ddboptr = NULL;

struct DdbNick *ddb_nick_get(char *ddb_key, char *ddb_content, int cache)
{
  json_object *json, *json_child;
  enum json_tokener_error jerr = json_tokener_success;

  if (jerr != json_tokener_success) {
    log_write(LS_DDB, L_ERROR, 0, "WARNING - Erroneus record Table 'n': Key '%s'", ddb_key);
    return NULL;
  }

  if (!ddbnptr)
    ddbnptr = MyMalloc(sizeof(struct DdbNick));

  /* Cache */
  if (cache && ddbnptr->name && !ircd_strcmp(ddbnptr->name, ddb_key))
    return ddbnptr;

  /* Clean old dates */
  if (ddbnptr->name)
    MyFree(ddbnptr->name);
  if (ddbnptr->password)
    MyFree(ddbnptr->password);
  if (ddbnptr->certificate)
    MyFree(ddbnptr->certificate);
  ddbnptr->flags = 0;
  if (ddbnptr->automodes)
    MyFree(ddbnptr->automodes);
  if (ddbnptr->reason)
    MyFree(ddbnptr->reason);

  DupString(ddbnptr->name, ddb_key);

  json_object_object_get_ex(json, "pass", &json_child);
  if (json_child)
    DupString(ddbnptr->password, (char *)json_object_get_string(json_child));

  json_object_object_get_ex(json, "certificate", &json_child);
  if (json_child)
    DupString(ddbnptr->certificate, (char *)json_object_get_string(json_child));

  json_object_object_get_ex(json, "flags", &json_child);
  if (json_child)
    ddbnptr->flags = json_object_get_int(json_child);

  json_object_object_get_ex(json, "automodes", &json_child);
  if (json_child)
    DupString(ddbnptr->automodes, (char *)json_object_get_string(json_child));

  json_object_object_get_ex(json, "reason", &json_child);
  if (json_child)
    DupString(ddbnptr->reason, (char *)json_object_get_string(json_child));

  return ddbnptr;
}

struct DdbNick *ddb_nick_find(char *nick)
{
  struct Ddb *ddb;
  ddb = ddb_find_key(DDB_NICKDB, nick);

  if (ddb)
    return ddb_nick_get(ddb_key(ddb), ddb_content(ddb), 1);
  else
    return NULL;
}

/** List of privs flag. */
static const struct PrivsFlag {
  enum Priv priv; /**< User mode constant. */
  u_int64_t flag;  /**< Character corresponding to the mode. */
} privsFlagList[] = {
  { PRIV_CHAN_LIMIT,         0x0000000001 },
  { PRIV_MODE_LCHAN,         0x0000000002 },
  { PRIV_WALK_LCHAN,         0x0000000004 },
  { PRIV_DEOP_LCHAN,         0x0000000008 },
  { PRIV_SHOW_INVIS,         0x0000000010 },
  { PRIV_SHOW_ALL_INVIS,     0x0000000020 },
  { PRIV_UNLIMIT_QUERY,      0x0000000040 },
  { PRIV_KILL,               0x0000000080 },
  { PRIV_LOCAL_KILL,         0x0000000100 },
  { PRIV_REHASH,             0x0000000200 },
  { PRIV_RESTART,            0x0000000400 },
  { PRIV_DIE,                0x0000000800 },
  { PRIV_GLINE,              0x0000001000 },
  { PRIV_LOCAL_GLINE,        0x0000002000 },
  { PRIV_JUPE,               0x0000004000 },
  { PRIV_LOCAL_JUPE,         0x0000008000 },
  { PRIV_OPMODE,             0x0000010000 },
  { PRIV_LOCAL_OPMODE,       0x0000020000 },
  { PRIV_SET,                0x0000040000 },
  { PRIV_WHOX,               0x0000080000 },
  { PRIV_BADCHAN,            0x0000100000 },
  { PRIV_LOCAL_BADCHAN,      0x0000200000 },
  { PRIV_SEE_CHAN,           0x0000400000 },
  { PRIV_PROPAGATE,          0x0000800000 },
  { PRIV_DISPLAY,            0x0001000000 },
  { PRIV_SEE_OPERS,          0x0002000000 },
  { PRIV_WIDE_GLINE,         0x0004000000 },
  { PRIV_LIST_CHAN,          0x0008000000 },
  { PRIV_FORCE_OPMODE,       0x0010000000 },
  { PRIV_FORCE_LOCAL_OPMODE, 0x0020000000 },
  { PRIV_APASS_OPMODE,       0x0040000000 },
  { PRIV_WALK_CHAN,          0x0080000000 },
  { PRIV_NETWORK,            0x0100000000 },
  { PRIV_CHANSERV,           0x0200000000 },
  { PRIV_HIDDEN_VIEWER,      0x0400000000 },
  { PRIV_WHOIS_NOTICE,       0x0800000000 },
  { PRIV_HIDE_IDLE,          0x1000000000 },
  { PRIV_DBQ,                0x2000000000 },
};

/** Length of #privsFlagList. */
#define PRIVSFLAGLIST_SIZE sizeof(privsFlagList) / sizeof(struct PrivsFlag)

struct DdbOperator *ddb_opers_get(char *ddb_key, char *ddb_content, int cache)
{
  json_object *json, *json_child;
  enum json_tokener_error jerr = json_tokener_success;
  int i;

  if (jerr != json_tokener_success)
  {
    log_write(LS_DDB, L_ERROR, 0, "WARNING - Erroneus record Table 'o': Key '%s'", ddb_key);
    return NULL;
  }

  if (!ddboptr)
    ddboptr = MyMalloc(sizeof(struct DdbOperator));

  /* Cache */
  if (cache && ddboptr->nick && !ircd_strcmp(ddboptr->nick, ddb_key))
    return ddboptr;

  /* Clean old dates */
  if (ddboptr->nick)
    MyFree(ddboptr->nick);
  if (ddboptr->modes)
    MyFree(ddboptr->modes);
  ddboptr->privs_flags = 0;
  memset(&ddboptr->privs, 0, sizeof(struct Privs));

  DupString(ddboptr->nick, ddb_key);

  json_object_object_get_ex(json, "modes", &json_child);
  if (json_child)
    DupString(ddboptr->modes, (char *)json_object_get_string(json_child));

  json_object_object_get_ex(json, "privs", &json_child);
  if (json_child)
    ddboptr->privs_flags, json_object_get_int64(json_child);

  /* Set Privs */
  for (i = 0; i < PRIVSFLAGLIST_SIZE; ++i)
  {
    if (ddboptr->privs_flags & privsFlagList[i].flag)
      FlagSet(&ddboptr->privs, privsFlagList[i].priv);
  }

  return ddboptr;
}

struct DdbOperator *ddb_opers_find(char *nick)
{
  struct Ddb *ddb;
  ddb = ddb_find_key(DDB_OPERDB, nick);

  if (ddb)
    return ddb_opers_get(ddb_key(ddb), ddb_content(ddb), 1);
  else
    return NULL;
}
