/*
 * IRC-Hispano IRC Daemon, ircd/m_dbq.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2004-2018 Toni Garcia (zoltan) <toni@tonigarcia.es>
 * Copyright (C) 1999-2004 Jesus Cea Avion <jcea@jcea.es>
 * Copyright (C) 1999-2000 Jordi Murgo <savage@apostols.org>
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
 * @brief Handlers for DBQ command.
 *
 * 1999/10/13 savage@apostols.org
 */
#include "config.h"

#include "ddb.h"
#include "client.h"
#include "ircd.h"
#include "ircd_alloc.h"
#include "ircd_chattr.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "send.h"
#include "s_debug.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <string.h>

/**
 *
 */
static int checkpriv_dbq(struct Client* sptr, unsigned char table, char *key)
{
  if (IsAdmin(sptr) || IsCoder(sptr))
    return 1;

  switch (table)
  {
    case DDB_NICKDB:
    case DDB_WEBIRCDB:
      return 0;

    case DDB_FEATUREDB:
      if (!strcmp(key, "KEYCIFRADO"))
        return 0;
  }
  return 1;
}

/** Handle a DBQ (DataBase Query) command from a server.
 * See @ref m_functions for general discussion of parameters.
 *
 * \a parv[1] is a server to query (optional)
 * \a parv[parc - 2] is a table
 * \a parv[parc - 1] is a key

 *
 * @param[in] cptr Client that sent us the message.
 * @param[in] sptr Original source of message.
 * @param[in] parc Number of arguments.
 * @param[in] parv Argument vector.
 */
int ms_dbq(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char table, *key, *server;
  struct Client *acptr;
  struct Ddb *ddb;
  static char *kn = NULL;
  static int kl = 0;
  int i;

  assert(0 != cptr);
  assert(0 != sptr);
  assert(IsServer(cptr));

  if (parc == 3)
  {
    server = NULL;
    table = *parv[parc - 2];
    key = parv[parc - 1];
  }
  else
  {
    server = parv[1];
    table = *parv[parc - 2];
    key = parv[parc - 1];
    if (*server == '*')
    {
      /* WOOW, BROADCAST */
      sendcmdto_serv_butone(sptr, CMD_DBQ, cptr, "* %c %s", table, key);
    }
    else
    {
      /* NOT BROADCAST */
      if (!(acptr = find_match_server(server)))
      {
        send_reply(sptr, ERR_NOSUCHSERVER, server);
        return 0;
      }

      if (!IsMe(acptr))
      {
        sendcmdto_one(acptr, CMD_DBQ, sptr, "%s %c %s", server, table, key);
        return 0;
      }
    }
  }

  sendwallto_group_butone(&me, WALL_WALLOPS, 0,
                   "Remote DBQ %c %s From %s", table, key, cli_name(sptr));
  log_write(LS_DDB, L_INFO, 0, "Remote DBQ %c %s From %C", table, key, sptr);

  if (!ddb_table_is_resident(table))
  {
    sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :DBQ ERROR Table='%c' Key='%s' TABLE_NOT_RESIDENT",
                sptr, table, key);
    return 0;
  }

  i = strlen(key) + 1;
  if (i > kl)
  {
    kl = i;
    if (kn)
      MyFree(kn);
    kn = MyMalloc(kl);
    assert(kn);
  }

  strcpy(kn, key);
  i = 0;
  while (kn[i] != 0)
  {
    kn[i] = ToLower(kn[i]);
    i++;
  }

  /* Check privilegies */
  if (!checkpriv_dbq(cptr, table, key))
  {
    sendcmdto_one(&me, CMD_NOTICE, cptr, "%C :DBQ ERROR You do not have permission to access Table='%c' Key='%s'",
                  cptr, table, key);
    return 0;
  }

  ddb = ddb_find_key(table, key);
  if (!ddb)
  {
    sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :DBQ ERROR Table='%c' Key='%s' REG_NOT_FOUND",
                  sptr, table, key);
    return 0;
  }


  sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :DBQ OK Table='%c' Key='%s' Content='%s'",
                sptr, table, ddb_key(ddb), ddb_content(ddb));

  return 0;
}

/** Handle a DBQ (DataBase Query) command from a operator.
 * See @ref m_functions for general discussion of parameters.
 *
 * \a parv[1] is a server to query (optional)
 * \a parv[parc - 2] is a table
 * \a parv[parc - 1] is a key
 *
 * @param[in] cptr Client that sent us the message.
 * @param[in] sptr Original source of message.
 * @param[in] parc Number of arguments.
 * @param[in] parv Argument vector.
 */
int mo_dbq(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char table, *key, *server;
  struct Client *acptr;
  struct Ddb *ddb;
  static char *kn = NULL;
  static int kl = 0;
  int i;

  if (!HasPriv(cptr, PRIV_DBQ))
    return send_reply(cptr, ERR_NOPRIVILEGES);

  if ((parc != 3 && parc != 4) ||
      (parc == 3 && (parv[1][0] == '\0' || parv[1][1] != '\0')) ||
      (parc == 4 && (parv[2][0] == '\0' || parv[2][1] != '\0')))
  {
    sendcmdto_one(&me, CMD_NOTICE, cptr, "%C :Incorrect parameters. Format: DBQ [<server>] <Table> <Key>",
                  cptr);
    return need_more_params(cptr, "DBQ");;
  }

  if (parc == 3)
  {
    server = NULL;
    table = *parv[parc - 2];
    key = parv[parc - 1];
  }
  else
  {
    server = parv[1];
    table = *parv[parc - 2];
    key = parv[parc - 1];
    if (*server == '*')
    {
      /* WOOW, BROADCAST */
      sendcmdto_serv_butone(cptr, CMD_DBQ, cptr, "* %c %s", table, key);
    }
    else
    {
      /* NOT BROADCAST */
      if (!(acptr = find_match_server(server)))
      {
        send_reply(cptr, ERR_NOSUCHSERVER, server);
        return 0;
      }

      if (!IsMe(acptr))
      {
        sendcmdto_one(acptr, CMD_DBQ, cptr, "%s %c %s", server, table, key);
        return 0;
      }
    }
  }

  if (!ddb_table_is_resident(table))
  {
    sendcmdto_one(&me, CMD_NOTICE, cptr, "%C :DBQ ERROR Table='%c' Key='%s' TABLE_NOT_RESIDENT",
                cptr, table, key);
    return 0;
  }

  i = strlen(key) + 1;
  if (i > kl)
  {
    kl = i;
    if (kn)
      MyFree(kn);
    kn = MyMalloc(kl);
    assert(kn);
  }

  strcpy(kn, key);
  i = 0;
  while (kn[i] != 0)
  {
    kn[i] = ToLower(kn[i]);
    i++;
  }

  /* Check privilegies */
  if (!checkpriv_dbq(cptr, table, key))
  {
    sendcmdto_one(&me, CMD_NOTICE, cptr, "%C :DBQ ERROR You do not have permission to access Table='%c' Key='%s'",
                  cptr, table, key);
    return 0;
  }

  ddb = ddb_find_key(table, key);
  if (!ddb)
  {
    sendcmdto_one(&me, CMD_NOTICE, cptr, "%C :DBQ ERROR Table='%c' Key='%s' REG_NOT_FOUND",
                  cptr, table, key);
    return 0;
  }


  sendcmdto_one(&me, CMD_NOTICE, cptr, "%C :DBQ OK Table='%c' Key='%s' Content='%s'",
                cptr, table, ddb_key(ddb), ddb_content(ddb));

  return 0;
}
