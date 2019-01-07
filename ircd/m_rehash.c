/*
 * IRC-Hispano IRC Daemon, ircd/m_rehash.c
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
 * @brief Handlers for REHASH command.
 */
#include "config.h"

#include "client.h"
#include "ddb.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_ssl.h"
#include "ircd_string.h"
#include "motd.h"
#include "numeric.h"
#include "s_conf.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

/*
 * mo_rehash - oper message handler
 *
 * parv[1] = 'm' flushes the MOTD cache and returns
 * parv[1] = 'l' reopens the log files and returns
 * parv[1] = 'q' to not rehash the resolver (optional)
 * parv[1] = 's' to reload SSL certificates
 * parv[1] = 'b' to reload DDB databases
 */
int mo_rehash(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  int flag = 0;

  if (!HasPriv(sptr, PRIV_REHASH))
    return send_reply(sptr, ERR_NOPRIVILEGES);

  if (parc > 1) { /* special processing */
    if (*parv[1] == 'm') {
      send_reply(sptr, SND_EXPLICIT | RPL_REHASHING, ":Flushing MOTD cache");
      motd_recache(); /* flush MOTD cache */
      return 0;
    } else if (*parv[1] == 'l') {
      send_reply(sptr, SND_EXPLICIT | RPL_REHASHING, ":Reopening log files");
      log_reopen(); /* reopen log files */
      return 0;
#if defined(USE_SSL)
    } else if (*parv[1] == 's') {
      send_reply(sptr, SND_EXPLICIT | RPL_REHASHING, ":Reloading SSL certificates");
      ssl_reinit(0);
      return 0;
#endif
#if defined(DDB)
    } else if (*parv[1] == 'b') {
      send_reply(sptr, SND_EXPLICIT | RPL_REHASHING, ":Reloading DDB databases");
      ddb_reload();
      return 0;
#endif
    } else if (*parv[1] == 'q')
      flag = 2;
  }

  send_reply(sptr, RPL_REHASHING, configfile);
  sendto_opmask_butone(0, SNO_OLDSNO, "%C is rehashing Server config file",
		       sptr);

  log_write(LS_SYSTEM, L_INFO, 0, "REHASH From %#C", sptr);

  return rehash(cptr, flag);
}

