/*
 * IRC - Internet Relay Chat, ircd/s_user.c (formerly ircd/s_msg.c)
 * Copyright (C) 1990 Jarkko Oikarinen and
 *                    University of Oulu, Computing Center
 *
 * See file AUTHORS in IRC package for additional names of
 * the programmers.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */
#include "s_user.h"
#include "IPcheck.h"
#include "channel.h"
#include "class.h"
#include "client.h"
#include "gline.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_alloc.h"
#include "ircd_chattr.h"
#include "ircd_features.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_string.h"
#include "list.h"
#include "match.h"
#include "motd.h"
#include "msg.h"
#include "msgq.h"
#include "numeric.h"
#include "numnicks.h"
#include "parse.h"
#include "querycmds.h"
#include "random.h"
#include "s_bsd.h"
#include "s_conf.h"
#include "s_debug.h"
#include "s_misc.h"
#include "s_serv.h" /* max_client_count */
#include "send.h"
#include "sprintf_irc.h"
#include "struct.h"
#include "support.h"
#include "supported.h"
#include "sys.h"
#include "userload.h"
#include "version.h"
#include "whowas.h"

#include "handlers.h" /* m_motd and m_lusers */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


static int userCount = 0;

/*
 * 'make_user' add's an User information block to a client
 * if it was not previously allocated.
 */
struct User *make_user(struct Client *cptr)
{
  assert(0 != cptr);

  if (!cli_user(cptr)) {
    cli_user(cptr) = (struct User*) MyMalloc(sizeof(struct User));
    assert(0 != cli_user(cptr));

    /* All variables are 0 by default */
    memset(cli_user(cptr), 0, sizeof(struct User));
#ifdef  DEBUGMODE
    ++userCount;
#endif
    cli_user(cptr)->refcnt = 1;
  }
  return cli_user(cptr);
}

/*
 * free_user
 *
 * Decrease user reference count by one and release block, if count reaches 0.
 */
void free_user(struct User* user)
{
  assert(0 != user);
  assert(0 < user->refcnt);

  if (--user->refcnt == 0) {
    if (user->away)
      MyFree(user->away);
    /*
     * sanity check
     */
    assert(0 == user->joined);
    assert(0 == user->invited);
    assert(0 == user->channel);

    MyFree(user);
#ifdef  DEBUGMODE
    --userCount;
#endif
  }
}

void user_count_memory(size_t* count_out, size_t* bytes_out)
{
  assert(0 != count_out);
  assert(0 != bytes_out);
  *count_out = userCount;
  *bytes_out = userCount * sizeof(struct User);
}


/*
 * next_client
 *
 * Local function to find the next matching client. The search
 * can be continued from the specified client entry. Normal
 * usage loop is:
 *
 * for (x = client; x = next_client(x,mask); x = x->next)
 *     HandleMatchingClient;
 *
 */
struct Client *next_client(struct Client *next, const char* ch)
{
  struct Client *tmp = next;

  if (!tmp)
    return NULL;

  next = FindClient(ch);
  next = next ? next : tmp;
  if (cli_prev(tmp) == next)
    return NULL;
  if (next != tmp)
    return next;
  for (; next; next = cli_next(next))
    if (!match(ch, cli_name(next)))
      break;
  return next;
}

/*
 * hunt_server
 *
 *    Do the basic thing in delivering the message (command)
 *    across the relays to the specific server (server) for
 *    actions.
 *
 *    Note:   The command is a format string and *MUST* be
 *            of prefixed style (e.g. ":%s COMMAND %s ...").
 *            Command can have only max 8 parameters.
 *
 *    server  parv[server] is the parameter identifying the
 *            target server.
 *
 *    *WARNING*
 *            parv[server] is replaced with the pointer to the
 *            real servername from the matched client (I'm lazy
 *            now --msa).
 *
 *    returns: (see #defines)
 */
int hunt_server_cmd(struct Client *from, const char *cmd, const char *tok,
                    struct Client *one, int MustBeOper, const char *pattern,
                    int server, int parc, char *parv[])
{
  struct Client *acptr;
  char *to;

  /* Assume it's me, if no server or an unregistered client */
  if (parc <= server || EmptyString((to = parv[server])) || IsUnknown(from))
    return (HUNTED_ISME);

  /* Make sure it's a server */
  if (MyUser(from)) {
    /* Make sure it's a server */
    if (!strchr(to, '*')) {
      if (0 == (acptr = FindClient(to)))
        return HUNTED_NOSUCH;

      if (cli_user(acptr))
        acptr = cli_user(acptr)->server;
    } else if (!(acptr = find_match_server(to))) {
      send_reply(from, ERR_NOSUCHSERVER, to);
      return (HUNTED_NOSUCH);
    }
  } else if (!(acptr = FindNServer(to)))
    return (HUNTED_NOSUCH);        /* Server broke off in the meantime */

  if (IsMe(acptr))
    return (HUNTED_ISME);

  if (MustBeOper && !IsPrivileged(from)) {
    send_reply(from, ERR_NOPRIVILEGES);
    return HUNTED_NOSUCH;
  }

  assert(!IsServer(from));

  parv[server] = (char *) acptr; /* HACK! HACK! HACK! ARGH! */

  sendcmdto_one(from, cmd, tok, acptr, pattern, parv[1], parv[2], parv[3],
                parv[4], parv[5], parv[6], parv[7], parv[8]);

  return (HUNTED_PASS);
}

/*
 * 'do_nick_name' ensures that the given parameter (nick) is really a proper
 * string for a nickname (note, the 'nick' may be modified in the process...)
 *
 * RETURNS the length of the final NICKNAME (0, if nickname is invalid)
 *
 * Nickname characters are in range 'A'..'}', '_', '-', '0'..'9'
 *  anything outside the above set will terminate nickname.
 * In addition, the first character cannot be '-' or a Digit.
 *
 * Note:
 *  The '~'-character should be allowed, but a change should be global,
 *  some confusion would result if only few servers allowed it...
 */
