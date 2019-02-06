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
#include "list.h"
#include "match.h"
#include "msg.h"
#include "numeric.h"
#include "numnicks.h"
#include "s_bsd.h"
#include "s_debug.h"
#include "s_misc.h"
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
struct ddb_stat ddb_stats_table[DDB_TABLE_MAX];

/** Last key on iterator. */
static struct Ddb *ddb_iterator_key = NULL;
/** Last content on iterator. */
static struct Ddb **ddb_iterator_content = NULL;
/** Position of hash on iterator. */
static int ddb_iterator_hash_pos = 0;
/** Length of hash on iterator. */
static int ddb_iterator_hash_len = 0;

static void ddb_table_init(unsigned char table);
static int ddb_add_key(unsigned char table, char *key, char *content);
static int ddb_del_key(unsigned char table, char *key);

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
  unsigned char table;

  ddb_db_init();
  ddb_events_init();

  memset(ddb_resident_table, 0, sizeof(ddb_resident_table));

  /*
   * The lengths MUST be powers of 2 and do not have
   * to be superior to HASHSIZE.
   */
  ddb_resident_table[DDB_BOTDB]          =   256;
  ddb_resident_table[DDB_CHANDB]         =  4096;
  ddb_resident_table[DDB_CHANDB2]        =  4096;
  ddb_resident_table[DDB_EXCEPTIONDB]    =   512;
  ddb_resident_table[DDB_FEATUREDB]      =   256;
  ddb_resident_table[DDB_ILINEDB]        =   512;
  ddb_resident_table[DDB_JUPEDB]         =   512;
  ddb_resident_table[DDB_LOGGINGDB]      =   256;
  ddb_resident_table[DDB_MOTDDB]         =   256;
  ddb_resident_table[DDB_NICKDB]         = 32768;
  ddb_resident_table[DDB_OPERDB]         =   256;
  ddb_resident_table[DDB_PSEUDODB]       =   256;
  ddb_resident_table[DDB_QUARANTINEDB]   =   256;
  ddb_resident_table[DDB_CHANREDIRECTDB] =   256;
  ddb_resident_table[DDB_UWORLDDB]       =   256;
  ddb_resident_table[DDB_VHOSTDB]        =  4096;
  ddb_resident_table[DDB_WEBIRCDB]       =   256;

  if (!ddb_db_cache()) {
    for (table = DDB_INIT; table <= DDB_END; table++)
      ddb_table_init(table);
  }

  /*
   * The previous operation it can be a long operation.
   * Updates time.
   */
  CurrentTime = time(NULL);
}

/** Initialize a table of %DDB.
 * @param[in] table
 */
static void ddb_table_init(unsigned char table)
{
  unsigned int hi, lo;

  /* First drop table */
  ddb_drop_memory(table, 0);

  /* Read the table on file or database */
  ddb_db_read(NULL, table, 0, 0);

  /* Read hashes */
  ddb_db_hash_read(table, &hi, &lo);

  /* Compare memory hashes with local hashes */
  sendto_opmask_butone(0, SNO_OLDSNO, "Lo: %d Hashtable_Lo: %d Hi: %d Hashtable_Hi %d",
                lo, ddb_hashtable_lo[table], hi, ddb_hashtable_hi[table]);

  if ((ddb_hashtable_hi[table] != hi) || (ddb_hashtable_lo[table] != lo))
  {
    struct DLink *lp;
    char buf[1024];

    log_write(LS_DDB, L_INFO, 0, "WARNING - Table '%c' is corrupt. Droping table...", table);
    ddb_drop(table);

    ircd_snprintf(0, buf, sizeof(buf), "Table '%c' is corrupt. Reloading via remote burst...", table);
    //ddb_splithubs(NULL, table, buf);
    //log_write(LS_DDB, L_INFO, 0, "Solicit a copy of table '%s' to neighboring nodes", table);

    /*
     * Corta conexiones con los HUBs
     */
  drop_hubs:
    for (lp = cli_serv(&me)->down; lp; lp = lp->next)
    {
      if (IsHub(lp->value.cptr)) {
        ircd_snprintf(0, buf, sizeof(buf), "Table '%c' is corrupt. Resync...", table);
        exit_client(lp->value.cptr, lp->value.cptr, &me, buf);
        goto drop_hubs;
      }
    }

    ddb_splithubs(NULL, table, buf);
    sendto_opmask_butone(0, SNO_OLDSNO, "Solicit DDB update table '%c'", table);

    /*
     * Solo pide a los HUBs, porque no se
     * aceptan datos de leafs.
     */
    for (lp = cli_serv(&me)->down; lp; lp = lp->next)
    {
      if (IsHub(lp->value.cptr))
      {
        sendcmdto_one(&me, CMD_DB, lp->value.cptr, "%C 0 J %u %c",
                      lp->value.cptr, ddb_id_table[table], table);
      }
    }
  }

  ddb_db_hash_write(table);

  /*
   * Si hemos leido algun registro de compactado,
   * sencillamente nos lo cargamos en memoria, para
   * que cuando llegue otro, el antiguo se borre
   * aunque tenga algun contenido (un texto explicativo, por ejemplo).
   */
  if (ddb_resident_table[table])
  {
    ddb_del_key(table, "*");
    log_write(LS_DDB, L_INFO, 0, "Loading Table '%c' finished: S=%u R=%u",
              table, ddb_id_table[table], ddb_count_table[table]);
  }
  else if (ddb_count_table[table])
    log_write(LS_DDB, L_INFO, 0, "Loading Table '%c' finished: S=%u NoResident",
              table, ddb_id_table[table]);
}

