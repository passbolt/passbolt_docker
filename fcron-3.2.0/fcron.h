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


#ifndef __FCRON_H__
#define __FCRON_H__

#include "global.h"
#include "mem.h"
#include "exe_list.h"
#include "lavg_list.h"
#include "fcronconf.h"

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#elif HAVE_SYS_DIRENT_H
#include <sys/dirent.h>
#elif HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <grp.h>

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/* global variables */
extern time_t now;
extern char foreground;
extern char default_mail_charset[TERM_LEN];
extern time_t first_sleep;
extern char *cdir;
extern pid_t daemon_pid;
extern char *orig_tz_envvar;
extern mode_t saved_umask;
extern char *prog_name;
extern uid_t rootuid;
extern gid_t rootgid;
extern char sig_hup;
extern struct cf_t *file_base;
extern struct job_t *queue_base;
extern unsigned long int next_id;
extern struct cl_t **serial_array;
extern short int serial_array_size;
extern short int serial_array_index;
extern short int serial_num;
extern short int serial_running;
extern short int serial_max_running;
extern short int serial_queue_max;
extern short int lavg_queue_max;
extern exe_list_t *exe_list;
extern lavg_list_t *lavg_list;
extern short int lavg_serial_running;
/* end of global variables */


/* functions prototypes */

/* fcron.c */
extern void xexit(int exit_value);

#endif                          /* __FCRON_H */
