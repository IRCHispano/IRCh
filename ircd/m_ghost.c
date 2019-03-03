/*
 * IRC-Hispano IRC Daemon, ircd/m_ghost.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2004 Toni Garcia (zoltan) <zoltan@irc-dev.net>
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
 * @brief Handlers for GHOST command.
 */
#include "config.h"

#include "client.h"
#include "ddb.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "msg.h"
#include "numnicks.h"
#include "s_misc.h"
#include "s_user.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <string.h>
#include <json-c/json.h>
#include <json-c/json_object.h>

/** Handle a GHOST command from a client.
 * See @ref m_functions for general discussion of parameters.
 *
 * \a parv[1] is a nick
 * \a parv[2] is a password (optional)
 *
 * @param[in] cptr Client that sent us the message.
 * @param[in] sptr Original source of message.
 * @param[in] parc Number of arguments.
 * @param[in] parv Argument vector.
 *
 * added by zoltan 5/Ago/2001
 */
int m_ghost(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;
  struct Ddb *ddb;
  json_object *json, *json_flags, *json_pass;
  enum json_tokener_error jerr = json_tokener_success;
  char *passddb, *passwd, *botname;
  int flagsddb = 0;

  assert(0 != cptr);
  assert(sptr == cptr);

  botname = ddb_get_botname(DDB_NICKSERV);

  if (parc < 2)
  {
    sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                     "%C :*** Syntax: /GHOST <nick> [password]", cptr);
    return need_more_params(cptr, "GHOST");
  }

  if (parc < 3)
  {
    passwd = strchr(parv[1], ':');
    if (passwd)
      *passwd++ = '\0';
    else
      passwd = cli_ddb_passwd(cptr);
  }
  else
    passwd = parv[2];

  if (!(acptr = FindUser(parv[1])))
  {
    sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                     "%C :*** The nick %s is not in use.", cptr, parv[1]);
    return 0;
  }

  if (cptr == acptr)
  {
    sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                     "%C :*** ERROR: You can not ghost yourself.", cptr);
    return 0;
  }

  if (!(ddb = ddb_find_key(DDB_NICKDB, parv[1])))
  {
    sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                     "%C :*** The nick %s is not registered.", cptr, parv[1]);
    return 0;
  }

  json = json_tokener_parse_verbose(ddb_content(ddb), &jerr);
  if (jerr != json_tokener_success) {
      sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                       "%C :*** The nick %s is prohibited.", cptr, parv[1]);
      return 0;
  }

  json_object_object_get_ex(json, "flags", &json_flags);
  flagsddb = json_object_get_int(json_flags);
  if (flagsddb) {
      sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                       "%C :*** The nick %s is %s.", cptr, parv[1],
                       flagsddb == 2 ? "suspended" : "prohibited");
      return 0;
  }

  json_object_object_get_ex(json, "pass", &json_pass);
  passddb = (char *)json_object_get_string(json_pass);

  if (!(verify_pass_nick(ddb_key(ddb), passddb, passwd)))
  {
    sendcmdbotto_one(botname, CMD_NOTICE, cptr,
                     "%C :*** Password incorrect.", cptr);
    /* TODO: Contraseña incorrecta* */
    return 0;
  }

  sendto_opmask_butone(0, SNO_SERVKILL,
       "Received KILL message for %C. From %C, Reason: GHOST kill", acptr, cptr);

  sendcmdto_serv_butone(&me, CMD_KILL, acptr, "%C :GHOST session released by %C",
      acptr, cptr);

  if (MyConnect(acptr))
  {
    sendcmdto_one(acptr, CMD_QUIT, cptr, ":Killed (GHOST session "
                  "released by %C)", cptr);
    sendcmdto_one(&me, CMD_KILL, acptr, "%C :GHOST session released by %C",
                  acptr, cptr);
  }
  sendcmdbotto_one(botname, CMD_NOTICE, cptr, "%C :*** %C GHOST session has been "
                   "released.", cptr, acptr);
  exit_client_msg(cptr, acptr, &me, "Killed (GHOST session released by %C)",
                  cptr);
  return 0;
}
