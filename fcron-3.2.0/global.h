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
   WARNING : this file should not be modified.
   Compilation's options are in config.h
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

/* config.h must be included before every other includes 
 * (contains the compilation options) */
#include "config.h"


#include <ctype.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef WITH_SELINUX
#include <selinux.h>
#include <get_context_list.h>
#include <selinux/flask.h>
#include <selinux/av_permissions.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <locale.h>
#include <nl_types.h>
#include <langinfo.h>
#include <pwd.h>
#include <signal.h>

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#include <time.h>
#elif HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#elif HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif

#ifdef HAVE_CRED_H
#include <cred.h>
#endif
#ifdef HAVE_UCRED_H
#include <ucred.h>
#endif
#ifdef HAVE_SYS_CRED_H
#include <sys/cred.h>
#endif
#ifdef HAVE_SYS_UCRED_H
#include <sys/ucred.h>
#endif

#ifdef WITH_AUDIT
#include <libaudit.h>
#endif

#ifdef HAVE_LIBPAM
#include "pam.h"
#endif

#include "bitstring.h"          /* bit arrays */
#include "option.h"             /* manage fcrontab's options */
#include "env_list.h"           /* manage fcrontab's environment variable lists */
#include "cl.h"                 /* Cron Line cl_t type and associated functions */

/* you should not change this (nor need to do it) */
#define ERR     -1
#define OK       0

/* options for local functions */
#define STD 0

/* Approximate max value of time_t which localtime() allows: on a 64bits system,
 * this is less than LONG_MAX (64bits) as this is limited by struct tm's tm_year
 * which is a (not long) int (32bits).
 * As a time_t of INT_MAX=2^31 is 'only' in year 2038, we try to use a larger value
 * if we can. */
// FIXME: test on 32bit system
/* 2^33 = 8589934592, so LONG is 64bits at least */
#if (LONG_MAX > INT_MAX) && (LONG_MAX > 8589934592)
/* defined as time_t of 1st Jan of year (SHRT_MAX-1900) at 00:00:00 */
#define TIME_T_MAX 971859427200
#else
/* struct tm's tm_year is of type int, and tm_year will always be smaller than
 * the equivalent time_t, so INT_MAX is always a safe max value for time_t. */
#define TIME_T_MAX INT_MAX
#endif

/* macros */
#ifndef HAVE_SETEUID
#define seteuid(arg) setresuid(-1,(arg),-1)
#endif

#ifndef HAVE_SETEGID
#define setegid(arg) setresgid(-1,(arg),-1)
#endif

#define Skip_blanks(PTR) \
        while((*(PTR) == ' ') || (*(PTR) == '\t')) \
	    (PTR)++;

#define Overwrite(x) \
        do {                     \
          register char *__xx__; \
          if ((__xx__=(x)))      \
            while (*__xx__)      \
              *__xx__++ = '\0';  \
        } while (0)

#define debug if(debug_opt) Debug

typedef struct cf_t {
    struct cf_t *cf_next;
    struct cl_t *cf_line_base;
    char *cf_user;              /* user-name                                 */
    env_list_t *cf_env_list;    /* list of all parsed env var                */
    int cf_running;             /* number of jobs running                    */
    signed char cf_tzdiff;      /* time diff between system and local hour   */
#ifdef WITH_SELINUX
    security_context_t cf_user_context;
    security_context_t cf_file_context;
#endif
} cf_t;


typedef struct job_t {
    struct cl_t *j_line;
    struct job_t *j_next;
} job_t;


#if SIZEOF_TIME_T == SIZEOF_SHORT_INT
#define ATTR_SIZE_TIMET "h"
#define CAST_TIMET_PTR (short int *)
#elif SIZEOF_TIME_T == SIZEOF_INT
#define ATTR_SIZE_TIMET ""
#define CAST_TIMET_PTR (int *)
#elif SIZEOF_TIME_T == SIZEOF_LONG_INT
#define ATTR_SIZE_TIMET "l"
#define CAST_TIMET_PTR (long int *)
#elif SIZEOF_TIME_T == SIZEOF_LONG_LONG_INT
#define ATTR_SIZE_TIMET "ll"
#define CAST_TIMET_PTR (long long int *)
#else
#error "SIZEOF_TIME_T does not correspond with a known format."
#endif

#if SIZEOF_PID_T == SIZEOF_SHORT_INT
#define ATTR_SIZE_PIDT "h"
#define CAST_PIDT_PTR (short int *)
#elif SIZEOF_PID_T == SIZEOF_INT
#define ATTR_SIZE_PIDT ""
#define CAST_PIDT_PTR (int *)
#elif SIZEOF_PID_T == SIZEOF_LONG_INT
#define ATTR_SIZE_PIDT "l"
#define CAST_PIDT_PTR (long int *)
#elif SIZEOF_PID_T == SIZEOF_LONG_LONG_INT
#define ATTR_SIZE_PIDT "ll"
#define CAST_PIDT_PTR (long long int *)
#else
#error "SIZEOF_PID_T does not correspond with a known format."
#endif


/* local header files : we include here the headers which may use some types defined
 *                      above. */

/* constants for fcrontabs needed to load and save the fcrontabs to disk */
#include "save.h"
/* log part */
#include "log.h"
/* functions used by fcrontab, fcrondyn, and fcron */
#include "subs.h"
/* file related helper functions */
#include "filesubs.h"

#endif                          /* __GLOBAL_H__ */
