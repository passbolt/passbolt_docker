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

#include "global.h"
#include "fcron.h"
#include "lavg_list.h"

lavg_list_t *
lavg_list_init(void)
{
    lavg_list_t *l =
        (lavg_list_t *) u_list_init(sizeof(lavg_t), LAVG_INITIAL_SIZE,
                                    LAVG_GROW_SIZE);
    l->max_entries = LAVG_QUEUE_MAX;
    return l;
}

lavg_t *
lavg_list_add_line(lavg_list_t * list, struct cl_t * line)
{
    lavg_t e = { NULL, 0 };
    e.l_line = line;            /* ANSI C does not allow us to directly replace NULL by line above */

    return (lavg_t *) u_list_add((u_list_t *) list, (u_list_entry_t *) & e);
}

lavg_t *
lavg_list_add(lavg_list_t * list, lavg_t * entry)
{
    return (lavg_t *) u_list_add((u_list_t *) list, (u_list_entry_t *) entry);
}

lavg_t *
lavg_list_first(lavg_list_t * list)
{
    return (lavg_t *) u_list_first((u_list_t *) list);
}

lavg_t *
lavg_list_next(lavg_list_t * list)
{
    return (lavg_t *) u_list_next((u_list_t *) list);
}

void
lavg_list_end_iteration(lavg_list_t * list)
{
    u_list_end_iteration((u_list_t *) list);
}

void
lavg_list_remove_cur(lavg_list_t * list)
{
    u_list_remove_cur((u_list_t *) list);
}

lavg_list_t *
lavg_list_destroy(lavg_list_t * list)
{
    return (lavg_list_t *) u_list_destroy((u_list_t *) list);
}
