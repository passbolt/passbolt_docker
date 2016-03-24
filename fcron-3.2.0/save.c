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
#include "save.h"

extern char debug_opt;

int write_buf_to_disk(int fd, char *write_buf, int *buf_used);
int save_type(int fd, short int type, char *write_buf, int *buf_used);
int save_str(int fd, short int type, char *str, char *write_buf, int *buf_used);
int save_strn(int fd, short int type, char *str, short int size,
              char *write_buf, int *buf_used);
int save_lint(int fd, short int type, long int value, char *write_buf,
              int *buf_used);
int save_one_file(cf_t * file, char *filename, uid_t own_uid, gid_t own_gid,
                  time_t save_date);


int
save_type(int fd, short int type, char *write_buf, int *buf_used)
/* save a single type (with no data attached) in a binary fcrontab file */
{
    short int size = 0;
    int write_len = sizeof(type) + sizeof(size);

    if (write_len > WRITE_BUF_LEN - *buf_used)
        if (write_buf_to_disk(fd, write_buf, buf_used) == ERR)
            return ERR;

    memcpy((write_buf + *buf_used), &type, sizeof(type));
    *buf_used += sizeof(type);
    memcpy((write_buf + *buf_used), &size, sizeof(size));
    *buf_used += sizeof(size);

    return OK;

}

int
save_str(int fd, short int type, char *str, char *write_buf, int *buf_used)
/* save a string of type "type" in a binary fcrontab file */
{
    short int size = (str != NULL) ? strlen(str) : 0;
    int write_len = sizeof(type) + sizeof(size) + size;

    if (write_len > WRITE_BUF_LEN - *buf_used)
        if (write_buf_to_disk(fd, write_buf, buf_used) == ERR)
            return ERR;

    memcpy((write_buf + *buf_used), &type, sizeof(type));
    *buf_used += sizeof(type);
    memcpy((write_buf + *buf_used), &size, sizeof(size));
    *buf_used += sizeof(size);
    memcpy((write_buf + *buf_used), str, size);
    *buf_used += size;

    return OK;
}

int
save_strn(int fd, short int type, char *str, short int size, char *write_buf,
          int *buf_used)
/* save a "size"-length string of type "type" in a binary fcrontab file */
{
    int write_len = sizeof(type) + sizeof(size) + size;

    if (write_len > WRITE_BUF_LEN - *buf_used)
        if (write_buf_to_disk(fd, write_buf, buf_used) == ERR)
            return ERR;

    memcpy((write_buf + *buf_used), &type, sizeof(type));
    *buf_used += sizeof(type);
    memcpy((write_buf + *buf_used), &size, sizeof(size));
    *buf_used += sizeof(size);
    memcpy((write_buf + *buf_used), str, size);
    *buf_used += size;

    return OK;
}

int
save_lint(int fd, short int type, long int value, char *write_buf,
          int *buf_used)
/* save an integer of type "type" in a binary fcrontab file */
{
    short int size = sizeof(value);
    int write_len = sizeof(type) + sizeof(size) + size;

    if (write_len > WRITE_BUF_LEN - *buf_used)
        if (write_buf_to_disk(fd, write_buf, buf_used) == ERR)
            return ERR;

    memcpy((write_buf + *buf_used), &type, sizeof(type));
    *buf_used += sizeof(type);
    memcpy((write_buf + *buf_used), &size, sizeof(size));
    *buf_used += sizeof(size);
    memcpy((write_buf + *buf_used), &value, size);
    *buf_used += size;

    return OK;
}


int
write_buf_to_disk(int fd, char *write_buf, int *buf_used)
/* write the buffer to disk */
{
    ssize_t to_write = *buf_used;
    ssize_t written = 0;
    ssize_t return_val;
    int num_retries = 0;

    while (written < to_write) {
        if (num_retries++ > (int)(to_write / 2)) {
            error("too many retries (%d) to write buf to disk : giving up.",
                  num_retries);
            return ERR;
        }
        return_val = write(fd, (write_buf + written), to_write - written);
        if (return_val == -1) {
            error_e("could not write() buf to disk");
            return ERR;
        }
        written += return_val;
    }

    /* */
    debug("write_buf_to_disk() : written %d/%d, %d (re)try(ies)", written,
          to_write, num_retries);
    /* */

    if (written == to_write) {
        *buf_used = 0;
        return OK;
    }
    else {
        error("write_buf_to_disk() : written %d bytes for %d requested.",
              written, to_write);
        return ERR;
    }
}


/* write_file_to_disk() error management */
#define Save_type(FD, TYPE, BUF, BUF_USED) \
        { \
          if ( save_type(FD, TYPE, BUF, BUF_USED) != OK ) { \
            error_e("Could not write type : file %s has not been saved.", \
                     file->cf_user); \
            return ERR; \
	  } \
        }