int do_nick_name(char* nick)
{
  char* ch  = nick;
  char* end = ch + NICKLEN;
  assert(0 != ch);

  if (*ch == '-' || IsDigit(*ch))        /* first character in [0..9-] */
    return 0;

  for ( ; (ch < end) && *ch; ++ch)
    if (!IsNickChar(*ch))
      break;

  *ch = '\0';

  return (ch - nick);
}

/*
 * clean_user_id
 *
 * Copy `source' to `dest', replacing all occurances of '~' and characters that
 * are not `isIrcUi' by an underscore.
 * Copies at most USERLEN - 1 characters or up till the first control character.
 * If `tilde' is true, then a tilde is prepended to `dest'.
 * Note that `dest' and `source' can point to the same area or to different
 * non-overlapping areas.
 */
static char *clean_user_id(char *dest, char *source, int tilde)
{
  char ch;
  char *d = dest;
  char *s = source;
  int rlen = USERLEN;

  ch = *s++;                        /* Store first character to copy: */
  if (tilde)
  {
    *d++ = '~';                        /* If `dest' == `source', then this overwrites `ch' */
    --rlen;
  }
  while (ch && !IsCntrl(ch) && rlen--)
  {
    char nch = *s++;        /* Store next character to copy */
    *d++ = IsUserChar(ch) ? ch : '_';        /* This possibly overwrites it */
    if (nch == '~')
      ch = '_';
    else
      ch = nch;
  }
  *d = 0;
  return dest;
}

/*
 * register_user
 *
 * This function is called when both NICK and USER messages
 * have been accepted for the client, in whatever order. Only
 * after this the USER message is propagated.
 *
 * NICK's must be propagated at once when received, although
 * it would be better to delay them too until full info is
 * available. Doing it is not so simple though, would have
 * to implement the following:
 *
 * 1) user telnets in and gives only "NICK foobar" and waits
 * 2) another user far away logs in normally with the nick
 *    "foobar" (quite legal, as this server didn't propagate it).
 * 3) now this server gets nick "foobar" from outside, but
 *    has already the same defined locally. Current server
 *    would just issue "KILL foobar" to clean out dups. But,
 *    this is not fair. It should actually request another
 *    nick from local user or kill him/her...
 */
