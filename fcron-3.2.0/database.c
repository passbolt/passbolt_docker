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


#include "fcron.h"

#include "database.h"
#include "job.h"
#include "getloadavg.h"

int is_leap_year(int year);
int get_nb_mdays(int year, int mon);
void set_wday(struct tm *date);
time_t mktime_no_dst(struct tm *t);
void goto_beginning_next_period_periodical(cl_t * line, struct tm *ftime);
void move_time_to(int where, cl_t * line, struct tm *ftime);
#define BEGIN_NEXT_PERIOD 11    /* move_time_to()'s where possible value */
#define END_OF_INTERVAL 12      /* move_time_to()'s where possible value */
void run_serial_job(void);
void run_lavg_job(lavg_t * l);
void run_queue_job(cl_t * line);

void
test_jobs(void)
  /* determine which jobs need to be run, and run them. */
{
    struct job_t *j;

/*      // */
    debug("Looking for jobs to execute ...");
/*      // */

    while ((j = queue_base) && j->j_line->cl_nextexe <= now) {

        if (j->j_line->cl_remain > 0 && --(j->j_line->cl_remain) > 0) {
            debug("    cl_remain: %d", j->j_line->cl_remain);
        }
        else {

            j->j_line->cl_remain = j->j_line->cl_runfreq;

            if (is_lavg(j->j_line->cl_option))
                add_lavg_job(j->j_line, -1);
            else if (is_serial(j->j_line->cl_option))
                add_serial_job(j->j_line, -1);
            else
                run_normal_job(j->j_line, -1);

            set_hasrun(j->j_line->cl_option);
        }

        if (is_runonce(j->j_line->cl_option) && is_hasrun(j->j_line->cl_option)) {
            explain("Line '%s' has runonce set: not re-scheduling it.",
                    j->j_line->cl_shell);
            job_queue_remove(j->j_line);
        }
        else {
            set_next_exe(j->j_line, STD, -1);
        }
    }

}


int
switch_timezone(const char *orig_tz, const char *dest_tz)
/* check if we have already switched to dest_tz timezone, otherwise do it */
/* If dest_tz is NULL, this function does nothing */
/* Returns 1 if this function has switched the timezone, 0 otherwise */
{
    char *current_tz = getenv("TZ");

    if (dest_tz != NULL &&
        (current_tz == NULL || strcmp(dest_tz, current_tz) != 0)) {
        my_setenv_overwrite("TZ", dest_tz);
        return 1;
    }
    else
        return 0;
}

void
switch_back_timezone(const char *orig_tz)
/* if orig_tz is NULL, unsets TZ
 * otherwise, sets TZ to orig_tz */
{
    if (orig_tz == NULL) {
        my_unsetenv("TZ");
    }
    else {
        my_setenv_overwrite("TZ", orig_tz);
    }
}


time_t
mktime_no_dst(struct tm *t)
/* same as mktime(), but without daylight saving time (dst) change adjustment
 * (ie. the returned time_t does not depend on the tm_isdst field) */
/* Remark : you may think that instead of creating a new function,
 *          it would be easier to set the tm_isdst field to -1.
 *          Unfortunately, the behaviour of mktime() with 
 *          tm_isdst set to -1 depends on the unix you run.
 *          In other word, it wouldn't be portable. */
/*
 * WARNING : the content of t has to be valid (for instance, 0<=t->tm_hour<=23,
 *           etc)
 */
{
    struct tm t2;
    time_t ti1;

    t2 = *t;

    ti1 = mktime(&t2);
    /* */
    debug("after  mktime() : %d:%d isdst:%d ti:%ld\n",
          t2.tm_hour, t2.tm_min, t2.tm_isdst, ti1);
    /* */

    /* check if there have been a dst change adjustment */
    if (t->tm_isdst != t2.tm_isdst) {

        time_t ti2;
        struct tm t3;

        /* recompute the time_t field with the other isdst value
         * it works well, unless in a special case, hence the test
         * below */
        t3 = *t;
        t3.tm_isdst = t2.tm_isdst;
        ti2 = mktime(&t3);
        /* */
        debug("after dst fix 1 : %d:%d isdst:%d ti:%ld\n",
              t3.tm_hour, t3.tm_min, t3.tm_isdst, ti2);
        /* */

        /* if t1 is in the "gap" of a dst change (for instance,
         * if t1 is 2:30 while at 2:00, it is 3:00 due to the dst change,
         * ie. 2:30 is never reached), the ti2 may be incorrect :
         * we check that it is correct before using it : */
        if (t3.tm_hour == t->tm_hour || ti1 < ti2) {
            t2 = t3;
            ti1 = ti2;
        }

    }

    *t = t2;
    return ti1;
}


void
run_normal_job(cl_t * line, int info_fd)
/* run a job, and write "log" on info_fd if positive */
{

    if (line->cl_numexe <= 0 ||
        (is_exe_sev(line->cl_option) && line->cl_numexe < UCHAR_MAX)) {
        line->cl_numexe += 1;
        run_queue_job(line);
        send_msg_fd(info_fd, "Job '%s' started.", line->cl_shell);
    }
    else {
        warn_fd(info_fd, "    process already running: %s's '%s'",
                line->cl_file->cf_user, line->cl_shell);
    }

}

void
run_lavg_job(lavg_t * l)
{

    run_queue_job(l->l_line);

    if (is_serial(l->l_line->cl_option))
        lavg_serial_running++;

}


void
run_serial_job(void)
    /* run the next serialized job */
{
/*      // */
/*      debug("running next serial job"); */
/*      //     */

    debug("num: %d running:%d  index:%d", serial_num, serial_running,
          serial_array_index);
    if (serial_num != 0) {
        run_queue_job(serial_array[serial_array_index]);
        serial_array[serial_array_index] = NULL;

        serial_running++;
        if (++serial_array_index >= serial_array_size)
            serial_array_index -= serial_array_size;
        serial_num--;

    }
}


void
run_queue_job(cl_t * line)
    /* run a job */
{

    exe_t e = { NULL, 0, 0 };

/*      // */
/*      debug("run_queue_job"); */
/*      // */

    e.e_line = line;

    /* run the job */
    if (run_job(&e) == OK) {
        /* append job to the list of executed job */
        exe_list_add(exe_list, &e);
        line->cl_file->cf_running += 1;
    }

}

