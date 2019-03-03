/*
 * IRC-Hispano IRC Daemon, include/ddb.h
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
 * @brief Distributed DataBases structures, macros and functions.
 */
#ifndef INCLUDED_ddb_h
#define INCLUDED_ddb_h

#include "config.h"
#include "client.h"


#if defined(DDB)

/*
 * General defines
 */
/** Maxium number of tables */
#define DDB_TABLE_MAX      256
/** Number of keys is caching */
#define DDB_BUF_CACHE  32

/*
 * Distributed DataBases Tables
 */
/** First table of %DDB Distributed Databases. */
#define DDB_INIT           'a'
/** Bots table of %DDB Distributed Databases. */
#define DDB_BOTDB          'b'
/** Channels table of %DDB Distributed Databases. */
#define DDB_CHANDB         'c'
/** Channels table (nicks) of %DDB Distributed Databases. */
#define DDB_CHANDB2        'd'
/** Exceptions table of %DDB Distributed Databases. */
#define DDB_EXCEPTIONDB    'e'
/** Features table of %DDB Distributed Databases. */
#define DDB_FEATUREDB      'f'
/** Ilines table of %DDB Distributed Databases. */
#define DDB_ILINEDB        'i'
/** Jupes table of %DDB Distributed Databases. */
#define DDB_JUPEDB         'j'
/** Logging table of %DDB Distributed Databases. */
#define DDB_LOGGINGDB      'l'
/** Motd table of %DDB Distributed Databases. */
#define DDB_MOTDDB         'm'
/** Nicks table of %DDB Distributed Databases. */
#define DDB_NICKDB         'n'
/** Operators table of %DDB Distributed Databases. */
#define DDB_OPERDB         'o'
/** Pseudocommands table of %DDB Distributed Databases. */
#define DDB_PSEUDODB       'p'
/** Quarantine channels table of %DDB Distributed Databases. */
#define DDB_QUARANTINEDB   'q'
/** Channel redirections table of %DDB Distributed Databases. */
#define DDB_CHANREDIRECTDB 'r'
/** Spam Filter table of %DDB Distributed Databases. */
#define DDB_SPAMDB         's'
/** Uworld table of %DDB Distributed Databases. */
#define DDB_UWORLDDB       'u'
/** Vhost table of %DDB Distributed Databases. */
#define DDB_VHOSTDB        'v'
/** WebIRC table of %DDB Distributed Databases. */
#define DDB_WEBIRCDB       'w'
/** Last table of %DDB Distributed Databases. */
#define DDB_END            'z'

/*
 * PseudoBots
 */
/** Nickname of virtual bot for nicks registers */
#define DDB_NICKSERV    "NickServ"
/** Nickname of virtual bot for channel bot */
#define DDB_CHANSERV   "ChanServ"


/** Describes a key on one table.
 */
struct Ddb {
  char*     ddb_key;    /**< Key of the register */
  char*     ddb_content;    /**< Content of the key */
  struct Ddb*   ddb_next;   /**< Next key on the table */
};

/** Get key of the register. */
#define ddb_key(ddb)        ((ddb)->ddb_key)
/** Get content of the key. */
#define ddb_content(ddb)    ((ddb)->ddb_content)
/** Get next key on the table. */
#define ddb_next(ddb)       ((ddb)->ddb_next)

/** An copy of kernel structure stat.
 */
struct ddb_stat {
  dev_t  dev;       /**< ID of device containing a directory entry for this file */
  ino_t  ino;       /**< Inode number */
  off_t  size;      /**< File size in bytes */
  time_t mtime;     /**< Time of last data modification */
};


/** DDB Macro for allocations. */
#define DdbMalloc(x)	MyMalloc(x)
/** DDB Macro for freeing memory. */
#define DdbFree(x)	MyFree(x)

/*
 * Prototypes
 */
extern struct Ddb **ddb_data_table[DDB_TABLE_MAX];
extern struct ddb_stat ddb_stats_table[DDB_TABLE_MAX];
extern unsigned int ddb_resident_table[DDB_TABLE_MAX];
extern unsigned int ddb_count_table[DDB_TABLE_MAX];
extern unsigned long ddb_id_table[DDB_TABLE_MAX];
typedef void (*ddb_events_table_td)(char *, char *, int);
extern ddb_events_table_td ddb_events_table[DDB_TABLE_MAX];
extern unsigned int ddb_hashtable_hi[DDB_TABLE_MAX];
extern unsigned int ddb_hashtable_lo[DDB_TABLE_MAX];

extern int ddb_table_is_resident(unsigned char table);
extern unsigned long ddb_id_in_table(unsigned char table);
extern unsigned int ddb_count_in_table(unsigned char table);

extern void ddb_init(void);
extern void ddb_events_init(void);
extern void ddb_end(void);

extern void ddb_new_register(struct Client *cptr, unsigned char table, unsigned long id, char *mask, char *key, char *content);
extern void ddb_drop(unsigned char table);
extern void ddb_drop_memory(unsigned char table, int events);
extern void ddb_compact(unsigned char table, unsigned long id, char *content);
extern void ddb_burst(struct Client *cptr);
extern int ddb_table_burst(struct Client *cptr, unsigned char table, unsigned long id);

extern struct Ddb *ddb_iterator_first(unsigned char table);
extern struct Ddb *ddb_iterator_next(void);
extern struct Ddb *ddb_find_key(unsigned char table, char *key);
extern char *ddb_get_botname(char *botname);

extern void ddb_splithubs(struct Client *cptr, unsigned char table, char *exitmsg);
extern void ddb_reload(void);
extern void ddb_die(const char *pattern, ...);
extern void ddb_report_stats(struct Client* to, const struct StatDesc* sd, char* param);
extern void ddb_count_memory(size_t* count_out, size_t* bytes_out);

/* ddb_db_*.c externs */
extern void ddb_db_init(void);
extern int ddb_db_cache(void);
extern int ddb_db_read(struct Client *cptr, unsigned char table, unsigned long id, int count);
extern void ddb_db_write(unsigned char table, unsigned long id, char *mask, char *key, char *content);
extern void ddb_db_drop(unsigned char table);
extern void ddb_db_compact(unsigned char table);
extern void ddb_db_hash_read(unsigned char table, unsigned int *hi, unsigned int *lo);
extern void ddb_db_hash_write(unsigned char table);
extern void ddb_db_end(void);

#endif /* defined(DDB) */

#endif /* INCLUDED_ddb_h */