int register_user(struct Client *cptr, struct Client *sptr,
                  const char *nick, char *username, struct Gline *agline)
{
  struct ConfItem* aconf;
  char*            parv[3];
  char*            tmpstr;
  char*            tmpstr2;
  char             c = 0;    /* not alphanum */
  char             d = 'a';  /* not a digit */
  short            upper = 0;
  short            lower = 0;
  short            pos = 0;
  short            leadcaps = 0;
  short            other = 0;
  short            digits = 0;
  short            badid = 0;
  short            digitgroups = 0;
  struct User*     user = cli_user(sptr);
  char             ip_base64[8];
  char             featurebuf[512];

  user->last = CurrentTime;
  parv[0] = cli_name(sptr);
  parv[1] = parv[2] = NULL;

  if (MyConnect(sptr))
  {
    static time_t last_too_many1;
    static time_t last_too_many2;

    assert(cptr == sptr);
    switch (conf_check_client(sptr))
    {
      case ACR_OK:
        break;
      case ACR_NO_AUTHORIZATION:
        sendto_opmask_butone(0, SNO_UNAUTH, "Unauthorized connection from %s.",
                             get_client_name(sptr, HIDE_IP));
        ++ServerStats->is_ref;
        return exit_client(cptr, sptr, &me,
                           "No Authorization - use another server");
      case ACR_TOO_MANY_IN_CLASS:
        if (CurrentTime - last_too_many1 >= (time_t) 60)
        {
          last_too_many1 = CurrentTime;
          sendto_opmask_butone(0, SNO_TOOMANY, "Too many connections in "
                               "class for %s.",
                               get_client_name(sptr, HIDE_IP));
        }
        ++ServerStats->is_ref;
        IPcheck_connect_fail(cli_ip(sptr));
        return exit_client(cptr, sptr, &me,
                           "Sorry, your connection class is full - try "
                           "again later or try another server");
      case ACR_TOO_MANY_FROM_IP:
        if (CurrentTime - last_too_many2 >= (time_t) 60)
        {
          last_too_many2 = CurrentTime;
          sendto_opmask_butone(0, SNO_TOOMANY, "Too many connections from "
                               "same IP for %s.",
                               get_client_name(sptr, HIDE_IP));
        }
        ++ServerStats->is_ref;
        return exit_client(cptr, sptr, &me,
                           "Too many connections from your host");
      case ACR_ALREADY_AUTHORIZED:
        /* Can this ever happen? */
      case ACR_BAD_SOCKET:
        ++ServerStats->is_ref;
        IPcheck_connect_fail(cli_ip(sptr));
        return exit_client(cptr, sptr, &me, "Unknown error -- Try again");
    }
    ircd_strncpy(user->host, cli_sockhost(sptr), HOSTLEN);
    aconf = cli_confs(sptr)->value.aconf;

    clean_user_id(user->username,
        (cli_flags(sptr) & FLAGS_GOTID) ? cli_username(sptr) : username,
        (cli_flags(sptr) & FLAGS_DOID) && !(cli_flags(sptr) & FLAGS_GOTID));

    if ((user->username[0] == '\0')
        || ((user->username[0] == '~') && (user->username[1] == '\000')))
      return exit_client(cptr, sptr, &me, "USER: Bogus userid.");

    if (!EmptyString(aconf->passwd)
        && !(IsDigit(*aconf->passwd) && !aconf->passwd[1])
        && strcmp(cli_passwd(sptr), aconf->passwd))
    {
      ServerStats->is_ref++;
      IPcheck_connect_fail(cli_ip(sptr));
      send_reply(sptr, ERR_PASSWDMISMATCH);
      return exit_client(cptr, sptr, &me, "Bad Password");
    }
    memset(cli_passwd(sptr), 0, sizeof(cli_passwd(sptr)));
    /*
     * following block for the benefit of time-dependent K:-lines
     */
    if (find_kill(sptr)) {
      ServerStats->is_ref++;
      IPcheck_connect_fail(cli_ip(sptr));
      return exit_client(cptr, sptr, &me, "K-lined");
    }
    /*
     * Check for mixed case usernames, meaning probably hacked.  Jon2 3-94
     * Summary of rules now implemented in this patch:         Ensor 11-94
     * In a mixed-case name, if first char is upper, one more upper may
     * appear anywhere.  (A mixed-case name *must* have an upper first
     * char, and may have one other upper.)
     * A third upper may appear if all 3 appear at the beginning of the
     * name, separated only by "others" (-/_/.).
     * A single group of digits is allowed anywhere.
     * Two groups of digits are allowed if at least one of the groups is
     * at the beginning or the end.
     * Only one '-', '_', or '.' is allowed (or two, if not consecutive).
     * But not as the first or last char.
     * No other special characters are allowed.
     * Name must contain at least one letter.
     */
    tmpstr2 = tmpstr = (username[0] == '~' ? &username[1] : username);
    while (*tmpstr && !badid)
    {
      pos++;
      c = *tmpstr;
      tmpstr++;
      if (IsLower(c))
      {
        lower++;
      }
      else if (IsUpper(c))
      {
        upper++;
        if ((leadcaps || pos == 1) && !lower && !digits)
          leadcaps++;
      }
      else if (IsDigit(c))
      {
        digits++;
        if (pos == 1 || !IsDigit(d))
        {
          digitgroups++;
          if (digitgroups > 2)
            badid = 1;
        }
      }
      else if (c == '-' || c == '_' || c == '.')
      {
        other++;
        if (pos == 1)
          badid = 1;
        else if (d == '-' || d == '_' || d == '.' || other > 2)
          badid = 1;
      }
      else
        badid = 1;
      d = c;
    }
    if (!badid)
    {
      if (lower && upper && (!leadcaps || leadcaps > 3 ||
          (upper > 2 && upper > leadcaps)))
        badid = 1;
      else if (digitgroups == 2 && !(IsDigit(tmpstr2[0]) || IsDigit(c)))
        badid = 1;
      else if ((!lower && !upper) || !IsAlnum(c))
        badid = 1;
    }
    if (badid && (!(cli_flags(sptr) & FLAGS_GOTID) ||
        strcmp(cli_username(sptr), username) != 0))
    {
      ServerStats->is_ref++;

      send_reply(cptr, SND_EXPLICIT | ERR_INVALIDUSERNAME,
                 ":Your username is invalid.");
      send_reply(cptr, SND_EXPLICIT | ERR_INVALIDUSERNAME,
                 ":Connect with your real username, in lowercase.");
      send_reply(cptr, SND_EXPLICIT | ERR_INVALIDUSERNAME,
                 ":If your mail address were foo@bar.com, your username "
                 "would be foo.");
      return exit_client(cptr, sptr, &me, "USER: Bad username");
    }
    Count_unknownbecomesclient(sptr, UserStats);
  }
  else {
    ircd_strncpy(user->username, username, USERLEN);
    Count_newremoteclient(UserStats, user->server);
  }
  SetUser(sptr);

  /* a gline wasn't passed in, so find a matching global one that isn't
   * a Uworld-set one, and propagate it if there is such an animal.
   */
  if (!agline &&
      (agline = gline_lookup(sptr, GLINE_GLOBAL | GLINE_LASTMOD)) &&
      !IsBurstOrBurstAck(cptr))
    gline_resend(cptr, agline);
  
  if (IsInvisible(sptr))
    ++UserStats.inv_clients;
  if (IsOper(sptr))
    ++UserStats.opers;

  if (MyConnect(sptr)) {
    cli_handler(sptr) = CLIENT_HANDLER;
    release_dns_reply(sptr);

    send_reply(sptr, RPL_WELCOME, nick);
    /*
     * This is a duplicate of the NOTICE but see below...
     */
    send_reply(sptr, RPL_YOURHOST, cli_name(&me), version);
    send_reply(sptr, RPL_CREATED, creation);
    send_reply(sptr, RPL_MYINFO, cli_name(&me), version);
    sprintf_irc(featurebuf,FEATURES,FEATURESVALUES);
    send_reply(sptr, RPL_ISUPPORT, featurebuf);
    m_lusers(sptr, sptr, 1, parv);
    update_load();
    motd_signon(sptr);
    nextping = CurrentTime;
    if (cli_snomask(sptr) & SNO_NOISY)
      set_snomask(sptr, cli_snomask(sptr) & SNO_NOISY, SNO_ADD);
    IPcheck_connect_succeeded(sptr);
  }
  else
    /* if (IsServer(cptr)) */
  {
    struct Client *acptr;

    acptr = user->server;
    if (cli_from(acptr) != cli_from(sptr))
    {
      sendcmdto_one(&me, CMD_KILL, cptr, "%C :%s (%s != %s[%s])",
                    sptr, cli_name(&me), cli_name(user->server), cli_name(cli_from(acptr)),
                    cli_sockhost(cli_from(acptr)));
      cli_flags(sptr) |= FLAGS_KILLED;
      return exit_client(cptr, sptr, &me, "NICK server wrong direction");
    }
    else
      cli_flags(sptr) |= (cli_flags(acptr) & FLAGS_TS8);

    /*
     * Check to see if this user is being propogated
     * as part of a net.burst, or is using protocol 9.
     * FIXME: This can be speeded up - its stupid to check it for
     * every NICK message in a burst again  --Run.
     */
    for (acptr = user->server; acptr != &me; acptr = cli_serv(acptr)->up) {
      if (IsBurst(acptr) || Protocol(acptr) < 10)
        break;
    }
    if (!IPcheck_remote_connect(sptr, (acptr != &me))) {
      /*
       * We ran out of bits to count this
       */
      sendcmdto_one(&me, CMD_KILL, sptr, "%C :%s (Too many connections from your host -- Ghost)",
                    sptr, cli_name(&me));
      return exit_client(cptr, sptr, &me,"Too many connections from your host -- throttled");
    }
  }
  tmpstr = umode_str(sptr);
  if (agline)
    sendcmdto_serv_butone(user->server, CMD_NICK, cptr,
                          "%s %d %Tu %s %s %s%s%s%%%Tu:%s@%s %s %s%s :%s",
                          nick, cli_hopcount(sptr) + 1, cli_lastnick(sptr),
                          user->username, user->host,
                          *tmpstr ? "+" : "", tmpstr, *tmpstr ? " " : "",
                          GlineLastMod(agline), GlineUser(agline),
                          GlineHost(agline),
                          inttobase64(ip_base64, ntohl(cli_ip(sptr).s_addr), 6),
                          NumNick(sptr), cli_info(sptr));
  else
    sendcmdto_serv_butone(user->server, CMD_NICK, cptr,
                          "%s %d %Tu %s %s %s%s%s%s %s%s :%s",
                          nick, cli_hopcount(sptr) + 1, cli_lastnick(sptr),
                          user->username, user->host,
                          *tmpstr ? "+" : "", tmpstr, *tmpstr ? " " : "",
                          inttobase64(ip_base64, ntohl(cli_ip(sptr).s_addr), 6),
                          NumNick(sptr), cli_info(sptr));
  
  /* Send umode to client */
  if (MyUser(sptr))
  {
    send_umode(cptr, sptr, 0, ALL_UMODES);
    if (cli_snomask(sptr) != SNO_DEFAULT && (cli_flags(sptr) & FLAGS_SERVNOTICE))
      send_reply(sptr, RPL_SNOMASK, cli_snomask(sptr), cli_snomask(sptr));
  }

  return 0;
}


