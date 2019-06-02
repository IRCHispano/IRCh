/*
 * IRC-Hispano IRC Daemon, ircd/m_user.c
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
 * @brief Handlers for the USER command.
 */
#include "config.h"

#include "handlers.h"
#include "client.h"
#include "ircd.h"
#include "ircd_chattr.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_auth.h"
#include "s_conf.h"
#include "s_debug.h"
#include "s_misc.h"
#include "s_user.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * m_user
 *
 * parv[0] = sender prefix
 * parv[1] = username           (login name, account)
 * parv[2] = host name          (ignored)
 * parv[3] = server name        (ignored)
 * parv[4] = users real name info
 */
int m_user(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  char*        username;
  char*        term;
  const char*  info;
  unsigned int mode_request;

  assert(0 != cptr);
  assert(cptr == sptr);

  if (IsServerPort(cptr))
    return exit_client(cptr, cptr, &me, "Use a different port");
  if (IsWebircPort(cptr) && !cli_wline(cptr))
    return exit_client(cptr, cptr, &me, "WebIRC authorization required");
  if (IsProxyPort(cptr) && !find_proxy(cptr))
    return exit_client(cptr, cptr, &me, "Proxy authorization required");

  if (parc < 5)
    return need_more_params(sptr, "USER");

  /* 
   * Copy parameters into better documenting variables
   *
   * ignore host part if u@h
   */
  if (!EmptyString(parv[1])) {
    if ((username = strchr(parv[1], '@')))
      *username = '\0';
    username = parv[1];
  }
  else
    username = "NoUser";

  if ((mode_request = strtoul(parv[2], &term, 10)) != 0
      && term != NULL && *term == '\0')
  {
    char *invisible[4] = { NULL, NULL, "+i", NULL };
    char *wallops[4] = { NULL, NULL, "+w" , NULL };
    /* These bitmask values are codified in RFC 2812, showing
     * ... well, something that is probably best not said.
     */
    if (mode_request & 8)
      set_user_mode(cptr, sptr, 3, invisible, ALLOWMODES_ANY);
    if (mode_request & 4)
      set_user_mode(cptr, sptr, 3, wallops, ALLOWMODES_ANY);
  }
  else if (parv[2][0] == '+')
  {
    char *user_modes[4];
    user_modes[0] = NULL;
    user_modes[1] = NULL;
    user_modes[2] = parv[2];
    user_modes[3] = NULL;
    set_user_mode(cptr, sptr, 3, user_modes, ALLOWMODES_ANY);
  }

  info     = (EmptyString(parv[4])) ? "No Info" : parv[4];

  return auth_set_user(cli_auth(cptr), username, parv[2], parv[3], info);
}

