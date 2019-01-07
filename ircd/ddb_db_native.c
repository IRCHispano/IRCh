/*
 * IRC-Hispano IRC Daemon, include/ddb_db_native.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2004-2007,2018 Toni Garcia (zoltan) <toni@tonigarcia.es>
 * Copyright (C) 1999-2003 Jesus Cea Avion <jcea@jcea.es>
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
 * @brief Native DataBase implementation of Distributed DataBases.
 */
#include "config.h"

#include "ddb.h"
#include "ircd_features.h"
#include "ircd_snprintf.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

/** Initialize database gestion module of
 * %DDB Distributed DataBases.
 */
void
ddb_db_init(void)
{
  char path[1024];
  struct stat sStat;
  unsigned char table;
  int fd;

  ircd_snprintf(0, path, sizeof(path), "%s/", feature_str(FEAT_DDBPATH));
  if ((stat(path, &sStat) == -1))
  {
    if (0 != mkdir(feature_str(FEAT_DDBPATH), 0775))
      ddb_die("Error when creating %s directory", feature_str(FEAT_DDBPATH));
  }
  else
  {
    if (!S_ISDIR(sStat.st_mode))
      ddb_die("Error S_ISDIR(%s)", feature_str(FEAT_DDBPATH));
  }

  /* Verify if hashes file is exist. */
  ircd_snprintf(0, path, sizeof(path), "%s/hashes", feature_str(FEAT_DDBPATH));
  alarm(3);
  fd = open(path, O_WRONLY, S_IRUSR | S_IWUSR);
  if (fd == -1)
  {
    fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1)
      ddb_die("Error when creating hashes file (OPEN)");

    for (table = DDB_INIT; table <= DDB_END; table++)
    {
      ircd_snprintf(0, path, sizeof(path), "%c AAAAAAAAAAAA\n", table);
      write(fd, path, 15);
    }
  }
  close(fd);
  alarm(0);

  /* Verify if tables file is exist. */
  for (table = DDB_INIT; table <= DDB_END; table++)
  {
    ircd_snprintf(0, path, sizeof(path), "%s/table.%c",
                  feature_str(FEAT_DDBPATH), table);
    alarm(3);
    fd = open(path, O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
      fd = open(path, O_CREAT, S_IRUSR | S_IWUSR);
      if (fd == -1)
        ddb_die("Error when creating table '%c' (OPEN)", table);
    }
    close(fd);
    alarm(0);
  }
}

/** Executes when finalizes the %DDB subsystem.
 */
void
ddb_db_end(void)
{
  /* Backup copy? */
}