static const struct UserMode {
  unsigned int flag;
  char         c;
} userModeList[] = {
  { FLAGS_OPER,        'o' },
  { FLAGS_LOCOP,       'O' },
  { FLAGS_INVISIBLE,   'i' },
  { FLAGS_WALLOP,      'w' },
  { FLAGS_SERVNOTICE,  's' },
  { FLAGS_DEAF,        'd' },
  { FLAGS_CHSERV,      'k' },
  { FLAGS_DEBUG,       'g' }
};

#define USERMODELIST_SIZE sizeof(userModeList) / sizeof(struct UserMode)

/*
 * XXX - find a way to get rid of this
 */
static char umodeBuf[BUFSIZE];

int set_nick_name(struct Client* cptr, struct Client* sptr,
                  const char* nick, int parc, char* parv[])
{
  if (IsServer(sptr)) {
    int   i;
    const char* p;
    char *t;
    struct Gline *agline = 0;

    /*
     * A server introducing a new client, change source
     */
    struct Client* new_client = make_client(cptr, STAT_UNKNOWN);
    assert(0 != new_client);

    cli_hopcount(new_client) = atoi(parv[2]);
    cli_lastnick(new_client) = atoi(parv[3]);
    if (Protocol(cptr) > 9 && parc > 7 && *parv[6] == '+') {
      for (p = parv[6] + 1; *p; p++) {
        for (i = 0; i < USERMODELIST_SIZE; ++i) {
          if (userModeList[i].c == *p) {
            cli_flags(new_client) |= userModeList[i].flag;
            break;
          }
        }
      }
    }
    client_set_privs(new_client); /* set privs on user */
    /*
     * Set new nick name.
     */
    strcpy(cli_name(new_client), nick);
    cli_user(new_client) = make_user(new_client);
    cli_user(new_client)->server = sptr;
    SetRemoteNumNick(new_client, parv[parc - 2]);
    /*
     * IP# of remote client
     */
    cli_ip(new_client).s_addr = htonl(base64toint(parv[parc - 3]));

    add_client_to_list(new_client);
    hAddClient(new_client);

    cli_serv(sptr)->ghost = 0;        /* :server NICK means end of net.burst */
    ircd_strncpy(cli_username(new_client), parv[4], USERLEN);
    ircd_strncpy(cli_user(new_client)->host, parv[5], HOSTLEN);
    ircd_strncpy(cli_info(new_client), parv[parc - 1], REALLEN);

    /* Deal with GLINE parameters... */
    if (*parv[parc - 4] == '%' && (t = strchr(parv[parc - 4] + 1, ':'))) {
      time_t lastmod;

      *(t++) = '\0';
      lastmod = atoi(parv[parc - 4] + 1);

      if (lastmod &&
          (agline = gline_find(t, GLINE_EXACT | GLINE_GLOBAL | GLINE_LASTMOD))
          && GlineLastMod(agline) > lastmod && !IsBurstOrBurstAck(cptr))
        gline_resend(cptr, agline);
    }
    return register_user(cptr, new_client, cli_name(new_client), parv[4], agline);
  }
  else if ((cli_name(sptr))[0]) {
    /*
     * Client changing its nick
     *
     * If the client belongs to me, then check to see
     * if client is on any channels where it is currently
     * banned.  If so, do not allow the nick change to occur.
     */
    if (MyUser(sptr)) {
      const char* channel_name;
      if ((channel_name = find_no_nickchange_channel(sptr))) {
        return send_reply(cptr, ERR_BANNICKCHANGE, channel_name);
      }
      /*
       * Refuse nick change if the last nick change was less
       * then 30 seconds ago. This is intended to get rid of
       * clone bots doing NICK FLOOD. -SeKs
       * If someone didn't change their nick for more then 60 seconds
       * however, allow to do two nick changes immedately after another
       * before limiting the nick flood. -Run
       */
      if (CurrentTime < cli_nextnick(cptr)) {
        cli_nextnick(cptr) += 2;
        send_reply(cptr, ERR_NICKTOOFAST, parv[1],
                   cli_nextnick(cptr) - CurrentTime);
        /* Send error message */
        sendcmdto_one(cptr, CMD_NICK, cptr, "%s", cli_name(cptr));
        /* bounce NICK to user */
        return 0;                /* ignore nick change! */
      }
      else {
        /* Limit total to 1 change per NICK_DELAY seconds: */
        cli_nextnick(cptr) += NICK_DELAY;
        /* However allow _maximal_ 1 extra consecutive nick change: */
        if (cli_nextnick(cptr) < CurrentTime)
          cli_nextnick(cptr) = CurrentTime;
      }
    }
    /*
     * Also set 'lastnick' to current time, if changed.
     */
    if (0 != ircd_strcmp(parv[0], nick))
      cli_lastnick(sptr) = (sptr == cptr) ? TStime() : atoi(parv[2]);

    /*
     * Client just changing his/her nick. If he/she is
     * on a channel, send note of change to all clients
     * on that channel. Propagate notice to other servers.
     */
    if (IsUser(sptr)) {
      sendcmdto_common_channels(sptr, CMD_NICK, ":%s", nick);
      add_history(sptr, 1);
      sendcmdto_serv_butone(sptr, CMD_NICK, cptr, "%s %Tu", nick,
                            cli_lastnick(sptr));
    }
    else
      sendcmdto_one(sptr, CMD_NICK, sptr, ":%s", nick);

    if ((cli_name(sptr))[0])
      hRemClient(sptr);
    strcpy(cli_name(sptr), nick);
    hAddClient(sptr);
  }
  else {
    /* Local client setting NICK the first time */

    strcpy(cli_name(sptr), nick);
    if (!cli_user(sptr)) {
      cli_user(sptr) = make_user(sptr);
      cli_user(sptr)->server = &me;
    }
    SetLocalNumNick(sptr);
    hAddClient(sptr);

    /*
     * If the client hasn't gotten a cookie-ping yet,
     * choose a cookie and send it. -record!jegelhof@cloud9.net
     */
    if (!cli_cookie(sptr)) {
      do {
        cli_cookie(sptr) = (ircrandom() & 0x7fffffff);
      } while (!cli_cookie(sptr));
      sendrawto_one(cptr, MSG_PING " :%u", cli_cookie(sptr));
    }
    else if (*(cli_user(sptr))->host && cli_cookie(sptr) == COOKIE_VERIFIED) {
      /*
       * USER and PONG already received, now we have NICK.
       * register_user may reject the client and call exit_client
       * for it - must test this and exit m_nick too !
       */
      cli_lastnick(sptr) = TStime();        /* Always local client */
      if (register_user(cptr, sptr, nick, cli_user(sptr)->username, 0) == CPTR_KILLED)
        return CPTR_KILLED;
    }
  }
  return 0;
}

