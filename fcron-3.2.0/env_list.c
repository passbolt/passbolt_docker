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

#include "global.h"
#include "fcron.h"
#include "env_list.h"

env_list_t *
env_list_init(void)
{
    return (env_list_t *) u_list_init(sizeof(env_t),
                                      ENVVAR_INITIAL_SIZE, ENVVAR_GROW_SIZE);
}

env_list_t *
env_list_copy(env_list_t * list)
{
    env_list_t *new_list = NULL;
    env_t *e = NULL;

    /* copy the list structure */
    new_list = (env_list_t *) u_list_copy((u_list_t *) list);

    /* for now the new list points to the same data strings - duplicate them */
    for (e = env_list_first(new_list); e != NULL; e = env_list_next(new_list)) {
        e->e_envvar = strdup2(e->e_envvar);
    }

    return new_list;

}

env_t *
env_list_setenv(env_list_t * list, char *name, char *value, int overwrite)
{
    env_t e = { NULL };
    env_t *c = NULL;
    size_t len = strlen(name) + 1 + strlen(value) + 1;  /* 1 for '=', 1 for '\0' */

    /* sanity check */
    if (name == NULL || name[0] == '\0')
        return NULL;

    /* check if a var 'name' already exists */
    for (c = env_list_first(list); c != NULL; c = env_list_next(list)) {
        if (strcmp_until(name, c->e_envvar, '=') == 0) {
            /* variable already set: overwrite the value if asked
             * and return that entry */
            if (overwrite == 1) {
                c->e_envvar =
                    realloc_safe(c->e_envvar, len, "new env var value");
                snprintf(c->e_envvar, len, "%s=%s", name, value);
            }
            env_list_end_iteration(list);
            return c;
        }
    }

    /* if we're here we didn't find a var called 'name': add it */
    e.e_envvar = alloc_safe(len, "new env var");
    snprintf(e.e_envvar, len, "%s=%s", name, value);
    return (env_t *) u_list_add((u_list_t *) list, (u_list_entry_t *) & e);
}

env_t *
env_list_putenv(env_list_t * list, char *envvar, int overwrite)
{
    env_t e = { NULL };
    env_t *c = NULL;
    size_t len = strlen(envvar) + 1;    /* +1 for the terminating '\0' */

    /* sanity check */
    if (envvar == NULL || envvar[0] == '\0')
        return NULL;

    /* check if a var 'name' already exists */
    for (c = env_list_first(list); c != NULL; c = env_list_next(list)) {
        if (strcmp_until(envvar, c->e_envvar, '=') == 0) {
            /* variable already set: overwrite the value if asked
             * and return that entry */
            if (overwrite == 1) {
                c->e_envvar =
                    realloc_safe(c->e_envvar, len, "new env var value");
                memcpy(c->e_envvar, envvar, len);       /* includes the final '\0' */
            }
            env_list_end_iteration(list);
            return c;
        }
    }

    /* if we're here we didn't find a var called 'name': add it */
    e.e_envvar = strdup2(envvar);
    return (env_t *) u_list_add((u_list_t *) list, (u_list_entry_t *) & e);
}

char *
env_list_getenv(env_list_t * list, char *name)
{
    env_t *c = NULL;

    /* sanity check */
    if (name == NULL || name[0] == '\0')
        return NULL;

    for (c = env_list_first(list); c != NULL; c = env_list_next(list)) {
        if (strcmp_until(name, c->e_envvar, '=') == 0) {
            /* found the var: return the pointer to the value */
            env_list_end_iteration(list);
            return (c->e_envvar + strlen(name) + 1);    /* +1 for '=' */
        }
    }

    /* var 'name' not found */
    return NULL;
}

env_t *
env_list_first(env_list_t * list)
{
    return (env_t *) u_list_first((u_list_t *) list);
}

env_t *
env_list_next(env_list_t * list)
{
    return (env_t *) u_list_next((u_list_t *) list);
}

void
env_list_end_iteration(env_list_t * list)
{
    u_list_end_iteration((u_list_t *) list);
}

env_list_t *
env_list_destroy(env_list_t * list)
    /* free() the memory allocated for list and returns NULL */
{
    env_t *c = NULL;

    /* make sure the iteration below won't fail in case one was already iterating */
    env_list_end_iteration(list);
    /* free the data in the env_t entries */
    for (c = env_list_first(list); c != NULL; c = env_list_next(list)) {
        Free_safe(c->e_envvar);
    }
    /* free the actual list structure */
    return (env_list_t *) u_list_destroy((u_list_t *) list);
}

char **
env_list_export_envp(env_list_t * list)
/* export list data as a char **envp structure to use as an argument of execle() */
{
    env_t *c = NULL;
    int i = 0;
    char **envp = NULL;

    /* +1 for the end-of-list NULL */
    envp =
        alloc_safe((list->num_entries + 1) * sizeof(char *),
                   "environment export");

    for (c = env_list_first(list), i = 0; c != NULL && i < list->num_entries;
         c = env_list_next(list), i++) {
        envp[i] = strdup2(c->e_envvar);
    }
    /* add a NULL as a end-of-list marker */
    envp[(list->num_entries + 1) - 1] = NULL;

    return envp;
}

void
env_list_free_envp(char **envp)
{
    char *p = NULL;

    for (p = envp[0]; p != NULL; p++) {
        Free_safe(p);
    }
    Free_safe(envp);

}
