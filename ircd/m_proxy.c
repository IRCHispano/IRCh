/*
 * IRC-Hispano IRC Daemon, ircd/m_proxy.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2015, 2019 Toni Garcia (zoltan) <toni@tonigarcia.es>
 * Copyright (C) 2015 Victor Roman Archidona (daijo) <vronam@victorroman.es>
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
 * @brief Handlers for PROXY command.
 */
#include "config.h"

#include "client.h"
#include "ircd.h"
#include "ircd_features.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "s_auth.h"
#include "s_conf.h"
#include "s_misc.h"

#include <string.h>

//registration
/** Handle a message handler for local clients
 * @param[in] cptr Client that sent us the message.
 * @param[in] sptr Original source of message.
 * @param[in] parc Number of arguments.
 * @param[in] parv Argument vector.
 *
 * parv[0] = sender prefix
 * parv[1] = protocol (TCP4, TCP6 or UNKNOWN)
 * parv[2] = direccion ip del cliente origen
 * parv[3] = direccion ip del servidor proxy
 * parv[4] = puerto origen del cliente origen
 * parv[5] = puerto destino del servidor proxy
 *
 * Ver: http://www.haproxy.org/download/1.5/doc/proxy-protocol.txt
 *
 */
int mr_proxy(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  if (!IsProxyPort(cptr))
    return exit_client(cptr, sptr, &me, "Use a different port");

  if (parc < 6)
    return need_more_params(sptr, "PROXY");

  if (!find_proxy(cptr))
    return exit_client(cptr, sptr, &me, "PROXY Not authorized from your address");

  if (!strcmp(parv[1], "UNKNOWN"))
    return exit_client(cptr, sptr, &me, "PROXY UNKNOWN protocol");

  if (strcmp(parv[1], "TCP4") && strcmp(parv[1], "TCP6"))
    return exit_client(cptr, sptr, &me, "PROXY Erroneous protocol");

  /* Treat client as a normally connecting user from now on. */
  cli_status(sptr) = STAT_UNKNOWN_USER;

  int res = auth_spoof_user(cli_auth(cptr), NULL, parv[2], parv[2]);
  if (res > 0)
    return exit_client(cptr, sptr, &me, "PROXY invalid spoof");
  return res;
}
