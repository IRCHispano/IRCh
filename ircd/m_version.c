/*
 * IRC-Hispano IRC Daemon, ircd/m_version.c
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
 * @brief Handlers for the VERSION command.
 */
#include "config.h"

#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_snprintf.h"
#include "ircd_string.h"
#include "match.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_debug.h"
#include "s_user.h"
#include "send.h"
#include "supported.h"
#include "version.h"

#if defined(USE_SSL)
#include "openssl/ssl.h"
#endif

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * m_version - generic message handler
 *
 *   parv[0] = sender prefix
 *   parv[1] = servername
 */
int m_version(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  if (hunt_server_cmd(sptr, CMD_VERSION, cptr, feature_int(FEAT_HIS_REMOTE),
                                                           ":%C", 1,
                                                           parc, parv)
                      == HUNTED_ISME)
  {
    send_reply(sptr, RPL_VERSION, version, debugmode, cli_name(&me),
	       debug_serveropts());

#if defined(USE_SSL)
    if (IsAnOper(sptr))
#ifdef DEBUGMODE
      sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :Headers: %s", sptr, OPENSSL_VERSION_TEXT);
#endif
      sendcmdto_one(&me, CMD_NOTICE, sptr, "%C :Library: %s", sptr, SSLeay_version(SSLEAY_VERSION));
#endif

    if (MyUser(sptr))
      send_supported(sptr);
  }

  return 0;
}
