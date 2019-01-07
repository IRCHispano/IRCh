/*
 * IRC-Hispano IRC Daemon, ircd/ddb.c
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 2004-2018 Toni Garcia (zoltan) <toni@tonigarcia.es>
 * Copyright (C) 1999-2004 Jesus Cea Avion <jcea@jcea.es>
 * Copyright (C) 1999-2000 Jordi Murgo <savage@apostols.org>
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
 * @brief Implementation of Distributed DataBase.
 */
#include "config.h"

#include "ddb.h"
#include "hash.h"
#include "ircd.h"
#include "ircd_alloc.h"
#include "ircd_chattr.h"
#include "ircd_log.h"
#include "ircd_reply.h"
#include "ircd_snprintf.h"
#include "ircd_string.h"
#include "ircd_tea.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_bsd.h"
#include "send.h"

/* #include <assert.h> -- Now using assert in ircd_log.h */
//#include <stdarg.h>
//#include <stdio.h>
#include <string.h>
//#include <time.h>

/** @page ddb Distributed DataBase
 *
 *
 * TODO, explicacion del sistema
 */
/** Count of allocated Ddb structures. */
static int ddbCount = 0;
/** DDB registers cache. */
static struct Ddb ddb_buf_cache[DDB_BUF_CACHE];
/** Buffer cache. */
static int ddb_buf_cache_i = 0;

/** Tables of %DDB. */
struct Ddb **ddb_data_table[DDB_TABLE_MAX];
/** Residents tables of %DDB. */
unsigned int ddb_resident_table[DDB_TABLE_MAX];
/** Registers count of %DDB tables. */
unsigned int ddb_count_table[DDB_TABLE_MAX];
/** ID number of %DDB tables. */
unsigned long ddb_id_table[DDB_TABLE_MAX];
/** Hi hash table. */
unsigned int ddb_hashtable_hi[DDB_TABLE_MAX];
/** Lo hash table. */
unsigned int ddb_hashtable_lo[DDB_TABLE_MAX];
/** File or DB stats of %DDB tables.*/
//struct ddb_stat ddb_stats_table[DDB_TABLE_MAX];

/** Last key on iterator. */
static struct Ddb *ddb_iterator_key = NULL;
/** Last content on iterator. */
static struct Ddb **ddb_iterator_content = NULL;
/** Position of hash on iterator. */
static int ddb_iterator_hash_pos = 0;
/** Length of hash on iterator. */
static int ddb_iterator_hash_len = 0;

/** Verify if a table is resident.
 * @param[in] table Table of the %DDB Distributed DataBase.
 * @return Non-zero if a table is resident; zero is not resident.
 */
int ddb_table_is_resident(unsigned char table)
{
  return ddb_resident_table[table] ? 1 : 0;
}

unsigned long ddb_id_in_table(unsigned char table)
{
  return ddb_id_table[table];
}

unsigned int ddb_count_in_table(unsigned char table)
{
  return ddb_count_table[table];
}

/** Copy a malloc in the memory.
 * @param[in] buf Buffer
 * @param[in] len Length
 * @param[in] p Pointer
 */
static void
DdbCopyMalloc(char *buf, int len, char **p)
{
  char *p2;

  p2 = *p;
  if ((p2) && (strlen(p2) < len))
  {
    MyFree(p2);
    p2 = NULL;
  }
  if (!p2)
  {
    p2 = MyMalloc(len + 1);    /* The '\0' */
    *p = p2;
  }
  memcpy(p2, buf, len);
  p2[len] = '\0';
}

/** Calculates the hash.
 * @param[in] line buffer line reading the tables.
 * @param[in] table Table of the %DDB Distributed DataBase.
 */
static void
ddb_hash_calculate(char *line, unsigned char table)
{
  unsigned int buffer[129 * sizeof(unsigned int)];
  unsigned int *p = buffer;
  unsigned int x[2], v[2], k[2];
  char *p2;

  memset(buffer, 0, sizeof(buffer));
  strncpy((char *)buffer, line, sizeof(buffer) - 1);
  while ((p2 = strchr((char *)buffer, '\n')))
    *p2 = '\0';
  while ((p2 = strchr((char *)buffer, '\r')))
    *p2 = '\0';

  k[0] = k[1] = 0;
  x[0] = ddb_hashtable_hi[table];
  x[1] = ddb_hashtable_lo[table];

  while (*p)
  {
    v[0] = ntohl(*p);
    p++;                        /* No se puede hacer a la vez porque la linea anterior puede ser una expansion de macros */
    v[1] = ntohl(*p);
    p++;                        /* No se puede hacer a la vez porque la linea anterior puede ser una expansion de macros */
    ircd_tea(v, k, x);
  }
  ddb_hashtable_hi[table] = x[0];
  ddb_hashtable_lo[table] = x[1];
}

