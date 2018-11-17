/*
 * IRC-Hispano IRC Daemon, include/whocmds.h
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
/** @file whocmds.h
 * @brief Support functions for /WHO-like commands.
 */
#ifndef INCLUDED_whocmds_h
#define INCLUDED_whocmds_h

struct Client;
struct Channel;


/*
 * m_who() 
 * m_who with support routines rewritten by Nemesi, August 1997
 * - Alghoritm have been flattened (no more recursive)
 * - Several bug fixes
 * - Strong performance improvement
 * - Added possibility to have specific fields in the output
 * See readme.who for further details.
 */

/* Macros used only in here by m_who and its support functions */

#define WHOSELECT_OPER 1   /**< Flag for /WHO: Show IRC operators. */
#define WHOSELECT_EXTRA 2  /**< Flag for /WHO: Pull rank to see users. */
#define WHOSELECT_DELAY 4  /**< Flag for /WHO: Show join-delayed users. */

#define WHO_FIELD_QTY 1    /**< Display query type. */
#define WHO_FIELD_CHA 2    /**< Show common channel name. */
#define WHO_FIELD_UID 4    /**< Show username. */
#define WHO_FIELD_NIP 8    /**< Show IP address. */
#define WHO_FIELD_HOS 16   /**< Show hostname. */
#define WHO_FIELD_SER 32   /**< Show server. */
#define WHO_FIELD_NIC 64   /**< Show nickname. */
#define WHO_FIELD_FLA 128  /**< Show flags (away, oper, chanop, etc). */
#define WHO_FIELD_DIS 256  /**< Show hop count (distance). */
#define WHO_FIELD_REN 512  /**< Show realname (info). */
#define WHO_FIELD_IDL 1024 /**< Show idle time. */
#define WHO_FIELD_ACC 2048 /**< Show account name. */
#define WHO_FIELD_OPL 4096 /**< Show oplevel. */

/** Default fields for /WHO */
#define WHO_FIELD_DEF ( WHO_FIELD_NIC | WHO_FIELD_UID | WHO_FIELD_HOS | WHO_FIELD_SER )

/** Is \a ac plainly visible to \a s?
 * @param[in] s Client trying to see \a ac.
 * @param[in] ac Client being looked at.
 */
#define IS_VISIBLE_USER(s,ac) ((s==ac) || (!IsInvisible(ac)))

/** Can \a s see \a ac by using the flags in \a b?
 * @param[in] s Client trying to see \a ac.
 * @param[in] ac Client being looked at.
 * @param[in] b Bitset of extra flags (options: WHOSELECT_EXTRA).
 */
#define SEE_LUSER(s, ac, b) (IS_VISIBLE_USER(s, ac) || \
                             ((b & WHOSELECT_EXTRA) && MyConnect(ac) && \
                             (HasPriv((s), PRIV_SHOW_INVIS) || \
                              HasPriv((s), PRIV_SHOW_ALL_INVIS))))

/** Can \a s see \a ac by using the flags in \a b?
 * @param[in] s Client trying to see \a ac.
 * @param[in] ac Client being looked at.
 * @param[in] b Bitset of extra flags (options: WHOSELECT_EXTRA).
 */
#define SEE_USER(s, ac, b) (SEE_LUSER(s, ac, b) || \
                            ((b & WHOSELECT_EXTRA) && \
                              HasPriv((s), PRIV_SHOW_ALL_INVIS)))

/** Should we show more clients to \a sptr?
 * @param[in] sptr Client listing other users.
 * @param[in,out] counter Default count for clients.
 */
#define SHOW_MORE(sptr, counter) (HasPriv(sptr, PRIV_UNLIMIT_QUERY) || (!(counter-- < 0)) )

/** Can \a s see \a chptr?
 * @param[in] s Client trying to see \a chptr.
 * @param[in] chptr Channel being looked at.
 * @param[in] b Bitset of extra flags (options: WHOSELECT_EXTRA).
 */
#define SEE_CHANNEL(s, chptr, b) (!SecretChannel(chptr) || ((b & WHOSELECT_EXTRA) && HasPriv((s), PRIV_SEE_CHAN)))

/** Maximum number of lines to send in response to a /WHOIS. */
#define MAX_WHOIS_LINES 50

/*
 * Prototypes
 */
extern void do_who(struct Client* sptr, struct Client* acptr, struct Channel* repchan,
                   int fields, char* qrt);

#endif /* INCLUDED_whocmds_h */
