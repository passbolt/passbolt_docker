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
#include "filesubs.h"

/* close() a file, and set the FD to -1.
 * Returns close()'s return value and leaves errno as is. */
int
xclose(int *fd)
{
    int retval = -1;

    if (*fd != -1) {
        retval = close(*fd);
        *fd = -1;
    }

    return retval;
}

/* close() a file, and set the FD to -1. Check for errors and log them.
 * Returns close()'s return value.
 * WARNING: do NOT call from log.c to avoid potential infinite loops! */
int
xclose_check(int *fd, const char *filedesc)
{
    int retval = -1;

    if (*fd != -1) {
        retval = close(*fd);
        if (retval != 0) {
            error_e("Error while closing %s", filedesc);
        }
        *fd = -1;
    }

    return retval;
}

/* fclose() a file, and set the FILE* to NULL.
 * Returns fclose()'s return value and leaves errno as is. */
int
xfclose(FILE ** f)
{
    int retval = EOF;

    if (f == NULL) {
        return retval;
    }

    if (*f != NULL) {
        retval = fclose(*f);
        *f = NULL;
    }

    return retval;
}

/* fclose() a file, and set the FILE* to NULL. Check for errors and log them.
 * Returns fclose()'s return value.
 * WARNING: do NOT call from log.c to avoid potential infinite loops! */
int
xfclose_check(FILE ** f, const char *filedesc)
{
    int retval = EOF;

    if (f == NULL) {
        return retval;
    }

    if (*f != NULL) {
        retval = fclose(*f);
        if (retval != 0) {
            error_e("Error while fclosing %s", filedesc);
        }
        *f = NULL;
    }

    return retval;
}