#define Save_str(FD, TYPE, STR, BUF, BUF_USED) \
        { \
          if ( save_str(FD, TYPE, STR, BUF, BUF_USED) != OK ) { \
            error_e("Could not write str : file %s has not been saved.", \
                     file->cf_user); \
            return ERR; \
	  } \
        }

#define Save_strn(FD, TYPE, STR, SIZE, BUF, BUF_USED) \
        { \
          if ( save_strn(FD, TYPE, STR, SIZE, BUF, BUF_USED) != OK ) { \
            error_e("Could not write strn : file %s has not been saved.", \
                     file->cf_user); \
            return ERR; \
	  } \
        }

#define Save_lint(FD, TYPE, VALUE, BUF, BUF_USED) \
        { \
          if ( save_lint(FD, TYPE, VALUE, BUF, BUF_USED) != OK ) { \
            error_e("Could not write lint : file %s has not been saved.", \
                     file->cf_user); \
            return ERR; \
	  } \
        }

int
write_file_to_disk(int fd, struct cf_t *file, time_t time_date)
/* write the data on the disk */
{
    cl_t *line = NULL;
    env_t *env = NULL;
    char write_buf[WRITE_BUF_LEN];
    int write_buf_used = 0;

    /* put program's version : it permits to daemon not to load
     * a file which he won't understand the syntax, for exemple
     * a file using a depreciated format generated by an old fcrontab,
     * if the syntax has changed */
    /* an binary fcrontab *must* start by such a header */
    Save_lint(fd, S_HEADER_T, S_FILEVERSION, write_buf, &write_buf_used);

    /* put the user's name : needed to check if his uid has not changed */
    /* S_USER_T *must* be the 2nd field of a binary fcrontab */
    Save_str(fd, S_USER_T, file->cf_user, write_buf, &write_buf_used);

    /* put the time & date of saving : this is use for calcutating 
     * the system down time. As it is a new file, we set it to 0 */
    /* S_USER_T *must* be the 3rd field of a binary fcrontab */
    Save_lint(fd, S_TIMEDATE_T, time_date, write_buf, &write_buf_used);

    /* Save the time diff between local (real) and system hour (if any) */
    if (file->cf_tzdiff != 0)
        Save_lint(fd, S_TZDIFF_T, file->cf_tzdiff, write_buf, &write_buf_used);

    /*   env variables, */
    for (env = env_list_first(file->cf_env_list);
         env != NULL; env = env_list_next(file->cf_env_list)) {
        Save_str(fd, S_ENVVAR_T, env->e_envvar, write_buf, &write_buf_used);
    }

    /*   then, lines. */
    for (line = file->cf_line_base; line; line = line->cl_next) {

        /* this ones are saved for every lines */
        Save_str(fd, S_SHELL_T, line->cl_shell, write_buf, &write_buf_used);
        Save_str(fd, S_RUNAS_T, line->cl_runas, write_buf, &write_buf_used);
        Save_str(fd, S_MAILTO_T, line->cl_mailto, write_buf, &write_buf_used);
        Save_strn(fd, S_OPTION_T, (char *)line->cl_option, OPTION_SIZE,
                  write_buf, &write_buf_used);
        Save_lint(fd, S_NEXTEXE_T, line->cl_nextexe, write_buf,
                  &write_buf_used);

        /* the following are saved only if needed */
        if (line->cl_numexe)
            Save_strn(fd, S_NUMEXE_T, (char *)&line->cl_numexe, 1, write_buf,
                      &write_buf_used);
        if (is_lavg(line->cl_option))
            Save_strn(fd, S_LAVG_T, (char *)line->cl_lavg, LAVG_SIZE,
                      write_buf, &write_buf_used);
        if (line->cl_until > 0)
            Save_lint(fd, S_UNTIL_T, line->cl_until, write_buf,
                      &write_buf_used);
        if (line->cl_nice != 0)
            Save_strn(fd, S_NICE_T, &line->cl_nice, 1, write_buf,
                      &write_buf_used);
        if (line->cl_runfreq > 0) {
            Save_lint(fd, S_RUNFREQ_T, line->cl_runfreq, write_buf,
                      &write_buf_used);
            Save_lint(fd, S_REMAIN_T, line->cl_remain, write_buf,
                      &write_buf_used);
        }
        if (line->cl_tz != NULL) {
            Save_str(fd, S_TZ_T, line->cl_tz, write_buf, &write_buf_used);
        }
        if (line->cl_jitter > 0) {
            Save_lint(fd, S_JITTER_T, line->cl_jitter, write_buf,
                      &write_buf_used);
        }

        if (is_freq(line->cl_option)) {
            /* save the frequency to run the line */
            Save_lint(fd, S_FIRST_T, line->cl_first, write_buf,
                      &write_buf_used);
            Save_lint(fd, S_TIMEFREQ_T, line->cl_timefreq, write_buf,
                      &write_buf_used);
        }
        else {
            /* save the time and date bit fields */
            Save_strn(fd, S_MINS_T, (char *)line->cl_mins, bitstr_size(60),
                      write_buf, &write_buf_used);
            Save_strn(fd, S_HRS_T, (char *)line->cl_hrs, bitstr_size(24),
                      write_buf, &write_buf_used);
            Save_strn(fd, S_DAYS_T, (char *)line->cl_days, bitstr_size(32),
                      write_buf, &write_buf_used);
            Save_strn(fd, S_MONS_T, (char *)line->cl_mons, bitstr_size(12),
                      write_buf, &write_buf_used);
            Save_strn(fd, S_DOW_T, (char *)line->cl_dow, bitstr_size(8),
                      write_buf, &write_buf_used);
        }

        /* This field *must* be the last of each line */
        Save_type(fd, S_ENDLINE_T, write_buf, &write_buf_used);
    }

    if (write_buf_to_disk(fd, write_buf, &write_buf_used) == ERR) {
        error
            ("Could not write final buffer content to disk: file %s has not been saved.",
             file->cf_user);
        return ERR;
    }

    /* Make sure the data is fully synchronize to disk before files are renamed 
     * to their final destination.
     * This is to avoid cases where the file name (meta-data) would be updated,
     * and there is a crash before the data is fully written: not sure if that
     * is possible, but better safe than sorry! */
    if (fdatasync(fd) < 0) {
        error_e("could not fdatasync() %s's fcrontab", file->cf_user);
        return ERR;
    }

    return OK;
}