job_t *
job_queue_remove(cl_t * line)
    /* remove a job from the queue list
     * returns a pointer to the previous entry,
     * or NULL if the line either wasn't in the queue or was the first entry */
{
    struct job_t *j;
    struct job_t *jprev = NULL;

    if (queue_base == NULL)
        return NULL;

    /* find the job in the list */
    for (j = queue_base; j != NULL; jprev = j, j = j->j_next) {
        if (j->j_line == line) {
            /* remove it from the list */
            if (jprev != NULL) {
                jprev->j_next = j->j_next;
            }
            else
                /* first element of the list */
                queue_base = j->j_next;

            Free_safe(j);
            return jprev;
        }
    }

    /* the job wasn't there */
    return NULL;
}

void
insert_nextexe(cl_t * line)
    /* insert a job at the right position in the job queue */
{
    struct job_t *newjob = NULL;
    struct job_t *j = NULL;
    struct job_t *jprev = NULL;

    Alloc(newjob, job_t);
    newjob->j_line = line;
    newjob->j_next = NULL;

    if (queue_base == NULL) {
        /* no job in queue */
        queue_base = newjob;
        return;
    }

    jprev = job_queue_remove(line);
    j = (jprev) ? jprev : queue_base;

    /* check if we should start from queue_base or from jprev
     * (in some cases, e.g. fcrontab has just been edited, the line should
     *  be moved *forward* in the queue) */
    if (jprev == NULL || line->cl_nextexe < jprev->j_line->cl_nextexe) {
        j = queue_base;
    }

    /* a job can only be moved back */
    while (j != NULL && (line->cl_nextexe >= j->j_line->cl_nextexe)) {
        jprev = j;
        j = j->j_next;
    }
    /* when we get out from the while(), newjob should be added between jprev and j */

    newjob->j_next = j;

    if (jprev == NULL)
        queue_base = newjob;
    else
        jprev->j_next = newjob;

}

void
add_serial_job(cl_t * line, int info_fd)
    /* add the next queued job in serial queue */
{
    short int i;

    /* check if the line is already in the serial queue 
     * (we consider serial jobs currently running as in the queue) */
    if ((is_serial_sev(line->cl_option) && line->cl_numexe >= UCHAR_MAX) ||
        (!is_serial_sev(line->cl_option) && line->cl_numexe > 0)) {
        send_msg_fd_debug(info_fd, "already in serial queue '%s'",
                          line->cl_shell);
        return;
    }

    send_msg_fd_debug(info_fd, "inserting in serial queue '%s'",
                      line->cl_shell);

    if (serial_num >= serial_array_size) {
        if (serial_num >= serial_queue_max) {
            error_fd(info_fd, "Could not add job : serial queue is full "
                     "(%d jobs). Consider using option serialonce, fcron's "
                     "option -m and/or -q : '%s'", serial_queue_max,
                     line->cl_shell);
            if (is_notice_notrun(line->cl_option))
                mail_notrun(line, QUEUE_FULL, NULL);
            return;
        }
        else {
            cl_t **ptr = NULL;
            short int old_size = serial_array_size;

            debug("Resizing serial_array");
            serial_array_size = (serial_array_size + SERIAL_GROW_SIZE);

            ptr =
                alloc_safe(serial_array_size * sizeof(cl_t *), "serial_array");

            /* copy lines in order to have the first line at the index 0 */
            memcpy(ptr + serial_array_index, serial_array,
                   (sizeof(cl_t *) * (old_size - serial_array_index)));
            memcpy(ptr, serial_array + (old_size - serial_array_index),
                   (sizeof(cl_t *) * serial_array_index));
            serial_array_index = 0;
            Free_safe(serial_array);
            serial_array = ptr;
        }
    }

    if ((i = serial_array_index + serial_num) >= serial_array_size)
        i -= serial_array_size;

    serial_array[i] = line;

    serial_num++;
    line->cl_numexe += 1;

    send_msg_fd_debug(info_fd, "serial num: %d size:%d index:%d curline:%d "
                      "running:%d (%s)", serial_num, serial_array_size,
                      serial_array_index, i, serial_running, line->cl_shell);


}


void
add_lavg_job(cl_t * line, int info_fd)
    /* add the next queued job in lavg queue */
    /* WARNING : must be run before a set_next_exe() to get the strict option
     * working correctly */
{
    lavg_t *lavg_entry = NULL;

    /* check if the line is already in the lavg queue
     * (we consider serial jobs currently running as in the queue) */
    if ((is_lavg_sev(line->cl_option) && line->cl_numexe >= UCHAR_MAX) ||
        (!is_lavg_sev(line->cl_option) && line->cl_numexe > 0)) {
        send_msg_fd_debug(info_fd, "already in lavg queue '%s'",
                          line->cl_shell);
        return;
    }
/*  	// */
    send_msg_fd_debug(info_fd, "inserting in lavg queue '%s'", line->cl_shell);
/*  	// */

    /* append job to the list of lavg job */
    lavg_entry = lavg_list_add_line(lavg_list, line);
    if (lavg_entry == NULL) {
        error_fd(info_fd,
                 "Could not add job '%s' : lavg queue is full (%d jobs)."
                 " Consider using options lavgonce, until, strict and/or "
                 "fcron's option -q.", line->cl_shell, lavg_list->max_entries);
        if (is_notice_notrun(line->cl_option))
            mail_notrun(line, QUEUE_FULL, NULL);
        return;
    }

    line->cl_numexe += 1;
    set_run_if_late(line->cl_option);
    if (is_strict(line->cl_option) && line->cl_runfreq == 1) {
        struct tm *ft;
        struct tm ftime;
        time_t begin_of_cur_int, end_of_cur_int = 0;
        int tz_changed = 0;

        /* Switch to another timezone if necessary. */
        /* If line should be scheduled in a different time zone
         * (ie. cl_tz != NULL),
         * switch to that timezone now, do the calculations,
         * and switch back to the local timezone at the end
         * of the function. */
        tz_changed = switch_timezone(orig_tz_envvar, line->cl_tz);

        /* handle timezone differences */
        begin_of_cur_int = line->cl_nextexe - (line->cl_file->cf_tzdiff * 3600);

        ft = localtime(&begin_of_cur_int);

        /* localtime() function seems to return every time the same pointer :
         * it resets our previous changes, so we need to prevent it
         * ( localtime() is used in the debug() function) */
        memcpy(&ftime, ft, sizeof(struct tm));

        move_time_to(END_OF_INTERVAL, line, &ftime);

        end_of_cur_int =
            mktime_no_dst(&ftime) + (line->cl_file->cf_tzdiff * 3600);

        if ((line->cl_until > 0) && (line->cl_until + now < end_of_cur_int))
            lavg_entry->l_until = line->cl_until + now;
        else {
            lavg_entry->l_until = end_of_cur_int;
            clear_run_if_late(line->cl_option);
        }

        if (tz_changed > 0)
            switch_back_timezone(orig_tz_envvar);
    }
    else
        lavg_entry->l_until = (line->cl_until > 0) ? now + line->cl_until : 0;

}