static unsigned char hash_target(unsigned int target)
{
  return (unsigned char) (target >> 16) ^ (target >> 8);
}

/*
 * add_target
 *
 * sptr must be a local client!
 *
 * Cannonifies target for client `sptr'.
 */
void add_target(struct Client *sptr, void *target)
{
  /* Ok, this shouldn't work esp on alpha
  */
  unsigned char  hash = hash_target((unsigned long) target);
  unsigned char* targets;
  int            i;
  assert(0 != sptr);
  assert(cli_local(sptr));

  targets = cli_targets(sptr);
  /* 
   * Already in table?
   */
  for (i = 0; i < MAXTARGETS; ++i) {
    if (targets[i] == hash)
      return;
  }
  /*
   * New target
   */
  memmove(&targets[RESERVEDTARGETS + 1],
          &targets[RESERVEDTARGETS], MAXTARGETS - RESERVEDTARGETS - 1);
  targets[RESERVEDTARGETS] = hash;
}

/*
 * check_target_limit
 *
 * sptr must be a local client !
 *
 * Returns 'true' (1) when too many targets are addressed.
 * Returns 'false' (0) when it's ok to send to this target.
 */
int check_target_limit(struct Client *sptr, void *target, const char *name,
    int created)
{
  unsigned char hash = hash_target((unsigned long) target);
  int            i;
  unsigned char* targets;

  assert(0 != sptr);
  assert(cli_local(sptr));
  targets = cli_targets(sptr);

  /*
   * Same target as last time?
   */
  if (targets[0] == hash)
    return 0;
  for (i = 1; i < MAXTARGETS; ++i) {
    if (targets[i] == hash) {
      memmove(&targets[1], &targets[0], i);
      targets[0] = hash;
      return 0;
    }
  }
  /*
   * New target
   */
  if (!created) {
    if (CurrentTime < cli_nexttarget(sptr)) {
      if (cli_nexttarget(sptr) - CurrentTime < TARGET_DELAY + 8) {
        /*
         * No server flooding
         */
        cli_nexttarget(sptr) += 2;
        send_reply(sptr, ERR_TARGETTOOFAST, name,
                   cli_nexttarget(sptr) - CurrentTime);
      }
      return 1;
    }
    else {
      cli_nexttarget(sptr) += TARGET_DELAY;
      if (cli_nexttarget(sptr) < CurrentTime - (TARGET_DELAY * (MAXTARGETS - 1)))
        cli_nexttarget(sptr) = CurrentTime - (TARGET_DELAY * (MAXTARGETS - 1));
    }
  }
  memmove(&targets[1], &targets[0], MAXTARGETS - 1);
  targets[0] = hash;
  return 0;
}

/*
 * whisper - called from m_cnotice and m_cprivmsg.
 *
 * parv[0] = sender prefix
 * parv[1] = nick
 * parv[2] = #channel
 * parv[3] = Private message text
 *
 * Added 971023 by Run.
 * Reason: Allows channel operators to sent an arbitrary number of private
 *   messages to users on their channel, avoiding the max.targets limit.
 *   Building this into m_private would use too much cpu because we'd have
 *   to a cross channel lookup for every private message!
 * Note that we can't allow non-chan ops to use this command, it would be
 *   abused by mass advertisers.
 *
 */
int whisper(struct Client* source, const char* nick, const char* channel,
            const char* text, int is_notice)
{
  struct Client*     dest;
  struct Channel*    chptr;
  struct Membership* membership;

  assert(0 != source);
  assert(0 != nick);
  assert(0 != channel);
  assert(MyUser(source));

  if (!(dest = FindUser(nick))) {
    return send_reply(source, ERR_NOSUCHNICK, nick);
  }
  if (!(chptr = FindChannel(channel))) {
    return send_reply(source, ERR_NOSUCHCHANNEL, channel);
  }
  /*
   * compare both users channel lists, instead of the channels user list
   * since the link is the same, this should be a little faster for channels
   * with a lot of users
   */
  for (membership = cli_user(source)->channel; membership; membership = membership->next_channel) {
    if (chptr == membership->channel)
      break;
  }
  if (0 == membership) {
    return send_reply(source, ERR_NOTONCHANNEL, chptr->chname);
  }
  if (!IsVoicedOrOpped(membership)) {
    return send_reply(source, ERR_VOICENEEDED, chptr->chname);
  }
  /*
   * lookup channel in destination
   */
  assert(0 != cli_user(dest));
  for (membership = cli_user(dest)->channel; membership; membership = membership->next_channel) {
    if (chptr == membership->channel)
      break;
  }
  if (0 == membership || IsZombie(membership)) {
    return send_reply(source, ERR_USERNOTINCHANNEL, cli_name(dest), chptr->chname);
  }
  if (is_silenced(source, dest))
    return 0;
          
  if (cli_user(dest)->away)
    send_reply(source, RPL_AWAY, cli_name(dest), cli_user(dest)->away);
  if (is_notice)
    sendcmdto_one(source, CMD_NOTICE, dest, "%C :%s", dest, text);
  else
    sendcmdto_one(source, CMD_PRIVATE, dest, "%C :%s", dest, text);
  return 0;
}


