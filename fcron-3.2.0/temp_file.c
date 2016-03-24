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
#include "temp_file.h"
#include "mem.h"

extern char *tmp_path;
extern char debug_opt;


int
temp_file(char **name)
    /* Open a temporary file and return its file descriptor */
    /* Returns the filename to *name if name is not null. */
    /* (die on error) */
{
    int fd;
#ifdef HAVE_MKSTEMP
    char name_local[PATH_LEN] = "";
    snprintf(name_local, sizeof(name_local), "%sfcr-XXXXXX", tmp_path);
    if ((fd = mkstemp(name_local)) == -1)
        die_e("Can't find a unique temporary filename");
    /* we must set the file mode to 600 (some versions of mkstemp may set it
     * incorrectly) */
    if (fchmod(fd, S_IWUSR | S_IRUSR) != 0)
        die_e("Can't fchmod temp file");
#else
    const int max_retries = 50;
    char *name_local = NULL;
    int i;

    i = 0;
    do {
        i++;
        Set(name_local, tempnam(NULL, NULL));
        if (name_local == NULL)
            die("Can't find a unique temporary filename");
        fd = open(name_local, O_RDWR | O_CREAT | O_EXCL | O_APPEND,
                  S_IRUSR | S_IWUSR);
        /* I'm not sure we actually need to be so persistent here */
    } while (fd == -1 && errno == EEXIST && i < max_retries);
    if (fd == -1)
        die_e("Can't open temporary file");
#endif
    if (name == NULL && unlink(name_local) != 0)
        die_e("Can't unlink temporary file %s", name_local);

    fcntl(fd, F_SETFD, 1);      /* set close-on-exec flag */

    /* give the name of the temp file if necessary */
    if (name != NULL)
        *name = strdup2(name_local);
#ifndef HAVE_MKSTEMP
    free(name_local);
#endif

    return fd;
}