void
wait_chld(void)
  /* wait_chld() - check for job completion */
{
    int pid;
    cl_t *line = NULL;
    exe_t *e = NULL;

/*      // */
/*      debug("wait_chld"); */
/*      // */

    while ((pid = wait3(NULL, WNOHANG, NULL)) > 0) {

        for (e = exe_list_first(exe_list); e != NULL;
             e = exe_list_next(exe_list)) {

            if (pid == e->e_ctrl_pid) {
                if (e->e_line == NULL) {
                    /* the corresponding file has been removed from memory */
                    debug("job finished: pid %d", pid);
                }
                else {

                    line = e->e_line;
/*  		    debug("job finished: %s", line->cl_shell); */
                    line->cl_numexe -= 1;
                    line->cl_file->cf_running -= 1;

                    if (is_serial_once(line->cl_option)) {
                        clear_serial_once(line->cl_option);
                        if (--serial_running < serial_max_running)
                            run_serial_job();
                    }
                    else if (is_serial(line->cl_option)
                             && !is_lavg(line->cl_option)) {
                        if (--serial_running < serial_max_running)
                            run_serial_job();
                    }
                    else if (is_lavg(line->cl_option)
                             && is_serial(line->cl_option))
                        lavg_serial_running--;
                }

                exe_list_remove_cur(exe_list);
                exe_list_end_iteration(exe_list);
                break;
            }
        }

    }

}


void
wait_all(int *counter)
   /* return after all jobs completion. */
{
    int pid;
    exe_t *e = NULL;

    debug("Waiting for all jobs");

    while ((*counter > 0) && (pid = wait3(NULL, 0, NULL)) > 0) {
        for (e = exe_list_first(exe_list); e != NULL;
             e = exe_list_next(exe_list)) {
            if (pid == e->e_ctrl_pid) {
                if (e->e_line == NULL) {
                    /* the corresponding file has been removed from memory */
                    debug("job finished: pid %d", pid);
                }
                else {

                    debug("job finished: '%s'", e->e_line->cl_shell);
                    e->e_line->cl_numexe -= 1;
                    e->e_line->cl_file->cf_running -= 1;

                    if (is_serial_once(e->e_line->cl_option))
                        clear_serial_once(e->e_line->cl_option);

                }

                exe_list_remove_cur(exe_list);
                exe_list_end_iteration(exe_list);
                break;
            }
        }
    }

}


int
is_leap_year(int year)
  /* return 1 if it's a leap year otherwise return 0 */
{
    return ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)));

}


int
get_nb_mdays(int year, int mon)
  /* return the number of days in a given month of a given year */
{
    if (mon == 1) {             /* is February ? */
        if (is_leap_year(year))
            return 29;
        else
            return 28;
    }
    else if (mon <= 6)
        if (mon % 2 == 0)
            return 31;
        else
            return 30;
    else if (mon % 2 == 0)
        return 30;
    else
        return 31;

}


void
set_wday(struct tm *date)
  /* we know that 01/01/2000 was a Saturday ( day number 6 )
   * so we count the number of days since 01/01/2000,
   * and add this number modulo 7 to the wday number */
{
    long nod = 0;
    int i;

    /* we add the number of days of each previous years */
    for (i = (date->tm_year - 1); i >= 100; i--)
        nod += (is_leap_year(i + 1900)) ? 366 : 365;

    /*     and month */
    for (i = (date->tm_mon - 1); i >= 0; i--)
        nod += get_nb_mdays((date->tm_year + 1900), i);

    /* then we add the number of days passed in the current month */
    nod += (date->tm_mday - 1); /* (mday is set from 1 to 31) */

    date->tm_wday = (nod % 7) + 6;

    if (date->tm_wday >= 7)
        date->tm_wday -= 7;

    debug("   dow of %04d-%02d-%02d : %d", (date->tm_year + 1900),
          (date->tm_mon + 1), date->tm_mday, date->tm_wday);

}