/*
 * added Sat Jul 25 07:30:42 EST 1992
 */
void send_umode_out(struct Client *cptr, struct Client *sptr, int old,
		    int prop)
{
  int i;
  struct Client *acptr;

  send_umode(NULL, sptr, old, SEND_UMODES & ~(prop ? 0 : FLAGS_OPER));

  for (i = HighestFd; i >= 0; i--) {
    if ((acptr = LocalClientArray[i]) && IsServer(acptr) &&
        (acptr != cptr) && (acptr != sptr) && *umodeBuf)
      sendcmdto_one(sptr, CMD_MODE, acptr, "%s :%s", cli_name(sptr), umodeBuf);
  }
  if (cptr && MyUser(cptr))
    send_umode(cptr, sptr, old, ALL_UMODES);
}


/*
 * send_user_info - send user info userip/userhost
 * NOTE: formatter must put info into buffer and return a pointer to the end of
 * the data it put in the buffer.
 */
void send_user_info(struct Client* sptr, char* names, int rpl, InfoFormatter fmt)
{
  char*          name;
  char*          p = 0;
  int            arg_count = 0;
  int            users_found = 0;
  struct Client* acptr;
  struct MsgBuf* mb;

  assert(0 != sptr);
  assert(0 != names);
  assert(0 != fmt);

  mb = msgq_make(sptr, rpl_str(rpl), cli_name(&me), cli_name(sptr));

  for (name = ircd_strtok(&p, names, " "); name; name = ircd_strtok(&p, 0, " ")) {
    if ((acptr = FindUser(name))) {
      if (users_found++)
	msgq_append(0, mb, " ");
      (*fmt)(acptr, mb);
    }
    if (5 == ++arg_count)
      break;
  }
  send_buffer(sptr, mb, 0);
  msgq_clean(mb);
}


/*
 * set_user_mode() added 15/10/91 By Darren Reed.
 *
 * parv[0] - sender
 * parv[1] - username to change mode for
 * parv[2] - modes to change
 */
