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


/*
 * 'First in first out' list of generic items
 */

#ifndef __FIFO_LIST_H__
#define __FIFO_LIST_H__

#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include "subs.h"

typedef void fifo_list_entry_t;

typedef struct fifo_list_t {
    /* PUBLIC: */
    int max_entries;            /* max allowed element number (0: no limit) */
    int num_entries;            /* READ ONLY: num of entries in the list now */
    /* PRIVATE: DO NOT ACCESS DIRECTLY */
    int array_size;             /* size of the array (in number of entries) */
    size_t entry_size;          /* number of element currently in the array */
    int grow_size;              /* grow array by grow_size entries at a time */
    u_list_entry_t *first_entry;        /* Current entry in iteration */
    u_list_entry_t *cur_entry;  /* Current entry in iteration
                                 * (null if not in iteration, i.e. X_first() has
                                 * not been called or we reached the list end */
    u_list_entry_t *entries_array;      /* pointer to the actual array */
} fifo_list_t;

/* functions prototypes */
extern fifo_list_t *fifo_list_init(size_t entry_size, int init_size,
                                   int grow_size);
extern fifo_list_entry_t *fifo_list_add(fifo_list_t * list,
                                        fifo_list_entry_t * entry);
extern fifo_list_entry_t *fifo_list_first(fifo_list_t * list);
extern fifo_list_entry_t *fifo_list_next(fifo_list_t * list);
extern void fifo_list_end_iteration(fifo_list_t * list);
extern void fifo_list_remove_first(fifo_list_t * list);
extern fifo_list_t *fifo_list_destroy(fifo_list_t * list);

#endif                          /* __FIFO_LIST_H__ */
