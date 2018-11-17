/*
 * IRC-Hispano IRC Daemon, ircd/m_starttls.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2013 Matthew Beeching (Jobe)
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
#include "config.h"

#include "client.h"
#include "dbuf.h"
#include "handlers.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "numeric.h"
#include "numnicks.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

int m_starttls(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
#if !defined(USE_SSL)
  return m_unregistered(cptr, sptr, parc, parv);
#else
  if (cli_socket(sptr).ssl || IsSSL(sptr))
    return send_reply(sptr, ERR_STARTTLS, "STARTTLS failed. Already using TLS.");

  DBufClear(&(cli_recvQ(sptr)));

  send_reply(sptr, RPL_STARTTLS);
  SetStartTLS(sptr);

  send_queued(sptr);

  if (!ssl_starttls(sptr)) {
    ClearStartTLS(sptr);
    return send_reply(sptr, ERR_STARTTLS, "STARTTLS failed.");
  }

  SetSSL(sptr);

  if (ssl_is_init_finished(cli_socket(cptr).ssl))
  {
    char *sslfp = ssl_get_fingerprint(cli_socket(cptr).ssl);
    if (sslfp)
      ircd_strncpy(cli_sslclifp(cptr), sslfp, BUFSIZE+1);
  }
#endif
  return 0;
}