/** Add a new register from the network or reading when ircd is starting.
 * @param[in] cptr %Server sending a new register. If is NULL, it is own server during ircd start.
 * @param[in] table Table of the %DDB Distributed DataBases.
 */
void
ddb_new_register(struct Client *cptr, unsigned char table, unsigned long id, char *mask, char *key, char *content)
{
  static char *keytemp = NULL;
  static int key_len = 0;
  char db_buf[1024];

  assert(0 != table);
  assert(0 != key);

  /* Para el calculo del hash */
  if (content)
    ircd_snprintf(0, db_buf, sizeof(db_buf), "%lu %s %c %s %s\n", id, mask, table, key, content);
  else
    ircd_snprintf(0, db_buf, sizeof(db_buf), "%lu %s %c %s\n", id, mask, table, key);

  ddb_hash_calculate(db_buf, table);

  /* In the ircd starting, cptr is NULL and it not writing on file or database */
  if (cptr)
  {
    ddb_db_write(table, id, mask, key, content);
    ddb_db_hash_write(table);
  }

  ddb_id_table[table] = id;

  /* If the table is not resident, do not save in memory */
  if (!ddb_resident_table[table])
    return;

  /* If a mask is not concerned with me, do not save in memory */
  /* For lastNNServer bug (find_match_server) it use collapse + match */
  collapse(mask);
  if (!match(mask, cli_name(&me)))
  {
    int update = 0, i = 0;

    if ((strlen(key) + 1 > key_len) || (!keytemp))
    {
      key_len = strlen(key) + 1;
      if (keytemp)
        MyFree(keytemp);
      keytemp = MyMalloc(key_len);

      assert(0 != keytemp);
    }
    strcpy(keytemp, key);

    while (keytemp[i])
    {
      keytemp[i] = ToLower(keytemp[i]);
      i++;
    }

    if (content)
      update = ddb_add_key(table, keytemp, content);
    else
      ddb_del_key(table, keytemp);

    if (ddb_events_table[table])
      ddb_events_table[table](key, content, update);
  }
}

/** Add or update an register on the memory.
 * @param[in] table Table of the %DDB Distributed DataBases.
 * @param[in] key Key of the register.
 * @param[in] content Content of the key.
 * @return 1 is an update, 0 is a new register.
 */
static int
ddb_add_key(unsigned char table, char *key, char *content)
{
  struct Ddb *ddb;
  char *k, *c;
  int hashi;
  int delete = 0;

  ddb_iterator_key = NULL;

  delete = ddb_del_key(table, key);

  ddb = DdbMalloc(sizeof(struct Ddb) + strlen(key) + strlen(content) + 2);
  assert(0 != ddb);

  k = (char *)ddb + sizeof(struct Ddb);
  c = k + strlen(key) + 1;

  strcpy(k, key);
  strcpy(c, content);

  ddb_key(ddb) = k;
  ddb_content(ddb) = c;
  ddb_next(ddb) = NULL;

  hashi = ddb_hash_register(ddb_key(ddb), ddb_resident_table[table]);

  Debug((DEBUG_INFO, "Add DDB: T='%c' K='%s' C='%s' H=%u", table, ddb_key(ddb), ddb_content(ddb), hashi));

  ddb_next(ddb) = ddb_data_table[table][hashi];
  ddb_data_table[table][hashi] = ddb;
  ddb_count_table[table]++;
  ddbCount++;

  return delete;
}

