/*
 * IRC-Hispano IRC Daemon, ircd/monitor.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2017 Toni Garcia (zoltan) <toni@tonigarcia.es>
 * Copyright (C) 2002 Toni Garcia (zoltan) <toni@tonigarcia.es> (WATCH)
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

#include "monitor.h"
#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_alloc.h"
#include "ircd_log.h"
#include "ircd_snprintf.h"
#include "ircd_string.h"
#include "list.h"
#include "numeric.h"
#include "send.h"
#include "struct.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <string.h>

/*
 * MONITOR FUNCTIONS
 *
 * MONITOR_LIST
 * |
 * |-mptr1-|- cptr1
 * |       |- cptr2
 * |       |- cptr3
 * |
 * |-mptr2-|- cptr2
 *         |- cptr1
 *
 * LINKS in the Client lists.
 *
 * cptr1            cptr2           cptr3
 * |- mptr1(nickA)  |-mptr1(nickA)  |-mptr1(nickA)
 * |- mptr2(nickB)  |-mptr2(nickB)
 *
 * The operation is based on the WATCH of Bahamut and UnrealIRCD.
 *
 * 2002/05/20 zoltan <toni@tonigarcia.es>
 *
 * Adapted for MONITOR IRCv3.x Standard:
 * 2017/12/01 zoltan <toni@tonigarcia.es>
 *
 */

/** Count of allocated Monitor structures. */
static int monitorCount = 0;

/** Reserve an entrance in the Monitor list.
 * @param[in] nick Nick to monitor.
 */
struct
Monitor *make_monitor(const char *nick)
{
  struct Monitor *mptr;

  mptr = (struct Monitor *)MyMalloc(sizeof(struct Monitor));
  assert(0 != mptr);

  /*
   * NOTE: Do not remove this, a lot of code depends on the entire
   * structure being zeroed out
   */
  memset(mptr, 0, sizeof(struct Monitor));	/* All variables are 0 by default */

  DupString(mo_nick(mptr), nick);

  hAddMonitor(mptr);
  monitorCount++;

  return (mptr);
}

/** Release an entrace of the Monitor list.
 *  @param[in] mptr Monitor structure to release.
 */
void
free_monitor(struct Monitor *mptr)
{

  hRemMonitor(mptr);
  MyFree(mo_nick(mptr));
  MyFree(mptr);

  monitorCount--;
}

/** Find number of Monitor structs allocated and memory used by them.
 * @param[out] count_out Receives number of Monitor structs allocated.
 * @param[out] bytes_out Receives number of bytes used by Monitor structs.
 */
void
monitor_count_memory(int* count_out, size_t* bytes_out)
{
  assert(0 != count_out);
  assert(0 != bytes_out);
  *count_out = monitorCount;
  *bytes_out = monitorCount * sizeof(struct Monitor);
}

/** Notifies any clients monitoring the nick that it has connected to the network.
 * @param[in] cptr Client who has just connected.
 */
void
monitor_notify(struct Client *cptr, int raw)
{
  struct Monitor *mptr;
  struct SLink *lp;
  struct MsgBuf *mb;
  char monbuf[NICKLEN+USERLEN+HOSTLEN+3];

  mptr = FindMonitor(cli_name(cptr));
  if (!mptr)
    return; /* Not this in some notify */

  if (raw == RPL_MONONLINE)
    ircd_snprintf(0, monbuf, sizeof(monbuf), "%s!%s@%s", cli_name(cptr),
                  IsUser(cptr) ? cli_user(cptr)->username : "<N/A>",
                  IsUser(cptr) ? cli_user(cptr)->host : "<N/A>");
  else
    ircd_snprintf(0, monbuf, sizeof(monbuf), "%s", cli_name(cptr));

  /* build buffer */
  mb = msgq_make(0, rpl_str(raw), cli_name(&me), "*", monbuf);

  /*
   * Sent the warning to all the users who
   * have it in notify.
   */
  for (lp = mo_monitor(mptr); lp; lp = lp->next)
  {
    /* send it to the user */
    send_buffer(lp->value.cptr, mb, 0);
  }

  msgq_clean(mb);
}