int set_user_mode(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  char** p;
  char*  m;
  struct Client *acptr;
  int what;
  int i;
  int setflags;
  unsigned int tmpmask = 0;
  int snomask_given = 0;
  char buf[BUFSIZE];
  int prop = 0;

  what = MODE_ADD;

  if (parc < 2)
    return need_more_params(sptr, "MODE");

  if (!(acptr = FindUser(parv[1])))
  {
    if (MyConnect(sptr))
      send_reply(sptr, ERR_NOSUCHCHANNEL, parv[1]);
    return 0;
  }

  if (IsServer(sptr) || sptr != acptr)
  {
    if (IsServer(cptr))
      sendcmdto_flag_butone(&me, CMD_WALLOPS, 0, FLAGS_WALLOP,
                            ":MODE for User %s from %s!%s", parv[1],
                            cli_name(cptr), cli_name(sptr));
    else
      send_reply(sptr, ERR_USERSDONTMATCH);
    return 0;
  }

  if (parc < 3)
  {
    m = buf;
    *m++ = '+';
    for (i = 0; i < USERMODELIST_SIZE; ++i) {
      if ( (userModeList[i].flag & cli_flags(sptr)))
        *m++ = userModeList[i].c;
    }
    *m = '\0';
    send_reply(sptr, RPL_UMODEIS, buf);
    if ((cli_flags(sptr) & FLAGS_SERVNOTICE) && MyConnect(sptr)
        && cli_snomask(sptr) !=
        (unsigned int)(IsOper(sptr) ? SNO_OPERDEFAULT : SNO_DEFAULT))
      send_reply(sptr, RPL_SNOMASK, cli_snomask(sptr), cli_snomask(sptr));
    return 0;
  }

  /*
   * find flags already set for user
   * why not just copy them?
   */
  setflags = cli_flags(sptr);

  if (MyConnect(sptr))
    tmpmask = cli_snomask(sptr);

  /*
   * parse mode change string(s)
   */
  for (p = &parv[2]; *p; p++) {       /* p is changed in loop too */
    for (m = *p; *m; m++) {
      switch (*m) {
      case '+':
        what = MODE_ADD;
        break;
      case '-':
        what = MODE_DEL;
        break;
      case 's':
        if (*(p + 1) && is_snomask(*(p + 1))) {
          snomask_given = 1;
          tmpmask = umode_make_snomask(tmpmask, *++p, what);
          tmpmask &= (IsAnOper(sptr) ? SNO_ALL : SNO_USER);
        }
        else
          tmpmask = (what == MODE_ADD) ?
              (IsAnOper(sptr) ? SNO_OPERDEFAULT : SNO_DEFAULT) : 0;
        if (tmpmask)
          cli_flags(sptr) |= FLAGS_SERVNOTICE;
        else
          cli_flags(sptr) &= ~FLAGS_SERVNOTICE;
        break;
      case 'w':
        if (what == MODE_ADD)
          SetWallops(sptr);
        else
          ClearWallops(sptr);
        break;
      case 'o':
        if (what == MODE_ADD)
          SetOper(sptr);
        else {
          cli_flags(sptr) &= ~(FLAGS_OPER | FLAGS_LOCOP);
          if (MyConnect(sptr)) {
            tmpmask = cli_snomask(sptr) & ~SNO_OPER;
            cli_handler(sptr) = CLIENT_HANDLER;
          }
        }
        break;
      case 'O':
        if (what == MODE_ADD)
          SetLocOp(sptr);
        else { 
          cli_flags(sptr) &= ~(FLAGS_OPER | FLAGS_LOCOP);
          if (MyConnect(sptr)) {
            tmpmask = cli_snomask(sptr) & ~SNO_OPER;
            cli_handler(sptr) = CLIENT_HANDLER;
          }
        }
        break;
      case 'i':
        if (what == MODE_ADD)
          SetInvisible(sptr);
        else
          ClearInvisible(sptr);
        break;
      case 'd':
        if (what == MODE_ADD)
          SetDeaf(sptr);
        else
          ClearDeaf(sptr);
        break;
      case 'k':
        if (what == MODE_ADD)
          SetChannelService(sptr);
        else
          ClearChannelService(sptr);
        break;
      case 'g':
        if (what == MODE_ADD)
          SetDebug(sptr);
        else
          ClearDebug(sptr);
        break;
      default:
        break;
      }
    }
  }
  /*
   * Evaluate rules for new user mode
   * Stop users making themselves operators too easily:
   */
  if (!IsServer(cptr)) {
    if (!(setflags & FLAGS_OPER) && IsOper(sptr))
      ClearOper(sptr);
    if (!(setflags & FLAGS_LOCOP) && IsLocOp(sptr))
      ClearLocOp(sptr);
    /*
     * new umode; servers can set it, local users cannot;
     * prevents users from /kick'ing or /mode -o'ing
     */
    if (!(setflags & FLAGS_CHSERV))
      ClearChannelService(sptr);
    /*
     * only send wallops to opers
     */
    if (feature_bool(FEAT_WALLOPS_OPER_ONLY) && !IsAnOper(sptr) &&
	!(setflags & FLAGS_WALLOP))
      ClearWallops(sptr);
  }
  if (MyConnect(sptr)) {
    if ((setflags & (FLAGS_OPER | FLAGS_LOCOP)) && !IsAnOper(sptr))
      det_confs_butmask(sptr, CONF_CLIENT & ~CONF_OPS);

    if (tmpmask != cli_snomask(sptr))
      set_snomask(sptr, tmpmask, SNO_SET);
    if (cli_snomask(sptr) && snomask_given)
      send_reply(sptr, RPL_SNOMASK, cli_snomask(sptr), cli_snomask(sptr));
  }
  /*
   * Compare new flags with old flags and send string which
   * will cause servers to update correctly.
   */
  if (!(setflags & FLAGS_OPER) && IsOper(sptr)) { /* user now oper */
    ++UserStats.opers;
    client_set_privs(sptr); /* may set propagate privilege */
  }
  if (HasPriv(sptr, PRIV_PROPAGATE)) /* remember propagate privilege setting */
    prop = 1;
  if ((setflags & FLAGS_OPER) && !IsOper(sptr)) { /* user no longer oper */
    --UserStats.opers;
    client_set_privs(sptr); /* will clear propagate privilege */
  }
  if ((setflags & FLAGS_INVISIBLE) && !IsInvisible(sptr))
    --UserStats.inv_clients;
  if (!(setflags & FLAGS_INVISIBLE) && IsInvisible(sptr))
    ++UserStats.inv_clients;
  send_umode_out(cptr, sptr, setflags, prop);

  return 0;
}

/*
 * Build umode string for BURST command
 * --Run
 */
char *umode_str(struct Client *cptr)
{
  char* m = umodeBuf;                /* Maximum string size: "owidg\0" */
  int   i;
  int   c_flags;

  c_flags = cli_flags(cptr) & SEND_UMODES; /* cleaning up the original code */
  if (HasPriv(cptr, PRIV_PROPAGATE))
    c_flags |= FLAGS_OPER;
  else
    c_flags &= ~FLAGS_OPER;

  for (i = 0; i < USERMODELIST_SIZE; ++i) {
    if ( (c_flags & userModeList[i].flag))
      *m++ = userModeList[i].c;
  }
  *m = '\0';

  return umodeBuf;                /* Note: static buffer, gets
                                   overwritten by send_umode() */
}

/*
 * Send the MODE string for user (user) to connection cptr
 * -avalon
 */
void send_umode(struct Client *cptr, struct Client *sptr, int old, int sendmask)
{
  int i;
  int flag;
  char *m;
  int what = MODE_NULL;

  /*
   * Build a string in umodeBuf to represent the change in the user's
   * mode between the new (sptr->flag) and 'old'.
   */
  m = umodeBuf;
  *m = '\0';
  for (i = 0; i < USERMODELIST_SIZE; ++i) {
    flag = userModeList[i].flag;
    if (MyUser(sptr) && !(flag & sendmask))
      continue;
    if ( (flag & old) && !(cli_flags(sptr) & flag))
    {
      if (what == MODE_DEL)
        *m++ = userModeList[i].c;
      else
      {
        what = MODE_DEL;
        *m++ = '-';
        *m++ = userModeList[i].c;
      }
    }
    else if (!(flag & old) && (cli_flags(sptr) & flag))
    {
      if (what == MODE_ADD)
        *m++ = userModeList[i].c;
      else
      {
        what = MODE_ADD;
        *m++ = '+';
        *m++ = userModeList[i].c;
      }
    }
  }
  *m = '\0';
  if (*umodeBuf && cptr)
    sendcmdto_one(sptr, CMD_MODE, cptr, "%s :%s", cli_name(sptr), umodeBuf);
}

/*
 * Check to see if this resembles a sno_mask.  It is if 1) there is
 * at least one digit and 2) The first digit occurs before the first
 * alphabetic character.
 */
int is_snomask(char *word)
{
  if (word)
  {
    for (; *word; word++)
      if (IsDigit(*word))
        return 1;
      else if (IsAlpha(*word))
        return 0;
  }
  return 0;
}

/*
 * If it begins with a +, count this as an additive mask instead of just
 * a replacement.  If what == MODE_DEL, "+" has no special effect.
 */
