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


/* This code was originally inspired by Anacron's sources of
   Itai Tzur <itzur@actcom.co.il> */


#include "fcron.h"

#include "log.h"

#include <sys/types.h>
#include <sys/socket.h>

#ifdef DEBUG
char debug_opt = 1;             /* set to 1 if we are in debug mode */
#else
char debug_opt = 0;             /* set to 1 if we are in debug mode */
#endif
int dosyslog = 1;
char *logfile_path = NULL;


char *make_msg(const char *append, char *fmt, va_list args);
void log_syslog_str(int priority, char *msg);
void log_file_str(int priority, char *msg);
void log_console_str(int priority, char *msg);
void log_fd_str(int fd, char *msg);
static void print_line_prefix(FILE * logfile, int priority);
static void xlog(int priority, int fd, char *fmt, va_list args);
static void xlog_e(int priority, int fd, char *fmt, va_list args);
#ifdef HAVE_LIBPAM
static void log_pame(int priority, pam_handle_t * pamh, int pamerrno,
                     char *fmt, va_list args);
#endif

static char truncated[] = " (truncated)";
static int log_open = 0;
static FILE *logfile = NULL;


/* Initialise logging to either syslog or a file specified in fcron.conf,
 * or take no action if logging is suppressed.
 * This function will be called automatically if you attempt to log something,
 * however you may have to call it explicitely as it needs to run before the
 * program becomes a daemon so as it can print an error on the console
 * if it can't open the logs correctly. */
void
xopenlog(void)
{
    if (log_open)
        return;

    /* we MUST set log_open to 1 before doing anything else. That way,
     * if we call a function that logs something, which calls xopenlog,
     * then we won't end up in a nasty loop */
    log_open = 1;

    // are we logging to a file or using syslog or not logging at all?
    if (dosyslog) {
        openlog(prog_name, LOG_PID, SYSLOG_FACILITY);
    }

    if (logfile_path != NULL) {
        logfile = fopen(logfile_path, "a");
        if (logfile == NULL) {
            int saved_errno = errno;

            if (dosyslog) {
                /* we have already called openlog() which cannot fail */
                syslog(COMPLAIN_LEVEL, "Could not fopen log file '%s': %s",
                       logfile_path, strerror(saved_errno));
            }

            print_line_prefix(stderr, COMPLAIN_LEVEL);
            fprintf(stderr, "Could not fopen log file '%s': %s\n", logfile_path,
                    strerror(saved_errno));
        }
    }

}


void
xcloselog()
{
    if (!log_open)
        return;

    // check whether we need to close syslog, or a file.
    if (logfile != NULL) {
        /* we must NOT use xfclose_check() in log.c to avoid infinite loops */
        if (xfclose(&logfile) != 0) {
            int saved_errno = errno;

            syslog(COMPLAIN_LEVEL, "Error while closing log file '%s': %s",
                   logfile_path, strerror(saved_errno));

            if (foreground == 1) {
                print_line_prefix(stderr, COMPLAIN_LEVEL);
                fprintf(stderr, "Error while closing log file '%s': %s\n",
                        logfile_path, strerror(saved_errno));
            }
        }
    }

    if (dosyslog) {
        closelog();
    }

    log_open = 0;
}


/* Construct the message string from its parts, and append a string to it */
char *
make_msg(const char *append, char *fmt, va_list args)
{
    int len;
    char *msg = NULL;

    if ((msg = calloc(1, MAX_MSG + 1)) == NULL)
        return NULL;
    /* There's some confusion in the documentation about what vsnprintf
     * returns when the buffer overflows.  Hmmm... */
    len = vsnprintf(msg, MAX_MSG + 1, fmt, args);
    if (append != NULL) {
        size_t size_to_cat = ((MAX_MSG - len) > 0) ? (MAX_MSG - len) : 0;
        strncat(msg, ": ", size_to_cat);
        strncat(msg, append, size_to_cat);
        len += 2 + strlen(append);
    }
    if (len >= MAX_MSG)
        strcpy(msg + (MAX_MSG - 1) - sizeof(truncated), truncated);

    return msg;
}