void
goto_beginning_next_period_periodical(cl_t * line, struct tm *ftime)
    /* From ftime, search the first/nearest time and date of the line's next
     * period of execution.
     *
     * Line must be periodical (i.e. is_freq_periodically(line->cl_option) == TRUE)
     *
     * ftime will contain this time and date when this function returns.
     *
     * Vocabulary:
     * interval of execution= a continuous interval of time during which
     *     the line can be executed.
     * period of execution= a continuous interval of time during which
     *     the line is to be executed once and only once. */
{
    int max = 0;

    /* sanity check */
    if (!is_freq_periodically(line->cl_option))
        die("goto_beginning_next_period() called with a non periodical line");


    /* number of days in ftime's month */
    max = get_nb_mdays(ftime->tm_year, ftime->tm_mon);

    /* STEP 1: find the beginning of the next period without ensuring
     * there is no overflow (min>=60, hour>=24, etc) */

    if (is_freq_mid(line->cl_option)) {

        if (is_freq_mins(line->cl_option))
            /* nothing to do : return */
            return;
        else if (is_freq_hrs(line->cl_option)) {
            if (ftime->tm_min >= 30)
                ftime->tm_hour++;
            ftime->tm_min = 30;
        }
        else {
            ftime->tm_min = 0;
            if (is_freq_days(line->cl_option)) {
                if (ftime->tm_hour >= 12)
                    ftime->tm_mday++;
                ftime->tm_hour = 12;
            }
            else {
                ftime->tm_hour = 0;
                if (is_freq_dow(line->cl_option)) {
                    int to_add = (ftime->tm_wday >= 4) ? 11 - ftime->tm_wday :
                        4 - ftime->tm_wday;
                    if (ftime->tm_mday + to_add > max) {
                        ftime->tm_mon++;
                        ftime->tm_mday = ftime->tm_mday + to_add - max;
                    }
                    else
                        ftime->tm_mday += to_add;
                }
                else {
                    if (is_freq_mons(line->cl_option)) {
                        if (ftime->tm_mday >= 15)
                            ftime->tm_mon++;
                        ftime->tm_mday = 15;
                    }
                    else {
                        /* weird : we have the bit freq_mid set, but
                         * none of freq_{mins|hour|days|dow|mons} is set :
                         * we do nothing but increase tm_min by 1
                         * so as we don't return a time in the past */
                        ftime->tm_min++;
                        warn("Line %s doesn't seem correct: consider "
                             "reinstalling the corresponding fcrontab");
                    }
                }
            }
        }

    }

    else {                      /* is_freq_mid(line->cl_option) */

        if (is_freq_mins(line->cl_option))
            /* nothing to do */
            return;
        else {
            ftime->tm_min = 0;
            if (is_freq_hrs(line->cl_option))
                ftime->tm_hour++;
            else {
                ftime->tm_hour = 0;
                if (is_freq_days(line->cl_option))
                    ftime->tm_mday++;
                else {
                    if (is_freq_dow(line->cl_option)) {
                        int to_add =
                            (ftime->tm_wday == 0) ? 1 : 8 - ftime->tm_wday;
                        if (ftime->tm_mday + to_add > max) {
                            ftime->tm_mday = ftime->tm_mday + to_add - max;
                            ftime->tm_mon++;
                        }
                        else
                            ftime->tm_mday += to_add;
                    }
                    else {
                        ftime->tm_mday = 1;
                        if (is_freq_mons(line->cl_option))
                            ftime->tm_mon++;
                    }
                }
            }
        }

    }                           /* is_freq_mid(line->cl_option) */

    /* we set tm_sec to 0 here and not before to ensure we will never return
     * a time in the past (case is_freq_mins(line->cl_option)
     * where we do nothing) */
    ftime->tm_sec = 0;

    /* STEP 2: fix the overflows.
     * (a value may exceed the max value of a field: fix it if necessary) */

    if (ftime->tm_min >= 60) {
        ftime->tm_min = 0;
        ftime->tm_hour++;
    }
    if (ftime->tm_hour >= 24) {
        ftime->tm_hour = 0;
        ftime->tm_mday++;
    }
    /* the month field may have changed */
    max = get_nb_mdays((ftime->tm_year + 1900), ftime->tm_mon);
    if (ftime->tm_mday > max) {
        ftime->tm_mday = 1;
        ftime->tm_mon++;
    }
    if (ftime->tm_mon >= 12) {
        ftime->tm_mon = 0;
        ftime->tm_year++;
    }

    if (debug_opt)
        set_wday(ftime);
    debug("   '%s' beginning of next period %04d-%02d-%02d wday:%d %02d:%02d "
          "(tzdiff=%d, timezone=%s)", line->cl_shell, (ftime->tm_year + 1900),
          (ftime->tm_mon + 1), ftime->tm_mday, ftime->tm_wday,
          ftime->tm_hour, ftime->tm_min, line->cl_file->cf_tzdiff,
          (line->cl_tz != NULL) ? line->cl_tz : "localtime");

}


void
move_time_to(int where, cl_t * line, struct tm *ftime)
    /* IF WHERE == BEGIN_NEXT_PERIOD: from ftime, search the first/nearest time and date
     * of the line's next period of execution.
     * IF WHERE == END_OF_INTERVAL: search the last time and date
     * of the line's interval of execution containing ftime.
     *
     * ftime will contain this time and date when move_time_to returns.
     *
     * Vocabulary:
     * interval of execution= a continuous interval of time during which
     *     the line can be executed.
     * period of execution= a continuous interval of time during which
     *     the line is to be executed once and only once. */
{
    struct tm tm_next_period;
    time_t timet_ftime;
    /* by default we set timet_next_period to now + 10 years, which will
     * always be later than the end of the interval of execution
     * so as to make the test timet_ftime < timet_next_period always true */
    time_t timet_next_period = now + 10 * 365 * 24 * 3600;

    /* to prevent from infinite loop with unvalid lines : */
    short int year_limit = MAXYEAR_SCHEDULE_TIME;
    /* Depending on the situation we may need to ignore some fields 
     * while we walk through time */
    char ignore_mins, ignore_hrs, ignore_days, ignore_mons, ignore_dow;

    /* sanity checks */
    if (where != BEGIN_NEXT_PERIOD && where != END_OF_INTERVAL)
        die("move_time_to() called with invalid argument 'where': %d",
            (int)where);


    if (where == BEGIN_NEXT_PERIOD && is_freq_periodically(line->cl_option)) {
        goto_beginning_next_period_periodical(line, ftime);
        return;
    }

    /* In all other cases, we will have to walk through time */

    if (is_freq_periodically(line->cl_option)) {

        /* In this case we want to make sure we won't go after the end
         * of the period of execution, so we need to set next_period */

        memcpy(&tm_next_period, ftime, sizeof(tm_next_period));
        goto_beginning_next_period_periodical(line, &tm_next_period);
        timet_next_period = mktime_no_dst(&tm_next_period);

    }

    timet_ftime = mktime_no_dst(ftime);

    if (where == BEGIN_NEXT_PERIOD) {
        /* we have to ignore the fields containing single numbers */
        ignore_mins = (is_freq_mins(line->cl_option)) ? 1 : 0;
        ignore_hrs = (is_freq_hrs(line->cl_option)) ? 1 : 0;
        ignore_days = (is_freq_days(line->cl_option)) ? 1 : 0;
        ignore_mons = (is_freq_mons(line->cl_option)) ? 1 : 0;
        ignore_dow = (is_freq_dow(line->cl_option)) ? 1 : 0;
    }
    else {
        /* we want to go to the end of the current interval:
         * we don't ignore anything */
        ignore_mins = ignore_hrs = ignore_days = ignore_mons = ignore_dow = 0;
    }

    /* */
    debug("   ignore: %d %d %d %d %d", ignore_mins, ignore_hrs,
          ignore_days, ignore_mons, ignore_dow);
    /* */

    /* while we are in an interval of execution and not in the next period */
    while ((ignore_mins == 1 || bit_test(line->cl_mins, ftime->tm_min)) &&
           (ignore_hrs == 1 || bit_test(line->cl_hrs, ftime->tm_hour)) &&
           ((is_dayand(line->cl_option) &&
             (ignore_days == 1 || bit_test(line->cl_days, ftime->tm_mday)) &&
             (ignore_dow == 1 || bit_test(line->cl_dow, ftime->tm_wday)))
            ||
            (is_dayor(line->cl_option) &&
             (ignore_days == 1 || bit_test(line->cl_days, ftime->tm_mday) ||
              ignore_dow == 1 || bit_test(line->cl_dow, ftime->tm_wday)))
           ) && (ignore_mons == 1 || bit_test(line->cl_mons, ftime->tm_mon))
           && (timet_ftime < timet_next_period)
        ) {

        ftime->tm_sec = 0;
        if (ignore_mins)
            ftime->tm_min = 60;
        else {
            do
                ftime->tm_min++;
            while (bit_test(line->cl_mins, ftime->tm_min)
                   && (ftime->tm_min < 60));
        }
        if (ftime->tm_min >= 60) {
            ftime->tm_min = 0;
            if (ignore_hrs && ignore_mins)
                ftime->tm_hour = 24;
            else
                ftime->tm_hour++;
            if (ftime->tm_hour >= 24) {
                ftime->tm_hour = 0;
                if (ignore_days && ignore_hrs && ignore_mins && ignore_dow)
                    ftime->tm_mday = 32;        /* go to next month */
                else
                    ftime->tm_mday++;
                if (ftime->tm_mday >
                    get_nb_mdays((ftime->tm_year + 1900), ftime->tm_mon)) {
                    ftime->tm_mday = 1;
                    if (ignore_mons && ignore_days && ignore_dow
                        && ignore_hrs && ignore_mins)
                        ftime->tm_mon = 12;
                    else
                        ftime->tm_mon++;
                    if (ftime->tm_mon >= 12) {
                        ftime->tm_mon = 0;
                        ftime->tm_year++;
                        if (--year_limit <= 0) {
                            error("Can't found a non matching date for '%s' "
                                  "in the next %d years. Maybe this line "
                                  "is corrupted : consider reinstalling "
                                  "the fcrontab", line->cl_shell,
                                  MAXYEAR_SCHEDULE_TIME);
                            return;
                        }
                    }
                }
                set_wday(ftime);
            }
        }

        /* // */
        {
            /* set temporarily debug_opt to false to avoid having too many
             * messages in the logs */
            char debug_opt_previous = debug_opt;
            debug_opt = 0;

            timet_ftime = mktime_no_dst(ftime);

            debug_opt = debug_opt_previous;
        }
        /* // */

    }

    if (timet_ftime > timet_next_period) {
        /* the end of the interval if after the end of the period:
         * we don't want to go in the next period, so we return
         * the value of the end of the period. */
        memcpy(ftime, &tm_next_period, sizeof(tm_next_period));
    }

    if (where == END_OF_INTERVAL) {
        /* we want the end of the current interval, not the beginning
         * of the first non-matching interval : go back by one minute */
        if (--ftime->tm_min < 0) {
            ftime->tm_min = 59;
            if (--ftime->tm_hour < 0) {
                ftime->tm_hour = 23;
                if (--ftime->tm_mday < 1) {
                    if (--ftime->tm_mon < 0) {
                        ftime->tm_mon = 11;
                        ftime->tm_year--;
                    }
                    ftime->tm_mday =
                        get_nb_mdays((ftime->tm_year + 1900), ftime->tm_mon);
                }
            }
        }
    }

    debug
        ("   '%s' %s %04d-%02d-%02d wday:%d %02d:%02d (tzdiff=%d, timezone=%s)",
         line->cl_shell,
         (where ==
          END_OF_INTERVAL) ? "end of interval" : "begin of next period",
         (ftime->tm_year + 1900), (ftime->tm_mon + 1), ftime->tm_mday,
         ftime->tm_wday, ftime->tm_hour, ftime->tm_min,
         line->cl_file->cf_tzdiff,
         (line->cl_tz != NULL) ? line->cl_tz : "localtime");
}


