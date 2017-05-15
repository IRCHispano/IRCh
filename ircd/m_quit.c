/*
 * IRC-Hispano IRC Daemon, ircd/m_quit.c
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
 * @brief Handlers for QUIT command.
 */
#include "config.h"

#include "channel.h"
#include "client.h"
#include "ircd.h"
#include "ircd_log.h"
#include "ircd_string.h"
#include "struct.h"
#include "s_misc.h"
#include "ircd_reply.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <string.h>

/*
 * m_quit - client message handler 
 *
 * parv[0]        = sender prefix
 * parv[parc - 1] = comment
 */
int m_quit(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  assert(0 != cptr);
  assert(0 != sptr);
  assert(cptr == sptr);

  if (cli_user(sptr)) {
    struct Membership* chan;
    for (chan = cli_user(sptr)->channel; chan; chan = chan->next_channel) {
        if (!IsZombie(chan) && !IsDelayedJoin(chan) && !member_can_send_to_channel(chan, 0))
        return exit_client(cptr, sptr, sptr, "Signed off");
    }
  }
  if (parc > 1 && !BadPtr(parv[parc - 1]))
    return exit_client_msg(cptr, sptr, sptr, "Quit: %s", parv[parc - 1]);
  else
    return exit_client(cptr, sptr, sptr, "Quit");
}


/*
 * ms_quit - server message handler
 *
 * parv[0] = sender prefix
 * parv[parc - 1] = comment
 */
int ms_quit(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  assert(0 != sptr);
  assert(parc > 0);
  if (IsServer(sptr)) {
  	protocol_violation(sptr,"Server QUIT, not SQUIT?");
  	return 0;
  }
  /*
   * ignore quit from servers
   */
  return exit_client(cptr, sptr, sptr, parv[parc - 1]);
}
