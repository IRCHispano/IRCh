/*
 * IRC-Hispano IRC Daemon, include/supported.h
 *
 * Copyright (C) 1997-2017 IRC-Hispano Development Team <devel@irc-hispano.es>
 * Copyright (C) 1999 Perry Lorier <isomer@coders.net>
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
 * Description: This file has the featureset that ircu announces on connecting
 *              a client.  It's in this .h because it's likely to be appended
 *              to frequently and s_user.h is included by basically everyone.
 */
#ifndef INCLUDED_supported_h
#define INCLUDED_supported_h

#include "channel.h"
#include "ircd_defs.h"

/* 
 * 'Features' supported by this ircd
 */
#define FEATURES1 \
                "WHOX"\
                " WALLCHOPS"\
                " WALLVOICES"\
                " USERIP"\
                " CPRIVMSG"\
                " CNOTICE"\
                " SILENCE=%i" \
                " MODES=%i" \
                " MAXCHANNELS=%i" \
                " MAXBANS=%i" \
                " NICKLEN=%i" \
                " MONITOR=%i"

#define FEATURES2 "MAXNICKLEN=%i" \
                " TOPICLEN=%i" \
                " AWAYLEN=%i" \
                " KICKLEN=%i" \
                " CHANNELLEN=%i" \
                " MAXCHANNELLEN=%i" \
                " CHANTYPES=%s" \
                " PREFIX=%s" \
                " STATUSMSG=%s" \
                " CHANMODES=%s" \
                " CASEMAPPING=%s" \
                " NETWORK=%s"

#define FEATURESVALUES1 feature_int(FEAT_MAXSILES), MAXMODEPARAMS, \
			feature_int(FEAT_MAXCHANNELSPERUSER), \
                        feature_int(FEAT_MAXBANS), feature_int(FEAT_NICKLEN), \
                        feature_int(FEAT_MAXMONITOR)

#define FEATURESVALUES2 NICKLEN, TOPICLEN, AWAYLEN, TOPICLEN, \
                        feature_int(FEAT_CHANNELLEN), CHANNELLEN, \
                        (feature_bool(FEAT_LOCAL_CHANNELS) ? "#&" : "#"), "(ov)@+", "@+", \
                        (feature_bool(FEAT_OPLEVELS) ? "b,AkU,l,imnpstrDdRcC" : "b,k,l,imnpstrDdRcC"), \
                        "rfc1459", feature_str(FEAT_NETWORK)

#endif /* INCLUDED_supported_h */