void
set_next_exe(cl_t * line, char option, int info_fd)
  /* set the cl_nextexe of a given cl_t and insert it in the queue */
{

    time_t basetime;
    struct tm *ft;
    struct tm ftime;
    int tz_changed = 0;

    basetime = (option & FROM_CUR_NEXTEXE) ? line->cl_nextexe : now;

    /* Switch to another timezone if necessary. */
    /* If line should be scheduled in a different time zone
     * (ie. cl_tz != NULL),
     * switch to that timezone now, do the calculations,
     * and switch back to the local timezone at the end 
     * of the function. */
    tz_changed = switch_timezone(orig_tz_envvar, line->cl_tz);

    if (is_td(line->cl_option)) {

        time_t nextexe = 0;
        int i;
        int max;
        char has_changed = 0;
        /* to prevent from invinite loop with unvalid lines : */
        short int year_limit = MAXYEAR_SCHEDULE_TIME;
        /* timezone difference */
        time_t basetime_tz = basetime - (line->cl_file->cf_tzdiff * 3600);

        ft = localtime(&basetime_tz);

        /* localtime() function seem to return every time the same pointer :
         * it resets our previous changes, so we need to prevent it
         * ( localtime() is used in the debug() function) */
        memcpy(&ftime, ft, sizeof(struct tm));

        /* creates a bug on DST change on some systems ?? */
        /* ftime.tm_isdst = -1; */

        /* to prevent multiple execution of &-jobs in the same minute
         * (but not if the user has explicitely asked to run jobs immediately) */
        if (first_sleep > 0 || option == STD || line->cl_runfreq != 1) {
            ftime.tm_min += 1;
            ftime.tm_sec = 0;
        }

        if (line->cl_runfreq == 1 && option != NO_GOTO && option != NO_GOTO_LOG)
            /* %-line: go to next period */
            move_time_to(BEGIN_NEXT_PERIOD, line, &ftime);

 setMonth:
        for (i = ftime.tm_mon; (bit_test(line->cl_mons, i) == 0) && (i < 12);
             i++) ;
        if (i >= 12) {
            ftime.tm_year++;
            if (--year_limit <= 0) {
                error("Can't found a matching date for '%s' in the next %d"
                      " years. Maybe this line is corrupted : consider"
                      " reinstalling the fcrontab.",
                      line->cl_shell, MAXYEAR_SCHEDULE_TIME);
                goto set_cl_nextexe;
            }
            if (has_changed < 3) {
                has_changed = 3;
                ftime.tm_mon = 0;
                ftime.tm_mday = 1;
                ftime.tm_hour = 0;
                ftime.tm_min = 0;
            }
            else
                ftime.tm_mon = 0;
            goto setMonth;
        }
        if (ftime.tm_mon != i) {
            ftime.tm_mon = i;
            if (has_changed < 2) {
                has_changed = 2;
                ftime.tm_mday = 1;
                ftime.tm_hour = 0;
                ftime.tm_min = 0;
            }
        }

        /* set the number of days in that month */
        max = get_nb_mdays((ftime.tm_year + 1900), ftime.tm_mon);

 setDay:
        if (is_dayand(line->cl_option)) {
            for (i = ftime.tm_mday;
                 (bit_test(line->cl_days, i) == 0) && (i <= max); i++) ;
            if (i > max) {
                ftime.tm_mon++;
                if (has_changed < 2) {
                    has_changed = 2;
                    ftime.tm_mday = 1;
                    ftime.tm_hour = 0;
                    ftime.tm_min = 0;
                }
                else
                    ftime.tm_mday = 1;
                goto setMonth;
            }
            if (ftime.tm_mday != i) {
                ftime.tm_mday = i;
                if (has_changed < 1) {
                    has_changed = 1;
                    ftime.tm_hour = 0;
                    ftime.tm_min = 0;
                }
            }

            set_wday(&ftime);

            /* check if the day of week is OK */
            if (bit_test(line->cl_dow, ftime.tm_wday) == 0) {
                ftime.tm_mday++;
                ftime.tm_hour = 0;
                ftime.tm_min = 0;
                goto setDay;
            }
        }
        else {                  /* dayor */
            int j;

            set_wday(&ftime);

            j = ftime.tm_wday;
            i = ftime.tm_mday;
            while ((bit_test(line->cl_days, i) == 0) &&
                   (bit_test(line->cl_dow, j) == 0)) {
                if (i > max) {
                    ftime.tm_mon++;
                    if (has_changed < 2) {
                        has_changed = 2;
                        ftime.tm_mday = 1;
                        ftime.tm_hour = 0;
                        ftime.tm_min = 0;
                    }
                    else
                        ftime.tm_mday = 1;
                    goto setMonth;
                }
                if (j >= 7)
                    j -= 7;
                i++;
                j++;
            }
            if (ftime.tm_mday != i) {
                ftime.tm_mday = i;
                if (has_changed < 1) {
                    has_changed = 1;
                    ftime.tm_hour = 0;
                    ftime.tm_min = 0;
                }
            }
        }

 setHour:
        for (i = ftime.tm_hour; (bit_test(line->cl_hrs, i) == 0) && (i < 24);
             i++) ;
        if (i >= 24) {
            ftime.tm_mday++;
            if (has_changed < 1) {
                has_changed = 1;
                ftime.tm_hour = 0;
                ftime.tm_min = 0;
            }
            else
                ftime.tm_hour = 0;
            goto setDay;
        }
        if (ftime.tm_hour != i) {
            ftime.tm_hour = i;
            ftime.tm_min = 0;
        }


        for (i = ftime.tm_min; (bit_test(line->cl_mins, i) == 0) && (i < 60);
             i++) ;
        if (i >= 60) {
            ftime.tm_hour++;
            ftime.tm_min = 0;
            goto setHour;
        }
        ftime.tm_min = i;

 set_cl_nextexe:
        /* set cl_nextexe (handle the timezone differences) */

        /* NOTE : the output of mktime does not depend on the timezone,
         *        hence, nextexe is correct even if option timezone is used. */
        nextexe = mktime_no_dst(&ftime);

        if (is_random(line->cl_option)) {
            /* run the job at a random time during its interval of execution */
            struct tm int_end;
            time_t int_end_timet;

            debug
                ("   cmd: '%s' begin int exec %04d-%02d-%02d wday:%d %02d:%02d "
                 "(tzdiff=%d, timezone=%s)", line->cl_shell,
                 (ftime.tm_year + 1900), (ftime.tm_mon + 1), ftime.tm_mday,
                 ftime.tm_wday, ftime.tm_hour, ftime.tm_min,
                 line->cl_file->cf_tzdiff,
                 (line->cl_tz != NULL) ? line->cl_tz : "localtime");

            memcpy(&int_end, &ftime, sizeof(int_end));
            move_time_to(END_OF_INTERVAL, line, &int_end);
            int_end_timet = mktime_no_dst(&int_end);

            /* set a random time to add to the first allowed time of execution */
            nextexe += ((i = int_end_timet - nextexe) > 0) ?
                (time_t) (((float)i * (float)rand()) / (float)RAND_MAX) : 0;
        }
        else if (is_td(line->cl_option) && line->cl_runfreq != 1
                 && line->cl_jitter > 0) {
            /* &-lines only:
             * run the command between nextexe and nextexe+jitter seconds,
             * as a way not to have 100 jobs all starting exactly at
             * the second 0 of the minute they should run */
            nextexe +=
                (time_t) (((float)line->cl_jitter * (float)rand()) /
                          (float)RAND_MAX);
        }

        line->cl_nextexe = nextexe + (line->cl_file->cf_tzdiff * 3600);

        if (option != NO_GOTO) {
            if (is_random(line->cl_option)) {
                ft = localtime(&nextexe);
                memcpy(&ftime, ft, sizeof(ftime));
            }
            send_msg_fd_debug(info_fd,
                              "   cmd: '%s' next exec %04d-%02d-%02d wday:%d "
                              "%02d:%02d:%02d (tzdiff=%d, timezone=%s)",
                              line->cl_shell, (ftime.tm_year + 1900),
                              (ftime.tm_mon + 1), ftime.tm_mday, ftime.tm_wday,
                              ftime.tm_hour, ftime.tm_min, ftime.tm_sec,
                              line->cl_file->cf_tzdiff,
                              (line->cl_tz != NULL) ? line->cl_tz : "system's");
        }

        /*
         * sanity check :
         * if the nextexe is set to the past because of a bug,
         * the line will be executed again immediately, and it is most likely
         * to be set again in the past next time.
         * It would create a nasty infinite loop, a kind of "while(1) fork();"
         *
         * We add a test here to limit the consequences that would have
         * an unknown bug in this function.
         */
        if (line->cl_nextexe <= now) {
            error("BUG ??? Fcron thinks the next exe time of '%s' is %ld, "
                  "hence before now (%ld). To avoid infinite loop, nextexe"
                  " will be set at now+5s.", line->cl_shell, line->cl_nextexe);
            line->cl_nextexe = now + 5;
        }

    }
    else {
        /* this is a job based on system up time */

        if (line->cl_timefreq == LONG_MAX) {
            /* when timefreq is set to LONG_MAX, it means that next time nextexe
             * is updated we want it to be the furthest away possible so as the job
             * is never executed again (unless at the next reboot/fcron startup
             * if the line as the appropriate options set) */
            /* NOTE: the options runonce/hasrun should be used to achieve this,
             *       but we keep this here as an extra safety */
            debug
                ("Setting cl_nextexe to TIME_T_MAX to prevent the line from running again.");
            line->cl_nextexe = TIME_T_MAX;
        }
        else {
            line->cl_nextexe = basetime + line->cl_timefreq;
            if (line->cl_nextexe <= basetime) {
                /* either there was an integer overflow, or the slept time is incorrect
                 * (e.g. fcron didn't shut down cleanly and the fcrontab wasn't saved correctly) */
                error
                    ("Error while setting next exe time for job '%s': cl_nextexe"
                     " overflowed (case3). basetime=%lu, cl_timefreq=%lu, cl_nextexe=%lu. "
                     "Did fcron shut down cleanly?", line->cl_shell, basetime,
                     line->cl_timefreq, line->cl_nextexe);
                error
                    ("Setting cl_nextexe to now+cl_timefreq to prevent an infinite loop.");
                line->cl_nextexe = now + line->cl_timefreq;
                error("next execution will now be at %ld.", line->cl_nextexe);
            }
        }

        ft = localtime(&(line->cl_nextexe));

        /* localtime() function seem to return every time the same pointer :
         * it resets our previous changes, so we need to prevent it
         * ( localtime() is used in the debug() function) */
        memcpy(&ftime, ft, sizeof(struct tm));

        send_msg_fd_debug(info_fd,
                          "   cmd: '%s' next exec %04d-%02d-%02d wday:%d "
                          "%02d:%02d:%02d (system time)", line->cl_shell,
                          (ftime.tm_year + 1900), (ftime.tm_mon + 1),
                          ftime.tm_mday, ftime.tm_wday, ftime.tm_hour,
                          ftime.tm_min, ftime.tm_sec);
    }

    insert_nextexe(line);

    if (tz_changed > 0)
        switch_back_timezone(orig_tz_envvar);

}


