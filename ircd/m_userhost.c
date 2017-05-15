/*
 * IRC-Hispano IRC Daemon, ircd/m_userhost.c
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
 * @brief Handlers for the USERHOST command.
 */
#include "config.h"

#include "client.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "msgq.h"
#include "numeric.h"
#include "s_user.h"
#include "struct.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */

static void userhost_formatter(struct Client* cptr, struct Client *sptr, struct MsgBuf* mb)
{
  assert(IsUser(cptr));
  msgq_append(0, mb, "%s%s=%c%s@%s", cli_name(cptr),
              SeeOper(sptr,cptr) ? "*" : "",
	      cli_user(cptr)->away ? '-' : '+', cli_user(cptr)->username,
	      /* Do not *EVER* change this to give opers the real host.
	       * Too many scripts rely on this data and can inadvertently
	       * publish the user's real host, thus breaking the security
	       * of +x.  If an oper wants the real host, he should go to
	       * /whois to get it.
	       */
	      HasHiddenHost(cptr) && (sptr != cptr) ?
	      cli_user(cptr)->host : cli_user(cptr)->realhost);
}

/*
 * m_userhost - generic message handler
 */
int m_userhost(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  assert(0 != cptr);
  assert(sptr == cptr);

  if (parc < 2)
    return need_more_params(sptr, "USERHOST");

  send_user_info(sptr, parv[1], RPL_USERHOST, userhost_formatter);
  return 0;
}
