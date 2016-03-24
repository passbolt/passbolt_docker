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
#include "mem.h"

#if defined(__sun)
/* Solaris 10 has no strndup() */
char *
strndup(const char *s, size_t n)
    /* Written by Kaveh R. Ghazi <ghazi@caip.rutgers.edu> */
{
    char *result;
    size_t len = strlen(s);

    if (n < len)
        len = n;

    result = (char *)malloc(len + 1);
    if (!result)
        return 0;

    memcpy(result, s, len);
    result[len] = '\0';
    return (result);
}
#endif

char *
strdup2(const char *str)
{
    char *ptr;

    if (str == NULL)
        return NULL;

    ptr = strdup(str);

    if (!ptr)
        die_e("Could not strdup()");

    return (ptr);
}

char *
strndup2(const char *str, size_t n)
{
    char *ptr;

    if (str == NULL)
        return NULL;

    /* note: if n==0 then ptr will be an empty string (non-NULL) */
    ptr = strndup(str, n);

    if (!ptr)
        die_e("Could not strdup()");

    return (ptr);
}

void *
alloc_safe(size_t len, const char *desc)
/* allocate len-bytes of memory, and return the pointer.
 * Die with a log message if there is any error */
{
    void *ptr = NULL;

    ptr = calloc(1, len);
    if (ptr == NULL) {
        die_e("Could not allocate %d bytes of memory%s%s", len,
              (desc) ? "for " : "", desc);
    }
    return ptr;
}

void *
realloc_safe(void *cur, size_t len, const char *desc)
/* allocate len-bytes of memory, and return the pointer.
 * Die with a log message if there is any error */
{
    void *new = NULL;

    new = realloc(cur, len);
    if (new == NULL) {
        die_e("Could not reallocate %d bytes of memory%s%s", len,
              (desc) ? "for " : "", desc);
    }
    return new;
}
