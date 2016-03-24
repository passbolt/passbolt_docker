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


#ifndef __SAVE_H__
#define __SAVE_H__

/* functions defined by save.c */
extern int write_file_to_disk(int fd, struct cf_t *file, time_t time_date);
extern int save_file_safe(cf_t * file, char *final_path, char *prog_name,
                          uid_t own_uid, gid_t own_gid, time_t save_date);


/* here is the format fcron(tab) uses to save the fcrontabs :
 *   type(short int)
 *   size(short int)
 *   data
 * unless for some special tags, which don't have data (size set to 0) */


/* kept for backward compatibility : we now have an extended
 * save format with no use of such a fileversion (unless to know that
 * the save file is in extended format, not an old one) */
#define S_FILEVERSION 100       /* syntax's version of fcrontabs */


/* type constants : DO NOT REMOVE ANY of them : they are used for backward
 *                  compatibility.
 *                  If necessary, add some. */

/* type is an unsigned short int */

/* header file types */
#define S_HEADER_T     1        /* file version */
#define S_USER_T       2        /* name of the fcrontab's owner */
#define S_TIMEDATE_T   3        /* time and date of saving */
#define S_TZDIFF_T     4        /* time diff between the local and system hour */

/* env var */
#define S_ENVVAR_T     1000     /* an environment variable */

/* line field types */
#define S_ENDLINE_T    2000     /* we need to know where a new line begin */
#define S_SHELL_T      2001     /* shell command of the line */
#define S_NUMEXE_T     2002     /* num of entries in queues & running processes */
#define S_LAVG_T       2003     /* load averages needed (1, 5, 15 mins) */
#define S_UNTIL_T      2004     /* timeout of the wait for a lavg value */
#define S_NICE_T       2005     /* nice value to control priority */
#define S_RUNAS_T      2006     /* determine permissions of the job */
#define S_MAILTO_T     2007     /* mail output to cl_mailto */
#define S_NEXTEXE_T    2008     /* time and date of the next execution */
#define S_REMAIN_T     2009     /* remaining until next execution */
#define S_TIMEFREQ_T   2010     /* Run every n seconds */
#define S_RUNFREQ_T    2011     /* Run once every n matches */
#define S_MINS_T       2012     /* mins boolean array */
#define S_HRS_T        2013     /* hrs boolean array */
#define S_DAYS_T       2014     /* days boolean array */
#define S_MONS_T       2015     /* mons boolean array */
#define S_DOW_T        2016     /* dow boolean array */
#define S_OPTION_T     2017     /* options for that line (see option.h) */
#define S_FIRST_T      2018     /* wait time before first execution */
#define S_TZ_T         2019     /* time zone of the line */
#define S_JITTER_T     2020     /* jitter of the line */

#endif                          /* __SAVE_H__ */