void
set_next_exe_notrun(cl_t * line, char context)
    /* set the time of the next execution and send a mail to tell user his job
     * has not run if necessary */
{
    time_t previous_period = 0, next_period = 0;
    struct tm *ft = NULL;
    struct tm ftime, last_nextexe;
    char set_next_exe_opt = 0;
    int tz_changed = 0;

/*  // */
    debug("  set_next_exe_notrun : '%s' %d", line->cl_shell, context);
/*  // */


    /* Switch to another timezone if necessary. */
    /* If line should be scheduled in a different time zone
     * (ie. cl_tz != NULL),
     * switch to that timezone now, do the calculations,
     * and switch back to the local timezone at the end 
     * of the function. */
    tz_changed = switch_timezone(orig_tz_envvar, line->cl_tz);

    if (context == SYSDOWN || context == SYSDOWN_RUNATREBOOT) {
        /* handle timezone differences */
        previous_period = line->cl_nextexe - (line->cl_file->cf_tzdiff * 3600);
        set_next_exe_opt = NO_GOTO;
    }
    else {
        previous_period = now - (line->cl_file->cf_tzdiff * 3600);
        set_next_exe_opt = NO_GOTO_LOG;
    }
    ft = localtime(&previous_period);

    /* localtime() function seem to return every time the same pointer :
     * it resets our previous changes, so we need to prevent it
     * ( localtime() is used in the debug() function) */
    memcpy(&ftime, ft, sizeof(ftime));
    /* we also copy it to last_nextexe which will be used in mail_notrun */
    memcpy(&last_nextexe, ft, sizeof(last_nextexe));

    ftime.tm_sec = 0;
    move_time_to(BEGIN_NEXT_PERIOD, line, &ftime);
    next_period = mktime_no_dst(&ftime) + (line->cl_file->cf_tzdiff * 3600);

    if (context == SYSDOWN_RUNATREBOOT)
        line->cl_nextexe = now;
    else
        set_next_exe(line, set_next_exe_opt, -1);

    if (line->cl_nextexe >= next_period) {
        /* line has not run during one or more period(s) : send a mail */
        mail_notrun(line, context, &last_nextexe);
    }

    if (tz_changed > 0)
        switch_back_timezone(orig_tz_envvar);

}