/** Initialize %DDB Distributed DataBases.
 */
void
ddb_init(void)
{

  ddb_db_init();
  ddb_events_init();

}


/** Sending the %DDB burst tables.
 * @param[in] cptr %Server sending the petition.
 */
void
ddb_burst(struct Client *cptr)
{
  int i;

  sendto_opmask_butone(0, SNO_NETWORK, "Bursting DDB tables");

/* TODO: zlib_microburst */

  sendcmdto_one(&me, CMD_DB, cptr, "* 0 J %lu 2",
                ddb_id_table[DDB_NICKDB]);

  for (i = DDB_INIT; i <= DDB_END; i++)
  {
    if (i != DDB_NICKDB)
      sendcmdto_one(&me, CMD_DB, cptr, "* 0 J %lu %c",
                    ddb_id_table[i], i);
  }

/* TODO: zlib_microburst */
}

/** Initializes %DDB iterator.
 *
 * @return ddb_iterator_key pointer.
 */
static struct Ddb *
ddb_iterator(void)
{
  struct Ddb *ddb;

  if (ddb_iterator_key)
  {
    ddb_iterator_key = ddb_next(ddb_iterator_key);
    if (ddb_iterator_key)
    {
      return ddb_iterator_key;
    }
  }

  /*
   * "ddb_iterator_hash_pos" siempre indica el PROXIMO valor a utilizar.
   */
  while (ddb_iterator_hash_pos < ddb_iterator_hash_len)
  {
    ddb = ddb_iterator_content[ddb_iterator_hash_pos++];
    if (ddb) {
      ddb_iterator_key = ddb;
      return ddb;
    }
  }

  ddb_iterator_key = NULL;
  ddb_iterator_content = NULL;
  ddb_iterator_hash_pos = 0;
  ddb_iterator_hash_len = 0;

  return NULL;
}

/** Initializes iterator for a table.
 * @param[in] table Table of the %DDB Distributed DataBase.
 * @return First active register on a table.
 */
struct Ddb *
ddb_iterator_first(unsigned char table)
{
  assert((table >= DDB_INIT) && (table <= DDB_END));

  ddb_iterator_hash_len = ddb_resident_table[table];
  assert(ddb_iterator_hash_len);

  ddb_iterator_hash_pos = 0;
  ddb_iterator_content = ddb_data_table[table];
  ddb_iterator_key = NULL;

  return ddb_iterator();
}

/** Next iterator.
 * @return Next active register on a table.
 */
struct Ddb *
ddb_iterator_next(void)
{
  assert(ddb_iterator_key);
  assert(ddb_iterator_content);
  assert(ddb_iterator_hash_len);

  return ddb_iterator();
}

/** Find a register by the key (internal).
 * @param[in] table Table of the %DDB Distributed DataBases.
 * @param[in] key Key of the register.
 * @return Pointer of the register.
 */
static struct Ddb *ddb_find_registry_table(unsigned char table, char *key)
{
  struct Ddb *ddb;
  static char *k = 0;
  static int k_len = 0;
  int i = 0, hashi;

  if ((strlen(key) + 1 > k_len) || (!k))
  {
    k_len = strlen(key) + 1;
    if (k)
      MyFree(k);
    k = MyMalloc(k_len);
    if (!k)
      return 0;
  }
  strcpy(k, key);

  /* Step to lowercase */
  while (k[i])
  {
    k[i] = ToLower(k[i]);
    i++;
  }

  hashi = ddb_hash_register(k, ddb_resident_table[table]);

  for (ddb = ddb_data_table[table][hashi]; ddb; ddb = ddb_next(ddb))
  {
    if (!ircd_strcmp(ddb_key(ddb), k))
    {
      assert(0 != ddb_content(ddb));
      return ddb;
    }
  }
  return NULL;
}

/** Find a register by the key.
 * @param[in] table Table of the %DDB Distributed DataBases.
 * @param[in] key Key of the register.
 * @return Pointer of the register.
 */