/* log a simple string to syslog if needed */
void
log_syslog_str(int priority, char *msg)
{
    if (dosyslog) {
        xopenlog();
        syslog(priority, "%s", msg);
    }
}

/* log a simple string to a log file if needed */
void
log_file_str(int priority, char *msg)
{
    xopenlog();

    /* we may have failed to open the logfile - check if
     * it does exist *after* xopenlog() */
    if (logfile != NULL) {
        print_line_prefix(logfile, priority);
        fprintf(logfile, "%s\n", msg);
        fflush(logfile);
    }

}

/* log a simple string to console if needed */
void
log_console_str(int priority, char *msg)
{
    if (foreground == 1) {
        print_line_prefix(stderr, priority);
        fprintf(stderr, "%s\n", msg);
    }
}

/* log a simple string to fd if needed */
void
log_fd_str(int fd, char *msg)
{
    if (fd >= 0) {
        send(fd, msg, strlen(msg), 0);
        send(fd, "\n", strlen("\n"), 0);
    }
}

/* Log a message, described by "fmt" and "args", with the specified 
 * "priority". */
/* write it also to fd if positive, and to stderr if foreground==1 */
static void
xlog(int priority, int fd, char *fmt, va_list args)
{
    char *msg;

    if ((msg = make_msg(NULL, fmt, args)) == NULL)
        return;

    log_syslog_str(priority, msg);
    log_file_str(priority, msg);
    log_console_str(priority, msg);
    log_fd_str(fd, msg);

    Free_safe(msg);
}

/* Same as xlog(), but also appends an error description corresponding
 * to "errno". */
static void
xlog_e(int priority, int fd, char *fmt, va_list args)
{
    int saved_errno;
    char *msg;

    saved_errno = errno;

    if ((msg = make_msg(strerror(saved_errno), fmt, args)) == NULL)
        return;

    log_syslog_str(priority, msg);
    log_file_str(priority, msg);
    log_console_str(priority, msg);
    log_fd_str(fd, msg);

    Free_safe(msg);
}

/* write a message to the file specified by logfile. */
static void
print_line_prefix(FILE * logfile, int priority)
{
    time_t t = time(NULL);
    struct tm *ft;
    char date[30];
    char *type;

    // print the current time as a string.
    ft = localtime(&t);
    date[0] = '\0';
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", ft);

    // is it an info/warning/error/debug message?
    switch (priority) {
    case EXPLAIN_LEVEL:
        type = " INFO";
        break;
    case WARNING_LEVEL:
        type = " WARN";
        break;
    case COMPLAIN_LEVEL:
        type = "ERROR";
        break;
    case DEBUG_LEVEL:
        type = "DEBUG";
        break;
    default:
        type = "UNKNOWN_SEVERITY";
    }

    // print the log message.
    fprintf(logfile, "%s %s ", date, type);
}


#ifdef HAVE_LIBPAM
/* Same as log_syslog(), but also appends an error description corresponding
 * to the pam_error. */
static void
log_pame(int priority, pam_handle_t * pamh, int pamerrno, char *fmt,
         va_list args)
{
    char *msg;

    if ((msg = make_msg(pam_strerror(pamh, pamerrno), fmt, args)) == NULL)
        return;

    log_syslog_str(priority, msg);
    log_console_str(priority, msg);

    xcloselog();

    Free_safe(msg);
}
#endif


/* Log an "explain" level message */
void
explain(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog(EXPLAIN_LEVEL, -1, fmt, args);
    va_end(args);
}

/* as explain(), but also write message to fd if positive */
void
explain_fd(int fd, char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog(EXPLAIN_LEVEL, fd, fmt, args);
    va_end(args);
}


