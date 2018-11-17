/*
 * IRC-Hispano IRC Daemon, include/crule.h
 *
 * Copyright (C) 1997-2019 IRC-Hispano Development Team <toni@tonigarcia.es>
 * Copyright (C) 1990 Darren Reed
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
/** @file crule.h
 * @brief Interfaces and declarations for connection rule checking.
 */
#ifndef INCLUDED_crule_h
#define INCLUDED_crule_h

/*
 * Proto types
 */

/*
 * opaque node pointer
 */
struct CRuleNode;

extern void crule_free(struct CRuleNode** elem);
extern int crule_eval(struct CRuleNode* rule);
extern struct CRuleNode* crule_parse(const char* rule);

#endif /* INCLUDED_crule_h */