/** Delete an register from memory.
 * @param[in] table Table of the %DDB Distributed DataBases.
 * @param[in] key Key of the register.
 * @return 1 on success; 0 do not delete.
 */
static int
ddb_del_key(unsigned char table, char *key)
{
  struct Ddb *ddb, *ddb2, **ddb3;
  int hashi;
  int delete = 0;

  ddb_iterator_key = NULL;

  hashi = ddb_hash_register(key, ddb_resident_table[table]);
  ddb3 = &ddb_data_table[table][hashi];

  for (ddb = *ddb3; ddb; ddb = ddb2)
  {
    ddb2 = ddb_next(ddb);
    if (!strcmp(ddb_key(ddb), key))
    {
      *ddb3 = ddb2;
      delete = 1;
      DdbFree(ddb);
      ddb_count_table[table]--;
      ddbCount--;
      break;
    }
    ddb3 = &(ddb_next(ddb));
  }
  return delete;
}

/** Deletes a table.
 * @param[in] table Table of the %DDB Distributed DataBases.
 */
void
ddb_drop(unsigned char table)
{

  /* Delete file or database of the table */
  ddb_db_drop(table);

  /* Delete table from memory */
  ddb_drop_memory(table, 0);

  /* Write hash on file or database */
  ddb_db_hash_write(table);
}

/** Deletes a table from memory.
 * @param[in] table Table of the %DDB Distributed DataBases.
 * @param[in] events Non-zero impliques events.
 */
void
ddb_drop_memory(unsigned char table, int events)
{
  struct Ddb *ddb, *ddb2;
  int i, n;

  ddb_id_table[table] = 0;
  ddb_count_table[table] = 0;
  ddb_hashtable_hi[table] = 0;
  ddb_hashtable_lo[table] = 0;

  n = ddb_resident_table[table];
  if (!n)
    return;

  if (ddb_data_table[table])
  {
    for (i = 0; i < n; i++)
    {
      for (ddb = ddb_data_table[table][i]; ddb; ddb = ddb2)
      {
        ddb2 = ddb_next(ddb);

        if (events && ddb_events_table[table])
          ddb_events_table[table](ddb_key(ddb), NULL, 0);

        DdbFree(ddb);
      }
    }
  }
  else
  {                             /* NO tenemos memoria para esa tabla, asi que la pedimos */
    ddb_data_table[table] = DdbMalloc(n * sizeof(struct Ddb *));
    assert(ddb_data_table[table]);
  }

  for (i = 0; i < n; i++)
    ddb_data_table[table][i] = NULL;

}

/** Packing the table.
 * @param[in] table Table of the %DDB Distributed DataBases.
 * @param[in] id Identify number of the register.
 * @param[in] content Content of the key.
 */
void
ddb_compact(unsigned char table, unsigned long id, char *content)
{
  log_write(LS_DDB, L_INFO, 0, "Packing table '%c'", table);
  ddb_id_table[table] = id;
  ddb_db_compact(table);

  //ddb_hash_calculate(table, id, "*", "*", content);
  ddb_db_hash_write(table);
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

/** Split all Hubs but one.
 * @param[in] cptr Client that is not closed.
 * @param[in] exitmsg SQUIT message.
 */
void
ddb_splithubs(struct Client *cptr, unsigned char table, char *exitmsg)
{
  struct Client *acptr;
  struct DLink *lp;
  char buf[1024];
  int num_hubs = 0;

  for (lp = cli_serv(&me)->down; lp; lp = lp->next)
  {
    if (IsHub(lp->value.cptr))
      num_hubs++;
  }

  if (num_hubs >= 2)
  {
    /*
     * No podemos simplemente hace el bucle, porque
     * el "exit_client()" modifica la estructura
     */
corta:
    if (num_hubs-- > 1)
    {                           /* Deja un HUB conectado */
      for (lp = cli_serv(&me)->down; lp; lp = lp->next)
      {
        acptr = lp->value.cptr;
        /*
         * Si se especifica que se desea mantener un HUB
         * en concreto, respeta esa peticion
         */
        if ((acptr != cptr) && IsHub(acptr))
        {
          ircd_snprintf(0, buf, sizeof(buf), "DDB '%c' %s. Resynchronizing...",
                        table, exitmsg);
          exit_client(acptr, acptr, &me, buf);
          goto corta;
        }
      }
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
