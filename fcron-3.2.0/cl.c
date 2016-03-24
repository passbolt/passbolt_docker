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

#include "global.h"
#include "cl.h"
#include "mem.h"

cl_t *
dups_cl(cl_t * orig)
    /* Duplicate a line, including the strings it points to. */
{
    cl_t *cl = NULL;

    Alloc(cl, cl_t);

    /* copy the structure */
    *cl = *orig;

    /* don't assume any link to a file or belonging to a list */
    cl->cl_file = NULL;
    cl->cl_next = NULL;

    /* we just copied the pointers of orig to cl, but we didn't
     * make a copy of the strings yet.
     * Reset the pointers, then copy the strings */
    cl->cl_shell = NULL;
    Set(cl->cl_shell, orig->cl_shell);

    cl->cl_runas = NULL;
    Set(cl->cl_runas, orig->cl_runas);
    debug("%s: Set cl->cl_runas=%s", __func__,
          (cl->cl_runas == NULL) ? "null" : cl->cl_runas);

    cl->cl_mailto = NULL;
    Set(cl->cl_mailto, orig->cl_mailto);

    cl->cl_tz = NULL;
    Set(cl->cl_tz, orig->cl_tz);

    return cl;
}

void
free_line(cl_t * cl)
    /* free a line, including its fields */
{
    if (cl != NULL) {
        Free_safe(cl->cl_shell);
        Free_safe(cl->cl_runas);
        Free_safe(cl->cl_mailto);
        Free_safe(cl->cl_tz);
        Free_safe(cl);
    }
}