void
mail_notrun_time_t(cl_t * line, char context, time_t since_time_t)
/* Same as mail_notrun() but with 'since' defined as a time_t instead of a struct tm */
{
    struct tm *since2 = NULL;
    struct tm since;
    int tz_changed = 0;

    since2 = localtime(&line->cl_nextexe);
    memcpy(&since, since2, sizeof(since));

    tz_changed = switch_timezone(orig_tz_envvar, line->cl_tz);

    mail_notrun(line, SYSDOWN, &since);

    if (tz_changed > 0)
        switch_back_timezone(orig_tz_envvar);

}

void
mail_notrun(cl_t * line, char context, struct tm *since)
    /* send a mail to tell user a job has not run (and why) */
{
    int pid = 0;
    FILE *mailf = 0;
    struct tm *time2 = NULL, time;
    char **sendmailenv = NULL;

    switch (pid = fork()) {
    case -1:
        error_e("Fork error : could not mail for not run '%s'", line->cl_shell);
        return;
    case 0:
        /* child */
        break;
    default:
        /* parent */

/*  // */
        debug("Reporting by mail non execution of '%s' (pid %d)",
              line->cl_shell, pid);
/*  // */

        /* create an entry in exe_list */
        /* set line to NULL as this is not a line ... */
        exe_list_add_line(exe_list, NULL);
        return;
    }

    /* If line should be scheduled in a different time zone
     * (ie. cl_tz != NULL),
     * switch to that timezone now, before we do the calculations. */
    /* No need to switch back as this function does NOT return. */
    switch_timezone(orig_tz_envvar, line->cl_tz);

    if (context == QUEUE_FULL)
        time2 = localtime(&now);
    else
        time2 = localtime(&line->cl_nextexe);
    memcpy(&time, time2, sizeof(time));

    /* create a temp file, and write in it the message to send */
    mailf = create_mail(line, "Non-execution of fcron job", NULL, NULL, NULL);

    switch (context) {
    case SYSDOWN:
        fprintf(mailf, "Line '%s' has not run since and including "
                "%04d-%02d-%02d wday:%d %02d:%02d (timezone=%s)\n"
                "due to system's down state.\n",
                line->cl_shell, (since->tm_year + 1900), (since->tm_mon + 1),
                since->tm_mday, since->tm_wday, since->tm_hour, since->tm_min,
                (line->cl_tz) ? line->cl_tz : "system's");
        fprintf(mailf,
                "It will be next executed at %04d-%02d-%02d wday:"
                "%d %02d:%02d\n", (time.tm_year + 1900), (time.tm_mon + 1),
                time.tm_mday, time.tm_wday, time.tm_hour, time.tm_min);
        break;
    case LAVG:
        fprintf(mailf, "Line '%s' has not run since and including "
                "%04d-%02d-%02d wday:%d %02d:%02d (timezone=%s)\n",
                line->cl_shell, (since->tm_year + 1900), (since->tm_mon + 1),
                since->tm_mday, since->tm_wday, since->tm_hour, since->tm_min,
                (line->cl_tz) ? line->cl_tz : "system's");
        fprintf(mailf,
                "due to a too high system load average or "
                "too many lavg-serial jobs.\n");
        fprintf(mailf,
                "It will be next executed at %04d-%02d-%02d "
                "wday:%d %02d:%02d (timezone=%s)\n", (time.tm_year + 1900),
                (time.tm_mon + 1), time.tm_mday, time.tm_wday, time.tm_hour,
                time.tm_min, (line->cl_tz) ? line->cl_tz : "system's");
        break;
    case QUEUE_FULL:
        fprintf(mailf,
                "Line '%s' couldn't be added to lavg or serial queue which"
                " is full ( %04d-%02d-%02d wday:%d %02d:%02d (timezone=%s)).\n",
                line->cl_shell, (time.tm_year + 1900), (time.tm_mon + 1),
                time.tm_mday, time.tm_wday, time.tm_hour, time.tm_min,
                (line->cl_tz) ? line->cl_tz : "system's");
        fprintf(mailf,
                "Consider using options lavgonce, until, strict, "
                "serialonce and/or fcron's option -m.\n");
        fprintf(mailf, "Note that job '%s' has not run.\n", line->cl_shell);
        break;
    }

    /* become user (for security reasons) */
    change_user_setup_env(line, &sendmailenv, NULL, NULL, NULL, NULL, NULL);

    /* then, send mail */
    launch_mailer(line, mailf, sendmailenv);

    /* we should not come here : launch_mailer does not return */
    die("mail_notrun : launch_mailer failed");

}

