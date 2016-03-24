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
#include "exe_list.h"

exe_list_t *
exe_list_init(void)
{
    return (exe_list_t *) u_list_init(sizeof(exe_t), EXE_INITIAL_SIZE,
                                      EXE_GROW_SIZE);
}

exe_t *
exe_list_add_line(exe_list_t * list, struct cl_t *line)
{
    exe_t e = { NULL, 0, 0 };
    e.e_line = line;            /* ANSI C does not allow us to directly replace NULL by line above */

    return (exe_t *) u_list_add((u_list_t *) list, (u_list_entry_t *) & e);
}

exe_t *
exe_list_add(exe_list_t * list, exe_t * e)
{
    return (exe_t *) u_list_add((u_list_t *) list, (u_list_entry_t *) e);
}

exe_t *
exe_list_first(exe_list_t * list)
{
    return (exe_t *) u_list_first((u_list_t *) list);
}

exe_t *
exe_list_next(exe_list_t * list)
{
    return (exe_t *) u_list_next((u_list_t *) list);
}

void
exe_list_end_iteration(exe_list_t * list)
{
    u_list_end_iteration((u_list_t *) list);
}

void
exe_list_remove_cur(exe_list_t * list)
{
    u_list_remove_cur((u_list_t *) list);
}

exe_list_t *
exe_list_destroy(exe_list_t * list)
    /* free() the memory allocated for list and returns NULL */
{
    return (exe_list_t *) u_list_destroy((u_list_t *) list);
}
