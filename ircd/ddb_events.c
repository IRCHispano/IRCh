/*
 * IRC-Hispano IRC Daemon, include/ddb_events.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2004-2007,2018 Toni Garcia (zoltan) <toni@tonigarcia.es>
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
 * @brief Events of Distributed DataBases.
 */
#include "config.h"

#include "ddb.h"

/** It indicates events is initialized */
static int events_init = 0;
/** Events table engine */
ddb_events_table_td ddb_events_table[DDB_TABLE_MAX];

/** Initialize events module of %DDB Distributed DataBases.
 */
void
ddb_events_init(void)
{
  if (events_init)
    return;

  ddb_events_table[DDB_BOTDB] = 0;
  ddb_events_table[DDB_CHANDB] = 0; //ddb_events_table_channels;
  ddb_events_table[DDB_CHANDB2] = 0; //ddb_events_table_channels2;
  ddb_events_table[DDB_EXCEPTIONDB] = 0; //ddb_events_table_exceptions;
  ddb_events_table[DDB_FEATUREDB] = 0; //ddb_events_table_features;
  ddb_events_table[DDB_ILINEDB] = 0;
  ddb_events_table[DDB_JUPEDB] = 0; //ddb_events_table_jupes;
  ddb_events_table[DDB_LOGGINGDB] = 0; //ddb_events_table_logging;
  ddb_events_table[DDB_MOTDDB] = 0;
  ddb_events_table[DDB_NICKDB] = 0; //ddb_events_table_nicks;
  ddb_events_table[DDB_OPERDB] = 0; //ddb_events_table_operators;
  ddb_events_table[DDB_PSEUDODB] = 0; //ddb_events_table_pseudo;
  ddb_events_table[DDB_QUARANTINEDB] = 0; //ddb_events_table_quarantines;
  ddb_events_table[DDB_CHANREDIRECTDB] = 0;
  ddb_events_table[DDB_SPAMDB] = 0; //ddb_events_table_spam;
  ddb_events_table[DDB_UWORLDDB] = 0; //ddb_events_table_uworlds;
  ddb_events_table[DDB_VHOSTDB] = 0; //ddb_events_table_vhosts;
  ddb_events_table[DDB_WEBIRCDB] = 0; //ddb_events_table_webircs;
  events_init = 1;
}
