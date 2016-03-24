/*
 * FCRON - periodic command scheduler 
 *
 *  Copyright 2000-2014 Thibault Godouet <fcron@free.fr>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 *  The GNU General Public License can also be found in the file
 *  `LICENSE' that comes with the fcron source distribution.
 */


/* List of jobs waiting for an appropriate system load average to be executed.
 * This is a wrapper for an u_list (unordered list) (see u_list.h and u_list.c),
 * to make the rest of the code clearer and as a way to ensure the compiler can checks
 * the type in the rest of the code (u_list extensively uses the void type) */

#ifndef __LAVG_LIST_H__
#define __LAVG_LIST_H__

#include "u_list.h"

/* Entry to describe one job waiting for an appropriate load average to be executed */
typedef struct lavg_t {
    struct cl_t *l_line;
    time_t l_until;             /* the timeout of the wait for load averages */
} lavg_t;

typedef struct u_list_t lavg_list_t;

/* functions prototypes */
extern lavg_list_t *lavg_list_init(void);
extern lavg_t *lavg_list_add_line(lavg_list_t * list, struct cl_t *line);
extern lavg_t *lavg_list_add(lavg_list_t * list, lavg_t * entry);
/* WARNING: there should always be a unique iteration loop based on
 *          u_list_first()/u_list_next() running at any one time in the code */
extern lavg_t *lavg_list_first(lavg_list_t * list);
extern lavg_t *lavg_list_next(lavg_list_t * list);
extern void lavg_list_end_iteration(lavg_list_t * list);
extern void lavg_list_remove_cur(lavg_list_t * list);
extern lavg_list_t *lavg_list_destroy(lavg_list_t * list);


#endif                          /* __LAVG_LIST_H__ */