unsigned int umode_make_snomask(unsigned int oldmask, char *arg, int what)
{
  unsigned int sno_what;
  unsigned int newmask;
  if (*arg == '+')
  {
    arg++;
    if (what == MODE_ADD)
      sno_what = SNO_ADD;
    else
      sno_what = SNO_DEL;
  }
  else if (*arg == '-')
  {
    arg++;
    if (what == MODE_ADD)
      sno_what = SNO_DEL;
    else
      sno_what = SNO_ADD;
  }
  else
    sno_what = (what == MODE_ADD) ? SNO_SET : SNO_DEL;
  /* pity we don't have strtoul everywhere */
  newmask = (unsigned int)atoi(arg);
  if (sno_what == SNO_DEL)
    newmask = oldmask & ~newmask;
  else if (sno_what == SNO_ADD)
    newmask |= oldmask;
  return newmask;
}

static void delfrom_list(struct Client *cptr, struct SLink **list)
{
  struct SLink* tmp;
  struct SLink* prv = NULL;

  for (tmp = *list; tmp; tmp = tmp->next) {
    if (tmp->value.cptr == cptr) {
      if (prv)
        prv->next = tmp->next;
      else
        *list = tmp->next;
      free_link(tmp);
      break;
    }
    prv = tmp;
  }
}

/*
 * This function sets a Client's server notices mask, according to
 * the parameter 'what'.  This could be even faster, but the code
 * gets mighty hard to read :)
 */
void set_snomask(struct Client *cptr, unsigned int newmask, int what)
{
  unsigned int oldmask, diffmask;        /* unsigned please */
  int i;
  struct SLink *tmp;

  oldmask = cli_snomask(cptr);

  if (what == SNO_ADD)
    newmask |= oldmask;
  else if (what == SNO_DEL)
    newmask = oldmask & ~newmask;
  else if (what != SNO_SET)        /* absolute set, no math needed */
    sendto_opmask_butone(0, SNO_OLDSNO, "setsnomask called with %d ?!", what);

  newmask &= (IsAnOper(cptr) ? SNO_ALL : SNO_USER);

  diffmask = oldmask ^ newmask;

  for (i = 0; diffmask >> i; i++) {
    if (((diffmask >> i) & 1))
    {
      if (((newmask >> i) & 1))
      {
        tmp = make_link();
        tmp->next = opsarray[i];
        tmp->value.cptr = cptr;
        opsarray[i] = tmp;
      }
      else
        /* not real portable :( */
        delfrom_list(cptr, &opsarray[i]);
    }
  }
  cli_snomask(cptr) = newmask;
}

/*
 * is_silenced : Does the actual check wether sptr is allowed
 *               to send a message to acptr.
 *               Both must be registered persons.
 * If sptr is silenced by acptr, his message should not be propagated,
 * but more over, if this is detected on a server not local to sptr
 * the SILENCE mask is sent upstream.
 */
int is_silenced(struct Client *sptr, struct Client *acptr)
{
  struct SLink *lp;
  struct User *user;
  static char sender[HOSTLEN + NICKLEN + USERLEN + 5];
  static char senderip[16 + NICKLEN + USERLEN + 5];

  if (!cli_user(acptr) || !(lp = cli_user(acptr)->silence) || !(user = cli_user(sptr)))
    return 0;
  sprintf_irc(sender, "%s!%s@%s", cli_name(sptr), user->username, user->host);
  sprintf_irc(senderip, "%s!%s@%s", cli_name(sptr), user->username,
              ircd_ntoa((const char*) &(cli_ip(sptr))));
  for (; lp; lp = lp->next)
  {
    if ((!(lp->flags & CHFL_SILENCE_IPMASK) && !match(lp->value.cp, sender)) ||
        ((lp->flags & CHFL_SILENCE_IPMASK) && !match(lp->value.cp, senderip)))
    {
      if (!MyConnect(sptr))
      {
        sendcmdto_one(acptr, CMD_SILENCE, cli_from(sptr), "%C %s", sptr,
                      lp->value.cp);
      }
      return 1;
    }
  }
  return 0;
}

/*
 * del_silence
 *
 * Removes all silence masks from the list of sptr that fall within `mask'
 * Returns -1 if none where found, 0 otherwise.
 */
int del_silence(struct Client *sptr, char *mask)
{
  struct SLink **lp;
  struct SLink *tmp;
  int ret = -1;

  for (lp = &(cli_user(sptr))->silence; *lp;) {
    if (!mmatch(mask, (*lp)->value.cp))
    {
      tmp = *lp;
      *lp = tmp->next;
      MyFree(tmp->value.cp);
      free_link(tmp);
      ret = 0;
    }
    else
      lp = &(*lp)->next;
  }
  return ret;
}

int add_silence(struct Client* sptr, const char* mask)
{
  struct SLink *lp, **lpp;
  int cnt = 0, len = strlen(mask);
  char *ip_start;

  for (lpp = &(cli_user(sptr))->silence, lp = *lpp; lp;)
  {
    if (0 == ircd_strcmp(mask, lp->value.cp))
      return -1;
    if (!mmatch(mask, lp->value.cp))
    {
      struct SLink *tmp = lp;
      *lpp = lp = lp->next;
      MyFree(tmp->value.cp);
      free_link(tmp);
      continue;
    }
    if (MyUser(sptr))
    {
      len += strlen(lp->value.cp);
      if ((len > (feature_int(FEAT_AVBANLEN) * feature_int(FEAT_MAXSILES))) ||
	  (++cnt >= feature_int(FEAT_MAXSILES)))
      {
        send_reply(sptr, ERR_SILELISTFULL, mask);
        return -1;
      }
      else if (!mmatch(lp->value.cp, mask))
        return -1;
    }
    lpp = &lp->next;
    lp = *lpp;
  }
  lp = make_link();
  memset(lp, 0, sizeof(struct SLink));
  lp->next = cli_user(sptr)->silence;
  lp->value.cp = (char*) MyMalloc(strlen(mask) + 1);
  assert(0 != lp->value.cp);
  strcpy(lp->value.cp, mask);
  if ((ip_start = strrchr(mask, '@')) && check_if_ipmask(ip_start + 1))
    lp->flags = CHFL_SILENCE_IPMASK;
  cli_user(sptr)->silence = lp;
  return 0;
}

