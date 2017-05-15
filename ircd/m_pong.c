/*
 * IRC-Hispano IRC Daemon, ircd/m_pong.c
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
 * @brief Handlers for PONG command.
 */
#include "config.h"

#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "opercmds.h"
#include "s_auth.h"
#include "s_user.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <string.h>
#include <stdlib.h>

/*
 * ms_pong - server message handler
 *
 * parv[0] = sender prefix
 * parv[1] = origin
 * parv[2] = destination
 */
int ms_pong(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char*          destination;
  assert(0 != cptr);
  assert(0 != sptr);
  assert(IsServer(cptr));

  if (parc < 2 || EmptyString(parv[1])) {
    return protocol_violation(sptr,"No Origin on PONG");
  }
  destination = parv[2];
  ClearPingSent(cptr);
  ClearPingSent(sptr);
  cli_lasttime(cptr) = CurrentTime;

  if (parc > 5)
  {
    /* AsLL pong */
    cli_serv(cptr)->asll_rtt = atoi(militime_float(parv[3]));
    cli_serv(cptr)->asll_to = atoi(parv[4]);
    cli_serv(cptr)->asll_from = atoi(militime_float(parv[5]));
    cli_serv(cptr)->asll_last = CurrentTime;
    return 0;
  }
  
  if (EmptyString(destination))
    return 0;
  
  if (*destination == '!')
  {
    /* AsLL ping reply from a non-AsLL server */
    cli_serv(cptr)->asll_rtt = atoi(militime_float(destination + 1));
  }
  else if (0 != ircd_strcmp(destination, cli_name(&me)))
  {
    struct Client* acptr;
    if ((acptr = findNUser(destination))) {
      if (MyUser(acptr))
        sendcmdto_one(sptr, CMD_PONG, acptr, "%C %s", sptr, parv[1]);
      else
        sendcmdto_one(sptr, CMD_PONG, acptr, "%s %C", parv[1], acptr);
    }
  }
  return 0;
}

/*
 * mr_pong - registration message handler
 *
 * parv[0] = sender prefix
 * parv[1] = pong response echo
 * NOTE: cptr is always unregistered here
 */
int mr_pong(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  assert(0 != cptr);
  assert(cptr == sptr);
  assert(!IsRegistered(sptr));

  ClearPingSent(cptr);
  return (parc > 1) ? auth_set_pong(cli_auth(sptr), strtoul(parv[parc - 1], NULL, 10)) : 0;
}

/*
 * m_pong - normal message handler
 *
 * parv[0] = sender prefix
 * parv[1] = origin
 * parv[2] = destination
 */
int m_pong(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  assert(0 != cptr);
  assert(cptr == sptr);

  ClearPingSent(cptr);
  cli_lasttime(cptr) = CurrentTime;
  return 0;
}