/*
 * add_nick_monitor()
 *
 * Add nick to the user monitor list.
 */
int
monitor_add_nick(struct Client *cptr, char *nick)
{
  struct Monitor *mptr;
  struct SLink *lp;

  /*
   * If not exist, create the registry.
   */
  if (!(mptr = FindMonitor(nick)))
  {
    mptr = make_monitor(nick);
    if (!mptr)
      return 0;
  }

  /*
   * Find if it already has it in monitor.
   */
  if ((lp = mo_monitor(mptr)))
  {
    while (lp && (lp->value.cptr != cptr))
      lp = lp->next;
  }

  /*
   * Not this, then add it.
   */
  if (!lp)
  {
    /*
     * Link monitor to cptr
     */
    lp = mo_monitor(mptr);
    mo_monitor(mptr) = make_link();
    memset(mo_monitor(mptr), 0, sizeof(struct SLink));
    mo_monitor(mptr)->value.cptr = cptr;
    mo_monitor(mptr)->next = lp;

    /*
     * Link client->user to monitor
     */
    lp = make_link();
    memset(lp, 0, sizeof(struct SLink));
    lp->next = cli_user(cptr)->monitor;
    lp->value.mptr = mptr;
    cli_user(cptr)->monitor = lp;
    cli_user(cptr)->monitors++;

  }
  return 0;
}


/*
 * del_nick_monitor()
 *
 * Delete a nick of the user monitor list.
 */
int
monitor_del_nick(struct Client *cptr, char *nick)
{
  struct Monitor *mptr;
  struct SLink *lp, *lptmp = 0;

  mptr = FindMonitor(nick);
  if (!mptr)
    return 0;			/* Not this in any list */

  /*
   * Find for in the link cptr->user to monitor
   */
  if ((lp = mo_monitor(mptr)))
  {
    while (lp && (lp->value.cptr != cptr))
    {
      lptmp = lp;
      lp = lp->next;
    }
  }

  if (!lp)
    return 0;

  if (!lptmp)
    mo_monitor(mptr) = lp->next;
  else
    lptmp->next = lp->next;

  free_link(lp);

  /*
   * For in the link monitor to cptr->user
   */
  lptmp = lp = 0;
  if ((lp = cli_user(cptr)->monitor))
  {
    while (lp && (lp->value.mptr != mptr))
    {
      lptmp = lp;
      lp = lp->next;
    }
  }

  assert(0 != lp);

  if (!lptmp)
    cli_user(cptr)->monitor = lp->next;
  else
    lptmp->next = lp->next;

  free_link(lp);

  /*
   * If it were the onlu associate to nick
   * delete registry in the monitor table.
   */
  if (!mo_monitor(mptr))
    free_monitor(mptr);

  /* Update count */
  cli_user(cptr)->monitors--;

  return 0;

}


/*
 * del_list_monitor()
 *
 * Delete all the monitor list.
 * When it execute a /WATCH C or it leaves the IRC.
 */
int
monitor_list_clean(struct Client *cptr)
{
  struct SLink *lp, *lp2, *lptmp;
  struct Monitor *mptr;

  if (!(lp = cli_user(cptr)->monitor))
    return 0;			/* Id had the empty list */

  /*
   * Loop of links cptr->user to monitor.
   */
  while (lp)
  {
    mptr = lp->value.mptr;
    lptmp = 0;
    for (lp2 = mo_monitor(mptr); lp2 && (lp2->value.cptr != cptr); lp2 = lp2->next)
      lptmp = lp2;


    assert(0 != lp2);

    if (!lptmp)
      mo_monitor(mptr) = lp2->next;
    else
      lptmp->next = lp2->next;
    free_link(lp2);

    /*
     * If it were the onlu associate to nick
     * delete registry in the monitor table.
     */
    if (!mo_monitor(mptr))
      free_monitor(mptr);

    lp2 = lp;
    lp = lp->next;
    free_link(lp2);
  }

  /* Update count */
  cli_user(cptr)->monitor = 0;
  cli_user(cptr)->monitors = 0;

  return 0;
}
