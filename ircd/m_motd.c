/*
 * IRC-Hispano IRC Daemon, ircd/m_motd.c
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
 * @brief Handlers for MOTD command.
 */
#include "config.h"

#include "client.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "match.h"
#include "motd.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_conf.h"
#include "class.h"
#include "s_user.h"
#include "send.h"

#include <stdlib.h>
/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * m_motd - generic message handler
 *
 * parv[0] - sender prefix
 * parv[1] - servername
 *
 * modified 30 mar 1995 by flux (cmlambertus@ucdavis.edu)
 * T line patch - display motd based on hostmask
 * modified again 7 sep 97 by Ghostwolf with code and ideas
 * stolen from comstud & Xorath.  All motd files are read into
 * memory in read_motd() in s_conf.c
 *
 * Now using the motd_* family of functions defined in motd.c
 */
int m_motd(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  if (hunt_server_cmd(sptr, CMD_MOTD, cptr, feature_int(FEAT_HIS_REMOTE), "%C", 1,
		      parc, parv) != HUNTED_ISME)
    return 0;

  return motd_send(sptr);
}

/*
 * ms_motd - server message handler
 *
 * parv[0] - sender prefix
 * parv[1] - servername
 *
 * modified 30 mar 1995 by flux (cmlambertus@ucdavis.edu)
 * T line patch - display motd based on hostmask
 * modified again 7 sep 97 by Ghostwolf with code and ideas
 * stolen from comstud & Xorath.  All motd files are read into
 * memory in read_motd() in s_conf.c
 *
 * Now using the motd_* family of functions defined in motd.c
 */
int ms_motd(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  if (hunt_server_cmd(sptr, CMD_MOTD, cptr, 0, "%C", 1, parc, parv) !=
      HUNTED_ISME)
    return 0;

  return motd_send(sptr);
}