/* Log an "explain" level message, with an error description */
void
explain_e(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog_e(EXPLAIN_LEVEL, -1, fmt, args);
    va_end(args);
}


/* Log a "warning" level message */
void
warn(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog(WARNING_LEVEL, -1, fmt, args);
    va_end(args);
}

/* as warn(), but also write message to fd if positive */
void
warn_fd(int fd, char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog(WARNING_LEVEL, fd, fmt, args);
    va_end(args);
}


/* Log a "warning" level message, with an error description */
void
warn_e(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog_e(WARNING_LEVEL, -1, fmt, args);
    va_end(args);
}


/* Log a "complain" level message */
void
error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog(COMPLAIN_LEVEL, -1, fmt, args);
    va_end(args);
}

/* as error(), but also write message to fd if positive */
void
error_fd(int fd, char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog(COMPLAIN_LEVEL, fd, fmt, args);
    va_end(args);
}


/* Log a "complain" level message, with an error description */
void
error_e(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog_e(COMPLAIN_LEVEL, -1, fmt, args);
    va_end(args);
}


#ifdef HAVE_LIBPAM
/* Log a "complain" level message, with a PAM error description */
void
error_pame(pam_handle_t * pamh, int pamerrno, char *fmt, ...)
{
    va_list args;

    xcloselog();                /* PAM is likely to have used openlog() */

    va_start(args, fmt);
    log_pame(COMPLAIN_LEVEL, pamh, pamerrno, fmt, args);
    va_end(args);
}
#endif

/* Log a "complain" level message, and exit */
void
die(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog(COMPLAIN_LEVEL, -1, fmt, args);
    va_end(args);
    if (getpid() == daemon_pid) {
        error("Aborted");
    }
    else {
        error("fcron child aborted: this does not affect the main fcron daemon,"
              " but this may prevent a job from being run or an email from being sent.");
    }

    exit(EXIT_ERR);

}


/* Log a "complain" level message, with an error description, and exit */
void
die_e(char *fmt, ...)
{
    va_list args;
    int err_no = 0;

    err_no = errno;

    va_start(args, fmt);
    xlog_e(COMPLAIN_LEVEL, -1, fmt, args);
    va_end(args);
    if (getpid() == daemon_pid) {
        error("Aborted");
    }
    else {
        error("fcron child aborted: this does not affect the main fcron daemon,"
              " but this may prevent a job from being run or an email from being sent.");
    }


    exit(err_no);

}

#ifdef HAVE_LIBPAM
/* Log a "complain" level message, with a PAM error description, and exit */
void
die_pame(pam_handle_t * pamh, int pamerrno, char *fmt, ...)
{
    va_list args;

    xcloselog();                /* PAM is likely to have used openlog() */

    va_start(args, fmt);
    log_pame(COMPLAIN_LEVEL, pamh, pamerrno, fmt, args);
    va_end(args);
    pam_end(pamh, pamerrno);
    if (getpid() == daemon_pid)
        error("Aborted");

    exit(EXIT_ERR);

}
#endif

/* Log a "debug" level message */
void
Debug(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    xlog(DEBUG_LEVEL, -1, fmt, args);
    va_end(args);
}

/* write message to fd, and to syslog in "debug" level message if debug_opt */
void
send_msg_fd_debug(int fd, char *fmt, ...)
{
    char *msg;

    va_list args;

    va_start(args, fmt);

    if ((msg = make_msg(NULL, fmt, args)) == NULL)
        return;

    if (debug_opt)
        log_syslog_str(DEBUG_LEVEL, msg);

    log_fd_str(fd, msg);

    Free_safe(msg);

    va_end(args);
}

/* write message to fd */
void
send_msg_fd(int fd, char *fmt, ...)
{
    char *msg;

    va_list args;

    va_start(args, fmt);

    if ((msg = make_msg(NULL, fmt, args)) == NULL)
        return;

    log_fd_str(fd, msg);

    Free_safe(msg);

    va_end(args);
}
