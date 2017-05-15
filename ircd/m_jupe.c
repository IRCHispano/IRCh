/*
 * IRC-Hispano IRC Daemon, ircd/m_jupe.c
 *
 * Copyright (C) 1997-2017 IRC-Hispano Development Team <devel@irc-hispano.es>
 * Copyright (C) 2000 Kevin L. Mitchell <klmitch@mit.edu>
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
 * @brief Handlers for JUPE command.
 */
#include "config.h"

#include "client.h"
#include "jupe.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "match.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_conf.h"
#include "s_misc.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdlib.h>
#include <string.h>

/*
 * ms_jupe - server message handler
 *
 * parv[0] = Send prefix
 *
 * From server:
 *
 * parv[1] = Target: server numeric or *
 * parv[2] = (+|-)<server name>
 * parv[3] = Expiration offset
 * parv[4] = Last modification time
 * parv[5] = Comment
 *
 */
int ms_jupe(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr = 0;
  struct Jupe *ajupe;
  unsigned int flags = 0;
  time_t expire_off, lastmod;
  char *server = parv[2], *target = parv[1], *reason = parv[5];

  if (parc < 6)
    return need_more_params(sptr, "JUPE");

  if (!(target[0] == '*' && target[1] == '\0')) {
    if (!(acptr = FindNServer(target)))
      return 0; /* no such server */

    if (!IsMe(acptr)) { /* manually propagate, since we don't set it */
      sendcmdto_one(sptr, CMD_JUPE, acptr, "%s %s %s %s :%s", target, server,
		    parv[3], parv[4], reason);
      return 0;
    }

    flags |= JUPE_LOCAL;
  }

  if (*server == '-')
    server++;
  else if (*server == '+') {
    flags |= JUPE_ACTIVE;
    server++;
  }

  expire_off = atoi(parv[3]);
  lastmod = atoi(parv[4]);

  ajupe = jupe_find(server);

  if (ajupe) {
    if (JupeIsLocal(ajupe) && !(flags & JUPE_LOCAL)) /* global over local */
      jupe_free(ajupe);
    else if (JupeLastMod(ajupe) < lastmod) { /* new modification */
      if (flags & JUPE_ACTIVE)
	return jupe_activate(cptr, sptr, ajupe, lastmod, flags);
      else
	return jupe_deactivate(cptr, sptr, ajupe, lastmod, flags);
    } else if (JupeLastMod(ajupe) == lastmod || IsBurstOrBurstAck(cptr))
      return 0;
    else
      return jupe_resend(cptr, ajupe); /* other server desynched WRT jupes */
  }

  return jupe_add(cptr, sptr, server, reason, expire_off, lastmod, flags);
}

/*
 * mo_jupe - oper message handler
 *
 * parv[0] = Send prefix
 * parv[1] = [[+|-]<server name>]
 *
 * Local (to me) style:
 *
 * parv[2] = [Expiration offset]
 * parv[3] = [Comment]
 *
 * Global (or remote local) style:
 *
 * parv[2] = [target]
 * parv[3] = [Expiration offset]
 * parv[4] = [Comment]
 *
 */
int mo_jupe(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr = 0;
  struct Jupe *ajupe;
  unsigned int flags = 0;
  time_t expire_off;
  char *server = parv[1], *target = 0, *reason;

  if (parc < 2)
    return jupe_list(sptr, 0);

  if (*server == '+') {
    flags |= JUPE_ACTIVE;
    server++;
  } else if (*server == '-')
    server++;
  else
    return jupe_list(sptr, server);

  if (!feature_bool(FEAT_CONFIG_OPERCMDS))
    return send_reply(sptr, ERR_DISABLED, "JUPE");

  if (parc == 4) {
    expire_off = atoi(parv[2]);
    reason = parv[3];
    flags |= JUPE_LOCAL;
  } else if (parc > 4) {
    target = parv[2];
    expire_off = atoi(parv[3]);
    reason = parv[4];
  } else
    return need_more_params(sptr, "JUPE");

  if (target) {
    if (!(target[0] == '*' && target[1] == '\0')) {
      if (!(acptr = find_match_server(target)))
	return send_reply(sptr, ERR_NOSUCHSERVER, target);

      if (!IsMe(acptr)) { /* manually propagate, since we don't set it */
	if (!HasPriv(sptr, PRIV_JUPE))
	  return send_reply(sptr, ERR_NOPRIVILEGES);

	sendcmdto_one(sptr, CMD_JUPE, acptr, "%C %c%s %s %Tu :%s", acptr,
		      flags & JUPE_ACTIVE ? '+' : '-', server, parv[3],
		      TStime(), reason);
	return 0;
      } else if (!HasPriv(sptr, PRIV_LOCAL_JUPE))
	return send_reply(sptr, ERR_NOPRIVILEGES);

      flags |= JUPE_LOCAL;
    } else if (!HasPriv(sptr, PRIV_JUPE))
      return send_reply(sptr, ERR_NOPRIVILEGES);
  }

  ajupe = jupe_find(server);

  if (ajupe) {
    if (JupeIsLocal(ajupe) && !(flags & JUPE_LOCAL)) /* global over local */
      jupe_free(ajupe);
    else {
      if (flags & JUPE_ACTIVE)
	return jupe_activate(cptr, sptr, ajupe, TStime(), flags);
      else
	return jupe_deactivate(cptr, sptr, ajupe, TStime(), flags);
    }
  }

  return jupe_add(cptr, sptr, server, reason, expire_off, TStime(), flags);
}

/*
 * m_jupe - user message handler
 *
 * parv[0] = Send prefix
 *
 * From user:
 *
 * parv[1] = [<server name>]
 *
 */
int m_jupe(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  if (parc < 2)
    return jupe_list(sptr, 0);

  return jupe_list(sptr, parv[1]);
}