struct Ddb *ddb_find_key(unsigned char table, char *key)
{
  struct Ddb *ddb;
  char *key_init;
  char *key_end;
  char *content_init;
  char *content_end;

  if (!ddb_resident_table[table])
    return NULL;

  ddb = ddb_find_registry_table(table, key);
  if (!ddb)
    return NULL;

  key_init = ddb_key(ddb);
  key_end = key_init + strlen(key_init);
  content_init = ddb_content(ddb);
  content_end = content_init + strlen(content_init);

  DdbCopyMalloc(key_init, key_end - key_init,
            &ddb_buf_cache[ddb_buf_cache_i].ddb_key);
  DdbCopyMalloc(content_init, content_end - content_init,
            &ddb_buf_cache[ddb_buf_cache_i].ddb_content);
  if (++ddb_buf_cache_i >= DDB_BUF_CACHE)
    ddb_buf_cache_i = 0;

  return ddb;
}

/** When IRCD is reloading, it is executing.
 */
void
ddb_reload(void)
{
  char buf[16];
  unsigned char table;

  log_write(LS_DDB, L_INFO, 0, "Reload Distributed DataBase...");
  sendto_opmask_butone(0, SNO_OLDSNO, "Reload Distributed DataBase...");

  /* ddb_init(); */

  for (table = DDB_INIT; table <= DDB_END; table++)
  {
    if (ddb_id_table[table])
    {
      inttobase64(buf, ddb_hashtable_hi[table], 6);
      inttobase64(buf + 6, ddb_hashtable_lo[table], 6);
      sendto_opmask_butone(0, SNO_OLDSNO, "DDB: '%c'. Last register: %lu. HASH: %s",
                           table, ddb_id_table[table], buf);
    }
  }
}

/** Die the server with an DDB error.
  * @param[in] pattern Format string of message.
  */
void
ddb_die(const char *pattern, ...)
{
  struct Client *acptr;
  char exitmsg[1024], exitmsg2[1024];
  va_list vl;
  int i;

  va_start(vl, pattern);
  ircd_vsnprintf(0, exitmsg2, sizeof(exitmsg2), pattern, vl);
  va_end(vl);

  ircd_snprintf(0, exitmsg, sizeof(exitmsg), "DDB Error: %s", exitmsg2);

  for (i = 0; i <= HighestFd; i++)
  {
    if (!(acptr = LocalClientArray[i]))
      continue;
    if (IsUser(acptr))
      sendcmdto_one(&me, CMD_NOTICE, acptr, "%C :Server Terminating. %s",
                           acptr, exitmsg);
    else if (IsServer(acptr))
      sendcmdto_one(&me, CMD_ERROR, acptr, ":Terminated by %s", exitmsg);
  }
//  exit_schedule(0, 0, 0, exitmsg);
  server_die(exitmsg);
}

/** Finalizes the %DDB subsystem.
 */
void
ddb_end(void)
{
  ddb_db_end();
}

/** Report all F-lines to a user.
 * @param[in] to Client requesting statistics.
 * @param[in] sd Stats descriptor for request (ignored).
 * @param[in] param Extra parameter from user (ignored).
 */
void
ddb_report_stats(struct Client* to, const struct StatDesc* sd, char* param)
{
  unsigned char table;

  for (table = DDB_INIT; table <= DDB_END; table++)
  {
    if (ddb_table_is_resident(table))
      send_reply(to, SND_EXPLICIT | RPL_STATSDEBUG,
                 "b :Table '%c' S=%lu R=%u", table,
                 ddb_id_table[table],
                 ddb_count_table[table]);
    else
    {
      if (ddb_id_table[table])
        send_reply(to, SND_EXPLICIT | RPL_STATSDEBUG,
                   "b :Table '%c' S=%lu NoResident", table,
                   ddb_id_table[table]);
    }
  }
}

/** Find number of DDB structs allocated and memory used by them.
 * @param[out] count_out Receives number of DDB structs allocated.
 * @param[out] bytes_out Receives number of bytes used by DDB structs.
 */
void ddb_count_memory(size_t* count_out, size_t* bytes_out)
{
  assert(0 != count_out);
  assert(0 != bytes_out);
  *count_out = ddbCount;
  *bytes_out = ddbCount * sizeof(struct Ddb);
}
