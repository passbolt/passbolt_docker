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

#ifndef __ENV_LIST_H__
#define __ENV_LIST_H__

#include "u_list.h"

/* One environment variable assignation */
typedef struct env_t {
    char *e_envvar;
} env_t;

typedef struct u_list_t env_list_t;

/* functions prototypes */
extern env_list_t *env_list_init(void);
extern env_list_t *env_list_copy(env_list_t * list);
/* WARNING: - These functions are NOT re-entrant.
 *            i.e. there should always be a unique iteration loop based on
 *            u_list_first()/u_list_next() running at any one time in the code
 *          - setenv, putenv, getenv  use an iteration internally
 *            so they cannot be called when already iterating */
extern env_t *env_list_setenv(env_list_t * list, char *name, char *value,
                              int overwrite);
extern env_t *env_list_putenv(env_list_t * list, char *envvar, int overwrite);
extern char *env_list_getenv(env_list_t * list, char *name);
extern env_t *env_list_first(env_list_t * list);
extern env_t *env_list_next(env_list_t * list);
extern void env_list_end_iteration(env_list_t * list);
extern env_list_t *env_list_destroy(env_list_t * list);

extern char **env_list_export_envp(env_list_t * list);
extern void env_list_free_envp(char **envp);


#endif                          /* __ENV_LIST_H__ */