time_t
check_lavg(time_t lim)
    /* run a job based on system load average if one should be run
     * and return the time to sleep */
{
    time_t tts = 0;
    lavg_t *l = NULL;

#ifdef NOLOADAVG
    for (l = lavg_list_first(lavg_list); l != NULL;
         l = lavg_list_next(lavg_list)) {
        run_lavg_job(l);
        lavg_list_remove_cur(lavg_list);
    }


    tts = time_to_sleep(lim);
    return tts;
#else

    int i = 0;
    double l_avg[3] = { 0, 0, 0 };

    /* first, check if some lines must be executed because of until */
    for (l = lavg_list_first(lavg_list); l != NULL;
         l = lavg_list_next(lavg_list))
        if ((l->l_line->cl_until > 0 || l->l_line->cl_runfreq == 1)
            && l->l_until < now) {
            if (!is_run_if_late(l->l_line->cl_option)) {
                if (!is_nolog(l->l_line->cl_option))
                    explain("Interval of execution exceeded : '%s' (not run)",
                            l->l_line->cl_shell);

                /* set time of the next execution and send a mail if needed */
                if (is_td(l->l_line->cl_option) &&
                    is_notice_notrun(l->l_line->cl_option))
                    set_next_exe_notrun(l->l_line, LAVG);
                else
                    set_next_exe(l->l_line, NO_GOTO_LOG, -1);

                /* remove this job from the lavg queue */
                l->l_line->cl_numexe -= 1;
                lavg_list_remove_cur(lavg_list);
            }
            else {
                debug("until '%s' %d", l->l_line->cl_shell, l->l_until);
                run_lavg_job(l);
                lavg_list_remove_cur(lavg_list);
            }
        }

    /* we do this set here as the nextexe of lavg line may change before */
    tts = time_to_sleep(lim);

    if (lavg_list->num_entries == 0)
        return tts;

    if ((i = getloadavg(l_avg, 3)) != 3)
        debug("got only %d lavg values", i);
    debug("get_lavg: %lf, %lf, %lf", l_avg[0], l_avg[1], l_avg[2]);
    /* the 3 values stored in the fcron lines are the real value *= 10 */
    l_avg[0] *= 10;
    l_avg[1] *= 10;
    l_avg[2] *= 10;
    for (l = lavg_list_first(lavg_list); l != NULL;
         l = lavg_list_next(lavg_list)) {
        /* check if the line should be executed */
        if (lavg_serial_running >= serial_max_running &&
            is_serial(l->l_line->cl_option)) {
            continue;
        }
        if ((is_land(l->l_line->cl_option)
             && (l_avg[0] < l->l_line->cl_lavg[0] || l->l_line->cl_lavg[0] == 0)
             && (l_avg[1] < l->l_line->cl_lavg[1] || l->l_line->cl_lavg[1] == 0)
             && (l_avg[2] < l->l_line->cl_lavg[2] || l->l_line->cl_lavg[2] == 0)
            )
            || (is_lor(l->l_line->cl_option)
                && (l_avg[0] < l->l_line->cl_lavg[0]
                    || l_avg[1] < l->l_line->cl_lavg[1]
                    || l_avg[2] < l->l_line->cl_lavg[2])
            )
            ) {
            debug("lavg '%s' %s %.0f:%d %.0f:%d %.0f:%d",
                  l->l_line->cl_shell,
                  (is_lor(l->l_line->cl_option)) ? "or" : "and",
                  l_avg[0], l->l_line->cl_lavg[0],
                  l_avg[1], l->l_line->cl_lavg[1], l_avg[2],
                  l->l_line->cl_lavg[2]);
            run_lavg_job(l);
            lavg_list_remove_cur(lavg_list);

        }
    }


    if (lavg_list->num_entries == 0)
        return tts;
    else
        return (LAVG_SLEEP < tts) ? LAVG_SLEEP : tts;

#endif                          /* def NOLOADAVG */

}

time_t
time_to_sleep(time_t lim)
  /* return the time to sleep until next task have to be executed. */
{
    /* we set tts to a big value, unless some problems can occurs
     * with files without any line */
    time_t tts = lim;
    time_t ti = time(NULL);

    /* note : jobs in queue_base are sorted */
    if (queue_base != NULL) {
        if (queue_base->j_line->cl_nextexe < lim)
            tts = queue_base->j_line->cl_nextexe;
    }

    tts = tts - ti;
    if (tts < 0)
        tts = 0;

/*      debug("Time to sleep: %lds", tts); */

    return tts;
}
