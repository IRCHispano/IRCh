/*
 * IRC-Hispano IRC Daemon, ircd/m_rpong.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 1996 Carlo Wood <carlo@runaway.xs4all.nl>
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
 * @brief Handlers for RPONG command.
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
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * ms_rpong - server message handler
 * -- by Run too :)
 *
 * parv[0] = sender prefix
 * parv[1] = from pinged server: start server; from start server: sender
 * parv[2] = from pinged server: sender; from start server: pinged server
 * parv[3] = pingtime in ms
 * parv[4] = client info (for instance start time)
 */
int ms_rpong(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;

  if (!IsServer(sptr))
    return 0;

  if (parc < 5) {
    /*
     * PROTOCOL ERROR
     */
    return need_more_params(sptr, "RPONG");
  }
  if (parc == 6) {
    /*
     * from pinged server to source server
     */
    if (!(acptr = FindServer(parv[1])) && !(acptr = FindNServer(parv[1])))
      return 0;
   
    if (IsMe(acptr)) {
      if (!(acptr = findNUser(parv[2])))
        return 0;

      sendcmdto_one(&me, CMD_RPONG, acptr, "%C %s %s :%s", acptr, cli_name(sptr),
		    militime(parv[3], parv[4]), parv[5]);
    } else
      sendcmdto_one(sptr, CMD_RPONG, acptr, "%s %s %s %s :%s", parv[1],
		    parv[2], parv[3], parv[4], parv[5]);
  } else {
    /*
     * returned from source server to client
     */
    if (!(acptr = findNUser(parv[1])))
      return 0;

    sendcmdto_one(sptr, CMD_RPONG, acptr, "%C %s %s :%s", acptr, parv[2],
		  parv[3], parv[4]);
  }
  return 0;
}
