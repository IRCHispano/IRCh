/*
 * IRC-Hispano IRC Daemon, ircd/m_links.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
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
 * @brief Handlers for LINKS command.
 */
#include "config.h"

#include "client.h"
#include "ircd.h"
#include "ircd_defs.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "match.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_user.h"
#include "send.h"
#include "struct.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * m_links - generic message handler
 *
 * parv[0] = sender prefix
 * parv[1] = servername mask
 *
 * or
 *
 * parv[0] = sender prefix
 * parv[1] = server to query
 * parv[2] = servername mask
 */
int m_links(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  char *mask;
  struct Client *acptr;

  if (feature_bool(FEAT_HIS_LINKS) && !IsAnOper(sptr))
  {
    send_reply(sptr, RPL_ENDOFLINKS, parc < 2 ? "*" : parv[1]);
    sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :%s %s", sptr,
                  "/LINKS has been disabled, from CFV-165.  Visit ", 
                  feature_str(FEAT_HIS_URLSERVERS));
    return 0;
  }

  if (parc > 2)
  {
    if (hunt_server_cmd(sptr, CMD_LINKS, cptr, 1, "%C :%s", 1, parc, parv) !=
        HUNTED_ISME)
      return 0;
    mask = parv[2];
  }
  else
    mask = parc < 2 ? 0 : parv[1];

  for (acptr = GlobalClientList, collapse(mask); acptr; acptr = cli_next(acptr))
  {
    if (!IsServer(acptr) && !IsMe(acptr))
      continue;
    if (!BadPtr(mask) && match(mask, cli_name(acptr)))
      continue;
    send_reply(sptr, RPL_LINKS, cli_name(acptr), cli_name(cli_serv(acptr)->up),
        cli_hopcount(acptr), cli_serv(acptr)->prot,
        ((cli_info(acptr))[0] ? cli_info(acptr) : "(Unknown Location)"));
  }

  send_reply(sptr, RPL_ENDOFLINKS, BadPtr(mask) ? "*" : mask);

  return 0;
}

/*
 * ms_links - server message handler
 *
 * parv[0] = sender prefix
 * parv[1] = servername mask
 *
 * or
 *
 * parv[0] = sender prefix
 * parv[1] = server to query
 * parv[2] = servername mask
 */
int
ms_links(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
 {
   char *mask;
   struct Client *acptr;

   if (parc > 2)
   {
     if (hunt_server_cmd(sptr, CMD_LINKS, cptr, 1, "%C :%s", 1, parc, parv) !=
         HUNTED_ISME)
       return 0;
     mask = parv[2];
   }
   else
     mask = parc < 2 ? 0 : parv[1];
 
   for (acptr = GlobalClientList, collapse(mask); acptr; acptr = cli_next(acptr))
   {
     if (!IsServer(acptr) && !IsMe(acptr))
       continue;
     if (!BadPtr(mask) && match(mask, cli_name(acptr)))
       continue;
     send_reply(sptr, RPL_LINKS, cli_name(acptr), cli_name(cli_serv(acptr)->up),
                cli_hopcount(acptr), cli_serv(acptr)->prot,
                ((cli_info(acptr))[0] ? cli_info(acptr) : "(Unknown Location)"));
   }
 
   send_reply(sptr, RPL_ENDOFLINKS, BadPtr(mask) ? "*" : mask);
   return 0;
 }
