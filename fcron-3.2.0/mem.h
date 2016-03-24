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

/* mem: manage memory (de)allocation.
 * Mostly wrappers around standard functions to check for errors.
 * We also always set variable to NULL after free()ing them, and check
 * if a variable is NULL before attempting to free it. */

#ifndef __MEM_H__
#define __MEM_H__

/* macros */
#define Alloc(PTR, TYPE) \
{ \
    if ( ( (PTR)=calloc(1, sizeof(TYPE)) ) == NULL ) { \
        die_e("Could not calloc."); \
    } \
}

#define Free_safe(PTR) \
{ \
    if ((PTR) != NULL) { \
        free((PTR)); \
        (PTR) = NULL; \
    } \
}

#define Set(VAR, VALUE) \
{ \
    Free_safe((VAR)); \
    (VAR) = strdup2((VALUE)); \
}


/* functions prototypes */
#if defined(__sun)
extern char *strndup(const char *s, size_t n);  /* Solaris 10 has no strndup() */
#endif
extern char *strdup2(const char *);
extern char *strndup2(const char *, size_t n);
extern void *alloc_safe(size_t len, const char *desc);
extern void *realloc_safe(void *ptr, size_t len, const char *desc);

#endif                          /* __MEM_H__ */
