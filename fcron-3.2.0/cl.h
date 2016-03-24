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

/* cl: Cron Line type and associated functions */

#ifndef __CL_H__
#define __CL_H__

/*
 * TYPES
 */

#define OPTION_SIZE 4           /* number of bytes to hold the cl_option bit array */
#define LAVG_SIZE 3
/* warning : do not change the order of the members of this structure
 *   because some tests made are dependent to that order */
/* warning : if you change a field type, you may have to also make some changes
 *   in the save/load binary fcrontab functions */
typedef struct cl_t {
    struct cl_t *cl_next;
    struct cf_t *cl_file;       /* the file in which the line is           */
    char *cl_shell;             /* shell command                           */
    char *cl_runas;             /* determine permissions of the job        */
    char *cl_mailto;            /* mail output to cl_mailto                */
    char *cl_tz;                /* time zone of the line                   */
    unsigned long cl_id;        /* line's unique id number                 */
    time_t cl_until;            /* timeout of the wait for a lavg value    */
    time_t cl_first;            /* initial delay preserved for volatile entries */
    time_t cl_nextexe;          /* time and date of the next execution     */
    long int cl_timefreq;       /* Run every n seconds                     */
    unsigned short cl_remain;   /* remaining until next execution          */
    unsigned short cl_runfreq;  /* Run once every n matches(=1 for %-lines) */
    unsigned char cl_option[OPTION_SIZE];       /* line's option (see option.h)   */
    unsigned char cl_lavg[LAVG_SIZE];   /*load averages needed (1,5,15 mins)   */
    unsigned char cl_numexe;    /* entries in queues & running processes   */
    char cl_nice;               /* nice value to control priority          */
    unsigned char cl_jitter;    /* run randomly late up to jitter seconds  */
    /* see bitstring(3) man page for more details */
    bitstr_t bit_decl(cl_mins, 60);     /* 0-59                            */
    bitstr_t bit_decl(cl_hrs, 24);      /* 0-23                            */
    bitstr_t bit_decl(cl_days, 32);     /* 1-31                            */
    bitstr_t bit_decl(cl_mons, 12);     /* 0-11                            */
    bitstr_t bit_decl(cl_dow, 8);       /* 0-7, 0 and 7 are both Sunday    */
} cl_t;


/*
 * functions prototypes
 */

/* duplicate a line, including strings it points to */
cl_t *dups_cl(cl_t * orig);
void free_line(cl_t * cl);

#endif                          /* __CL_H__ */