int
save_one_file(cf_t * file, char *filename, uid_t own_uid, gid_t own_gid,
              time_t save_date)
/* save a given file to disk */
{
    int fd;

    /* open file */
#ifdef WITH_SELINUX
    if (is_selinux_enabled() && setfscreatecon(file->cf_file_context)) {
        error_e("Could not set create context for file %s", filename);
        return ERR;
    }
#endif
    fd = open_as_user(filename, own_uid, own_gid,
                      O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR);
#ifdef WITH_SELINUX
    if (is_selinux_enabled())
        setfscreatecon(NULL);
#endif
    if (fd == -1) {
        error_e("Could not open %s", filename);
        return ERR;
    }

    if (fchown(fd, own_uid, own_gid) != 0) {
        error_e("Could not fchown %s to uid:%d gid:%d", filename, own_uid,
                own_gid);
        if (xclose(&fd) < 0)
            error_e("save_one_file(%s): could not xclose(fd)", filename);
        remove_as_user(filename, own_uid, own_gid);
        return ERR;
    }

    /* save file : */
    if (write_file_to_disk(fd, file, save_date) == ERR) {
        if (xclose(&fd) < 0)
            error_e("save_one_file(%s): could not xclose(fd)", filename);
        remove_as_user(filename, own_uid, own_gid);
        return ERR;
    }

    if (xclose(&fd) < 0)
        error_e("save_one_file(%s): could not xclose(fd)", filename);

    return OK;
}


int
save_file_safe(cf_t * file, char *final_path, char *prog_name, uid_t own_uid,
               gid_t own_gid, time_t save_date)
/* save a file to a temp path, and then rename it (safely) to avoid loss of data
 * if a system crash, hardware failure, etc happens. */
{
    char temp_path[PATH_LEN + 4];
    int final_path_len, temp_path_index;
    char *tmp_str = ".tmp";

    final_path_len = strlen(final_path);
    strncpy(temp_path, final_path, sizeof(temp_path) - sizeof(tmp_str));
    temp_path_index = (final_path_len > sizeof(temp_path) - sizeof(tmp_str)) ?
        sizeof(temp_path) - sizeof(tmp_str) : final_path_len;
    strcpy(&temp_path[temp_path_index], tmp_str);

    if (save_one_file(file, temp_path, own_uid, own_gid, save_date) == OK) {
        if (rename_as_user(temp_path, final_path, own_uid, own_gid) != 0) {
            error_e("Cannot rename %s to %s", temp_path, final_path);
            error("%s will try to save the name to its definitive filename "
                  "directly.", prog_name);
            error
                ("If there is an error, root may consider to replace %s (which is "
                 "a valid copy) by %s manually.", final_path, temp_path);
            if (save_one_file(file, final_path, own_uid, own_gid, save_date) ==
                ERR)
                return ERR;
        }
    }
    else {
        error("Since %s has not been able to save %s's file, it will keep "
              "the previous version (if any) of %s.", prog_name, final_path,
              final_path);
        return ERR;
    }

    return OK;

}
