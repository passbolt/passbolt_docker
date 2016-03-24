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


/* List of jobs currently being executed.
 * This is a wrapper for an u_list (unordered list) (see u_list.h and u_list.c),
 * to make the rest of the code clearer and as a way to ensure the compiler can checks
 * the type in the rest of the code (u_list extensively uses the void type) */

#ifndef __EXE_LIST_H__
#define __EXE_LIST_H__

#include "u_list.h"

/* Entry to describe one job being executed */
typedef struct exe_t {
    struct cl_t *e_line;
    pid_t e_ctrl_pid;           /* pid of the fcron process controling the job */
    pid_t e_job_pid;            /* pid of the job itself */
} exe_t;

typedef struct u_list_t exe_list_t;

/* functions prototypes */
extern exe_list_t *exe_list_init(void);
extern exe_t *exe_list_add_line(exe_list_t * list, struct cl_t *line);
extern exe_t *exe_list_add(exe_list_t * list, exe_t * e);
/* WARNING: there should always be a unique iteration loop based on
 *          u_list_first()/u_list_next() running at any one time in the code */
extern exe_t *exe_list_first(exe_list_t * list);
extern exe_t *exe_list_next(exe_list_t * list);
extern void exe_list_end_iteration(exe_list_t * list);
extern void exe_list_remove_cur(exe_list_t * list);
extern exe_list_t *exe_list_destroy(exe_list_t * list);


#endif                          /* __EXE_LIST_H__ */
