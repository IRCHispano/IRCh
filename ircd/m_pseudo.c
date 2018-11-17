/*
 * IRC-Hispano IRC Daemon, ircd/m_pseudo.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2002-2003 Zoot <zoot@gamesurge.net>
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
 * @brief Handlers for PSEUDO command.
 */
#include "config.h"

#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_relay.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "ircd_snprintf.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_conf.h"
#include "s_user.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * m_pseudo - generic service message handler
 *
 * parv[0] = sender prefix
 * parv[1] = service mapping (s_map * disguised as char *)
 * parv[2] = message
 */
int m_pseudo(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char *text, buffer[BUFSIZE];
  struct s_map *map;
  struct nick_host *nh;

  assert(0 != cptr);
  assert(cptr == sptr);
  assert(0 != cli_user(sptr));

  /* By default, relay the message straight through. */
  text = parv[parc - 1];

  if (parc < 3 || EmptyString(text))
    return send_reply(sptr, ERR_NOTEXTTOSEND);

  /* HACK! HACK! HACK! HACK! Yes. It's icky, but
   * it's the only way. */
  map = (struct s_map *)parv[1];
  assert(0 != map);

  if (map->prepend) {
    ircd_snprintf(0, buffer, sizeof(buffer) - 1, "%s%s", map->prepend, text);
    buffer[sizeof(buffer) - 1] = 0;
    text = buffer;
  }

  for (nh = map->services; nh; nh = nh->next) {
    struct Client *target, *server;

    if (NULL == (server = FindServer(nh->nick + nh->nicklen + 1)))
      continue;
    nh->nick[nh->nicklen] = '\0';
    if ((NULL == (target = FindUser(nh->nick)))
        || (server != cli_user(target)->server))
      continue;
    nh->nick[nh->nicklen] = '@';
    relay_directed_message(sptr, nh->nick, nh->nick + nh->nicklen, text);
    return 0;
  }

  return send_reply(sptr, ERR_SERVICESDOWN, map->name);
}
