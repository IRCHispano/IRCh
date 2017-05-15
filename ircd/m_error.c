/*
 * IRC-Hispano IRC Daemon, ircd/m_error.c
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
 * @brief Handlers for ERROR command.
 */
#include "config.h"

#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_alloc.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_debug.h"
#include "s_misc.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <string.h>

/*
 * mr_error - unregistered client message handler
 *
 * parv[0] = sender prefix
 * parv[parc-1] = text
 */
int mr_error(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char *para;

  if (!IsHandshake(cptr) && !IsConnecting(cptr))
    return 0; /* ignore ERROR from regular clients */

  para = (parc > 1 && *parv[parc - 1] != '\0') ? parv[parc - 1] : "<>";

  Debug((DEBUG_ERROR, "Received ERROR message from %s: %s", cli_name(sptr), para));

  if (cptr == sptr)
    sendto_opmask_butone(0, SNO_OLDSNO, "ERROR :from %C -- %s", cptr, para);
  else
    sendto_opmask_butone(0, SNO_OLDSNO, "ERROR :from %C via %C -- %s", sptr,
			 cptr, para);

  if (cli_serv(sptr))
  {
    MyFree(cli_serv(sptr)->last_error_msg);
    DupString(cli_serv(sptr)->last_error_msg, para);
  }

  return 0;
}

/*
 * ms_error - server message handler
 *
 * parv[0] = sender prefix
 * parv[parc-1] = text
 */
int ms_error(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  const char *para;

  para = (parc > 1 && *parv[parc - 1] != '\0') ? parv[parc - 1] : "<>";

  Debug((DEBUG_ERROR, "Received ERROR message from %s: %s", cli_name(sptr), para));

  if (cptr == sptr)
    sendto_opmask_butone(0, SNO_OLDSNO, "ERROR :from %C -- %s", cptr, para);
  else
    sendto_opmask_butone(0, SNO_OLDSNO, "ERROR :from %C via %C -- %s", sptr,
			 cptr, para);

  if (cli_serv(sptr))
  {
    MyFree(cli_serv(sptr)->last_error_msg);
    DupString(cli_serv(sptr)->last_error_msg, para);
  }

  return 0;
}
