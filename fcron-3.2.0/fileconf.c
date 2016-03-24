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


#include "fcrontab.h"

#include "fileconf.h"

char *get_string(char *ptr);
int get_line(char *str, size_t size, FILE * file);
void init_default_line(cl_t * cl, cf_t * cf);
char *get_time(char *ptr, time_t * time, int zero_allowed);
char *get_num(char *ptr, int *num, int max, short int decimal,
              const char **names);
char *get_nice(char *ptr, int *nice);
char *get_bool(char *ptr, int *i);
char *read_field(char *ptr, bitstr_t * ary, int max, const char **names);
void read_freq(char *ptr, cf_t * cf);
void read_arys(char *ptr, cf_t * cf);
void read_period(char *ptr, cf_t * cf);
int read_shortcut(char *ptr, cf_t * cf);
void read_env(char *ptr, cf_t * cf);
char *read_opt(char *ptr, cl_t * cl);
char *check_username(char *ptr, cf_t * cf, cl_t * cl);
size_t option_strlen(char *value);

char need_correction;
cl_t default_line;              /* default options for a line */
char *file_name;
int line;

/* warning : all names must have the same length */
const char *dows_ary[] = {
    "sun", "mon", "tue", "wed", "thu", "fri", "sat",
    NULL
};

/* warning : all names must have the same length */
const char *mons_ary[] = {
    "jan", "feb", "mar", "apr", "may", "jun",
    "jul", "aug", "sep", "oct", "nov", "dec",
    NULL
};


#define GET_LINE_EOF 999

char *
get_string(char *ptr)
    /* read string pointed by ptr, remove blanks and manage
     * string placed in quotes */
    /* return NULL on mismatched quotes */
{
    char quote = 0;
    int length = 0;
    char *rtn_string = NULL;

    if (*ptr == '\"' || *ptr == '\'') {
        quote = *ptr;
        ptr++;
    }

    length = remove_blanks(ptr);

    if (quote != 0) {
        if (*(ptr + length - 1) == quote)
            *(ptr + length - 1) = '\0';
        else {
            /* mismatched quotes */
            need_correction = 1;
            return NULL;
        }
    }

    Set(rtn_string, ptr);
    return rtn_string;

}


int
get_line(char *str, size_t size, FILE * file)
    /* similar to fgets, but increase line if necessary,
     * and continue over an "\" followed by an "\n" char */
{
    size_t size_max = size - 1;
    int i = 0;
    int c;

    while (i < size_max) {

        switch (c = getc(file)) {

        case '\n':
            /* check if the \n char is preceded by a "\" char :
             *  in this case, suppress the "\", don't copy the \n,
             *  and continue */
            if (i > 0 && *(str + i - 1) == '\\') {
                i--;
                line++;
                continue;
            }
            else {
                *(str + i) = (char)'\0';
                return OK;
            }
            break;

        case EOF:
            *(str + i) = (char)'\0';
            /* we couldn't return EOF ( equal to ERR by default )
             * nor ERR, which is used for another error */
            return GET_LINE_EOF;

        default:
            *(str + i) = (char)c;
            i++;

        }

    }

    /* line is too long : goto next line and return ERR */
    while (((c = getc(file)) != EOF) && (c != '\n')) ;
    line++;
    need_correction = 1;
    return ERR;

}

void
init_default_line(cl_t * cl, cf_t * cf)
/* clear all context/options from cl */
{
    bzero(cl, sizeof(cl_t));
    Set(cl->cl_runas, runas);
    Set(cl->cl_mailto, runas);
    Free_safe(cl->cl_tz);
    set_default_opt(cl->cl_option);
    cl->cl_file = cf;
}


int
read_file(char *filename, int fd)
    /* read file "name" and append cf_t list */
{
    cf_t *cf = NULL;
    FILE *file = NULL;
    char buf[LINE_LEN];
    int max_lines;
    int max_entries = MAXENTRIES;
    int entries = 0;
    char *ptr = NULL;
    int ret;

    bzero(buf, sizeof(buf));
    need_correction = 0;
    line = 1;
    file_name = filename;

    /* open file */

    if ((file = fdopen(fd, "r")) == NULL) {
        fprintf(stderr, "Could not open \"%s\": %s\n", file_name,
                strerror(errno));
        return ERR;
    }

    /* Rewind, just in case */
    rewind(file);

    Alloc(cf, cf_t);
    cf->cf_env_list = env_list_init();
    Set(cf->cf_user, user);
    init_default_line(&default_line, cf);

    if (debug_opt)
        fprintf(stderr, "FILE %s\n", file_name);

    if (strcmp(runas, ROOTNAME) == 0)
        max_entries = 65535;

    /* max_lines acts here as a security counter to avoid endless loop. */
    max_lines = (max_entries * 10) + 10;

    while (entries < max_entries && line <= max_lines) {

        ret = get_line(buf, sizeof(buf), file);

        if (ret == ERR) {
            fprintf(stderr,
                    "%s:%d: Line is too long (more than %d): skipping line.\n",
                    file_name, line, (int)sizeof(buf));
            continue;
        }

        ptr = buf;
        Skip_blanks(ptr);

        if (debug_opt && *ptr != '#' && *ptr != '\0')
            fprintf(stderr, "      %s\n", buf);

        switch (*ptr) {
        case '#':
        case '\0':
            /* comments or empty line: skipping */
            break;
        case '@':
            /* if it is not a shortcut line then read_shortcut() won't do anything. */
            if (!read_shortcut(ptr, cf))
                read_freq(ptr, cf);
            entries++;
            break;
        case '&':
            read_arys(ptr, cf);
            entries++;
            break;
        case '%':
            read_period(ptr, cf);
            entries++;
            break;
        case '!':
            ptr = read_opt(ptr, &default_line);
            if (ptr != NULL && *ptr != '\0') {
                fprintf(stderr, "%s:%d: Syntax error: string \"%s\" ignored\n",
                        file_name, line, ptr);
                need_correction = 1;
            }
            break;
        default:
            if (isdigit((int)*ptr) || *ptr == '*') {
                read_arys(ptr, cf);
                entries++;
            }
            else
                read_env(ptr, cf);
        }

        line++;

        if (ret != OK)
            /* in this case, ret == GET_LINE_EOF :
             * no more lines, so we exit the loop */
            break;

    }

    if (entries == max_entries) {
        error("%s:%d: maximum number of entries (%d) has been reached by %s",
              file_name, line, user);
        fprintf(stderr, "Anything after this line will be ignored\n");
    }
    else if (line == max_lines)
        error("%s:%d: maximum number of lines (%d) has been reached by %s",
              file_name, line, user);

    cf->cf_next = file_base;
    file_base = cf;

    /* don't close as underlying fd may still be used by calling function */
    if (fflush(file) != 0)
        error_e("could not fflush() file_name");

    Free_safe(default_line.cl_runas);
    Free_safe(default_line.cl_mailto);
    Free_safe(default_line.cl_tz);

    if (!need_correction)
        return OK;
    else
        return 2;

}

void
read_env(char *ptr, cf_t * cf)
    /* append env variable list.
     * (remove blanks) */
{
    char name[LINE_LEN];
    int j = 0;
    char *val = NULL;

    bzero(name, sizeof(name));

    /* copy env variable's name */
    while ((isalnum((int)*ptr) || *ptr == '_') && *ptr != '='
           && !isspace((int)*ptr) && j < sizeof(name)) {
        name[j++] = *ptr;
        ptr++;
    }
    name[j] = '\0';

    if (name[0] == '\0')
        goto error;

    /* skip '=' and spaces around */
    while (isspace((int)*ptr))
        ptr++;

    /* if j == 0 name is a zero length string */
    if (*ptr++ != '=' || j == 0)
        goto error;

    while (isspace((int)*ptr))
        ptr++;

    /* get value */
    if ((val = get_string(ptr)) == NULL) {
        fprintf(stderr, "%s:%d: Mismatched  quotes: skipping line.\n",
                file_name, line);
        need_correction = 1;
        return;
    }

    if (debug_opt)
        fprintf(stderr, "  Env : '%s=%s'\n", name, val);

    /* we ignore USER/LOGNAME's assignment */
    if (strcmp(name, "USER") == 0 || strcmp(name, "LOGNAME") == 0) {
        fprintf(stderr,
                "%s:%d: USER or LOGNAME assignement is not allowed: ignored.\n",
                file_name, line);
        return;
    }

    /* the MAILTO assignment is, in fact, an fcron option :
     *  we don't store it in the same way. */
    /* please note that we check if the mailto is valid in conf.c */
    if (strcmp(name, "MAILTO") == 0) {
        if (strcmp(val, "\0") == 0) {
            clear_mail(default_line.cl_option);
            clear_mailzerolength(default_line.cl_option);
        }
        else {
            Set(default_line.cl_mailto, val);
            set_mail(default_line.cl_option);
        }

    }
    else {
        env_list_setenv(cf->cf_env_list, name, val, 1);
    }

    Free_safe(val);

    return;

 error:
    fprintf(stderr, "%s:%d: Syntax error: skipping line.\n", file_name, line);
    need_correction = 1;
    return;

}


char *
get_nice(char *ptr, int *nice)
    /* read a nice value and put it in variable nice */
{
    char negative = 0;

    if (*ptr == '-') {
        negative = 1;
        ptr++;
    }

    if ((ptr = get_num(ptr, nice, 20, 0, NULL)) == NULL)
        return NULL;

    if (negative == 1) {
        if (getuid() != rootuid) {
            fprintf(stderr, "must be privileged to use a negative argument "
                    "with nice: set to 0\n");
            need_correction = 1;
            *nice = 0;
        }

        *nice *= (-1);
    }

    return ptr;

}

size_t
option_strlen(char *value)
/* return the length of the string value of an option */
{
    char *ptr = value;
    size_t len = 0;

    /* look for the end of the option value */
    while (ptr != NULL && *ptr != ')' && *ptr != '\0') {
        ptr++;
        len++;
    }

    return len;
}

int
assign_option_string(char **var, char *value)
/* Read the value of an option: if non-empty, assign it to the var,
 * otherwise set to NULL.
 * Returns the length of the value (0 if empty), or -1 on error. */
{
    char start = value[0];
    char end = '\0';
    size_t len = 0;

    Free_safe(*var);

    len = option_strlen(value);

    if (len <= 0) {
        return len;
    }

    end = value[len - 1];

    /* spaces and quotes are not allowed before or after the value */
    if (isspace((int)start) || isspace((int)end)
        || start == '\'' || start == '"' || end == '\'' || end == '"') {
        return -1;
    }

    *var = strndup2(value, len);

    return len;
}


char *
get_bool(char *ptr, int *i)
    /* get a bool value : either true (1) or false (0)
     * return NULL on error */
{
    if (*ptr == '1')
        goto true;
    else if (*ptr == '0')
        goto false;
    else if (strncmp(ptr, "true", 4) == 0) {
        ptr += 3;
        goto true;
    }
    else if (strncmp(ptr, "yes", 3) == 0) {
        ptr += 2;
        goto true;
    }
    else if (strncmp(ptr, "false", 5) == 0) {
        ptr += 4;
        goto false;
    }
    else if (strncmp(ptr, "no", 2) == 0) {
        ptr += 1;
        goto false;
    }
    else
        return NULL;

 true:
    *i = 1;
    ptr++;
    return ptr;

 false:
    *i = 0;
    ptr++;
    return ptr;

}


char *
read_opt(char *ptr, cl_t * cl)
    /* read one or several options and fill in the field "option" */
{
    char opt_name[20];
    int i;
    char in_brackets;

#define Handle_err \
    { \
        fprintf(stderr, "%s:%d: Argument(s) for option \"%s\" not valid: " \
		"skipping end of line.\n", file_name, line, opt_name); \
        need_correction = 1; \
        return NULL; \
    }

    if (*ptr == '!')
        ptr++;

    do {
        i = 0;
        bzero(opt_name, sizeof(opt_name));

        while (isalnum((int)*ptr) && i < sizeof(opt_name))
            opt_name[i++] = *ptr++;

        i = 1;
        in_brackets = 0;

        if (*ptr == '(') {
            in_brackets = 1;
            ptr++;

            /* spaces are not allowed -- make sure there is no leading space. */
            if (isspace((int)*ptr)) {
                Handle_err;
            }
        }

        /* global options for a file */

        if (strcmp(opt_name, "tzdiff") == 0) {
            char negative = 0;

            if (!in_brackets)
                Handle_err;
            if (*ptr == '-') {
                negative = 1;
                ptr++;
            }
            if ((ptr = get_num(ptr, &i, 24, 0, NULL)) == NULL)
                Handle_err;
            if (negative)
                cl->cl_file->cf_tzdiff = (-i);
            else
                cl->cl_file->cf_tzdiff = i;

            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" (-)%d\n", opt_name, i);
        }

        /* options related to a line (or a set of lines) */

        else if (strcmp(opt_name, "timezone") == 0) {
            int len = -1;

            if (!in_brackets) {
                Handle_err;
            }

            /* assign_option_string() will set cl_tz to NULL is the value is empty,
             * which means "use the system timezone" */
            len = assign_option_string(&(cl->cl_tz), ptr);
            if (len < 0) {
                Handle_err;
            }
            else {
                ptr += len;
            }

            if (debug_opt) {
                fprintf(stderr, "  Opt : \"%s\" \"%s\"\n", opt_name, cl->cl_tz);
            }
        }


        else if (strcmp(opt_name, "s") == 0 || strcmp(opt_name, "serial") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_serial(cl->cl_option);
            else
                set_serial(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "serialonce") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                set_serial_sev(cl->cl_option);
            else
                clear_serial_sev(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "lavgonce") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                set_lavg_sev(cl->cl_option);
            else
                clear_lavg_sev(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "exesev") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_exe_sev(cl->cl_option);
            else
                set_exe_sev(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "b") == 0 || strcmp(opt_name, "bootrun") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_bootrun(cl->cl_option);
            else
                set_bootrun(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "rebootreset") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_rebootreset(cl->cl_option);
            else
                set_rebootreset(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }


        else if (strcmp(opt_name, "runatreboot") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_runatreboot(cl->cl_option);
            else
                set_runatreboot(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }


        else if (strcmp(opt_name, "runonce") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_runonce(cl->cl_option);
            else
                set_runonce(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "reset") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 1) {
                init_default_line(cl, cl->cl_file);
            }
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
        }

        else if (strcmp(opt_name, "f") == 0 || strcmp(opt_name, "first") == 0) {
            if (!in_brackets
                || (ptr = get_time(ptr, &(cl->cl_first), 1)) == NULL)
                Handle_err;
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %ld\n", opt_name,
                        (long int)cl->cl_first);
        }

        else if (strcmp(opt_name, "r") == 0 || strcmp(opt_name, "runfreq") == 0) {
            if (cl->cl_runfreq == 1) {
                fprintf(stderr, "cannot change runfreq value in a %%-line");
                Handle_err;
            }
            if (!in_brackets
                || (ptr = get_num(ptr, &i, USHRT_MAX, 0, NULL)) == NULL)
                Handle_err;
            if (i <= 1) {
                fprintf(stderr, "runfreq must be 2 or more.\n");
                Handle_err;
            }
            cl->cl_runfreq = i;
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "strict") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_strict(cl->cl_option);
            else
                set_strict(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "noticenotrun") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_notice_notrun(cl->cl_option);
            else
                set_notice_notrun(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "lavg") == 0) {
            if (!in_brackets
                || (ptr = get_num(ptr, &i, UCHAR_MAX, 1, NULL)) == NULL)
                Handle_err;
            cl->cl_lavg[0] = i;
            if (debug_opt)
                fprintf(stderr, "  Opt : 'lavg1' %d\n", i);
            if (*ptr++ != ',')
                Handle_err;
            if (!in_brackets
                || (ptr = get_num(ptr, &i, UCHAR_MAX, 1, NULL)) == NULL)
                Handle_err;
            cl->cl_lavg[1] = i;
            if (debug_opt)
                fprintf(stderr, "  Opt : 'lavg5' %d\n", i);
            if (*ptr++ != ',')
                Handle_err;
            if (!in_brackets
                || (ptr = get_num(ptr, &i, UCHAR_MAX, 1, NULL)) == NULL)
                Handle_err;
            cl->cl_lavg[2] = i;
            if (debug_opt)
                fprintf(stderr, "  Opt : 'lavg15' %d\n", i);
            if (cl->cl_lavg[0] || cl->cl_lavg[1] || cl->cl_lavg[2])
                set_lavg(cl->cl_option);
            else
                clear_lavg(cl->cl_option);
#ifdef NOLOADAVG
            warn("As fcron has been compiled with no procfs support,\n"
                 "you will not be able to use the lavg* options");
#endif                          /* NOLOADAVG */
        }

        else if (strcmp(opt_name, "lavg1") == 0) {
            if (!in_brackets
                || (ptr = get_num(ptr, &i, UCHAR_MAX, 1, NULL)) == NULL)
                Handle_err;
            cl->cl_lavg[0] = i;
            if (cl->cl_lavg[0] || cl->cl_lavg[1] || cl->cl_lavg[2])
                set_lavg(cl->cl_option);
            else
                clear_lavg(cl->cl_option);
#if NOLOADAVG
            warn("As fcron has been compiled with no procfs support,\n"
                 "you will not be able to use the lavg* options");
#endif                          /* NOLOADAVG */
            if (debug_opt)
                fprintf(stderr, "  Opt : 'lavg1' %d\n", i);
        }

        else if (strcmp(opt_name, "lavg5") == 0) {
            if (!in_brackets
                || (ptr = get_num(ptr, &i, UCHAR_MAX, 1, NULL)) == NULL)
                Handle_err;
            cl->cl_lavg[1] = i;
            if (cl->cl_lavg[0] || cl->cl_lavg[1] || cl->cl_lavg[2])
                set_lavg(cl->cl_option);
            else
                clear_lavg(cl->cl_option);
#ifdef NOLOADAVG
            warn("As fcron has been compiled with no procfs support,\n"
                 "you will not be able to use the lavg* options");
#endif                          /* NOLOADAVG = 0 */
            if (debug_opt)
                fprintf(stderr, "  Opt : 'lavg5' %d\n", i);
        }

        else if (strcmp(opt_name, "lavg15") == 0) {
            if (!in_brackets
                || (ptr = get_num(ptr, &i, UCHAR_MAX, 1, NULL)) == NULL)
                Handle_err;
            cl->cl_lavg[2] = i;
            if (cl->cl_lavg[0] || cl->cl_lavg[1] || cl->cl_lavg[2])
                set_lavg(cl->cl_option);
            else
                clear_lavg(cl->cl_option);
#ifdef NOLOADAVG
            warn("As fcron has been compiled with no procfs support,\n"
                 "you will not be able to use the lavg* options");
#endif                          /* NOLOADAVG = 0 */
            if (debug_opt)
                fprintf(stderr, "  Opt : 'lavg15' %d\n", i);
        }

        else if (strcmp(opt_name, "lavgand") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                set_lor(cl->cl_option);
            else
                set_land(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "lavgor") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                set_land(cl->cl_option);
            else
                set_lor(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "u") == 0 || strcmp(opt_name, "until") == 0) {
            if (!in_brackets
                || (ptr = get_time(ptr, &(cl->cl_until), 0)) == NULL)
                Handle_err;
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %ld\n", opt_name,
                        (long int)cl->cl_until);
        }

        else if (strcmp(opt_name, "m") == 0 || strcmp(opt_name, "mail") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0) {
                clear_mail(cl->cl_option);
                clear_mailzerolength(cl->cl_option);
            }
            else
                set_mail(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "forcemail") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_mailzerolength(cl->cl_option);
            else {
                set_mailzerolength(cl->cl_option);
                set_mail(cl->cl_option);
            }
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "mailto") == 0) {
            int len = -1;

            if (!in_brackets) {
                Handle_err;
            }

            /* assign_option_string() set the value to NULL if the length is zero.
             * However cl_mailto must not be NULL (as expected in
             * conf.c:add_line_to_file()), so we check if the length is >= 0
             * before calling assign_option_string() */
            /* Also please note that we check if the mailto is valid in conf.c */
            len = option_strlen(ptr);
            if (len <= 0) {
                clear_mail(cl->cl_option);
                clear_mailzerolength(cl->cl_option);
                if (debug_opt) {
                    fprintf(stderr, "  Opt : \"mail\" 0\n");
                    fprintf(stderr, "  Opt : \"forcemail\" 0\n");
                }
            }
            else {
                len = assign_option_string(&(cl->cl_mailto), ptr);
                if (len < 0) {
                    Handle_err;
                }
                else {
                    ptr += len;
                    set_mail(cl->cl_option);
                }
            }

            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" \"%s\"\n", opt_name,
                        cl->cl_mailto);
        }

        else if (strcmp(opt_name, "erroronlymail") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_erroronlymail(cl->cl_option);
            else
                set_erroronlymail(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "dayand") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                set_dayor(cl->cl_option);
            else
                set_dayand(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "dayor") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                set_dayand(cl->cl_option);
            else
                set_dayor(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "nolog") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_nolog(cl->cl_option);
            else
                set_nolog(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "volatile") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_volatile(cl->cl_option);
            else
                set_volatile(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "stdout") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_stdout(cl->cl_option);
            else
                set_stdout(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "n") == 0 || strcmp(opt_name, "nice") == 0) {
            if (!in_brackets || (ptr = get_nice(ptr, &i)) == NULL)
                Handle_err;
            cl->cl_nice = (char)i;
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" (-)%d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "runas") == 0) {
            int len = -1;
            char *runas = NULL;
            struct passwd *pas;

            if (!in_brackets) {
                Handle_err;
            }

            len = assign_option_string(&runas, ptr);
            if (len <= 0) {
                Handle_err;
            }
            else {
                ptr += len;
            }

            if (getuid() != rootuid) {
                fprintf(stderr, "must be privileged to use option runas: "
                        "skipping option\n");
                need_correction = 1;
            }
            else if (len > USER_NAME_LEN) {
                fprintf(stderr, "runas: user name \"%s\" longer than %d"
                        "characters: skipping option\n", runas, USER_NAME_LEN);
                need_correction = 1;
            }
            else if ((pas = getpwnam(runas)) == NULL) {
                fprintf(stderr, "runas: \"%s\" is not in passwd file : "
                        "ignored", runas);
                need_correction = 1;
            }

            if (need_correction) {
                Free_safe(runas);
            }
            else {
                /* only set cl_runas if all is well, as cl_runas MUST always
                 * be set to a valid value */
                Set(cl->cl_runas, runas);
            }

            /* all good */
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %s ptr=%p\n", opt_name,
                        cl->cl_runas, cl->cl_runas);

        }

        else if (strcmp(opt_name, "random") == 0) {
            if (in_brackets && (ptr = get_bool(ptr, &i)) == NULL)
                Handle_err;
            if (i == 0)
                clear_random(cl->cl_option);
            else
                set_random(cl->cl_option);
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        else if (strcmp(opt_name, "jitter") == 0) {
            if (!in_brackets
                || (ptr = get_num(ptr, &i, UCHAR_MAX, 0, NULL)) == NULL)
                Handle_err;
            cl->cl_jitter = i;
            if (debug_opt)
                fprintf(stderr, "  Opt : \"%s\" %d\n", opt_name, i);
        }

        /* handle %-line : we check if we are really in a %-line (which we do not do
         * for other options), because writing "&hourly" in a fcrontab results in an
         * error (hourly ignored) hard to find, and, in any case, annoying. */
        else if (cl->cl_runfreq == 1) {
            /* options to run once per interval :
             * ignore every fields below the limit */
            if (strcmp(opt_name, "mins") == 0) {
                /* nothing to do */
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
            else if (strcmp(opt_name, "hours") == 0) {
                set_freq_mins(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
            else if (strcmp(opt_name, "days") == 0) {
                set_freq_mins(cl->cl_option);
                set_freq_hrs(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
            else if (strcmp(opt_name, "mons") == 0) {
                set_freq_mins(cl->cl_option);
                set_freq_hrs(cl->cl_option);
                set_freq_days(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
            else if (strcmp(opt_name, "dow") == 0) {
                set_freq_mins(cl->cl_option);
                set_freq_hrs(cl->cl_option);
                set_freq_days(cl->cl_option);
                set_freq_mons(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }

            /* run once an element of the selected field
             * (once an hour, once a day, etc) */
            else if (strcmp(opt_name, "hourly") == 0) {
                set_freq_hrs(cl->cl_option);
                set_freq_periodically(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
            else if (strcmp(opt_name, "daily") == 0) {
                set_freq_days(cl->cl_option);
                set_freq_periodically(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
            else if (strcmp(opt_name, "monthly") == 0) {
                set_freq_mons(cl->cl_option);
                set_freq_periodically(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
            else if (strcmp(opt_name, "weekly") == 0) {
                set_freq_dow(cl->cl_option);
                set_freq_periodically(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }

            /* run once an element of the selected field
             * from middle to middle of that field
             * (ie once from 12h to 12h the following day) */
            else if (strcmp(opt_name, "midhourly") == 0) {
                set_freq_hrs(cl->cl_option);
                set_freq_periodically(cl->cl_option);
                set_freq_mid(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
            else if (strcmp(opt_name, "middaily") == 0
                     || strcmp(opt_name, "nightly") == 0) {
                set_freq_days(cl->cl_option);
                set_freq_periodically(cl->cl_option);
                set_freq_mid(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
            else if (strcmp(opt_name, "midmonthly") == 0) {
                set_freq_mons(cl->cl_option);
                set_freq_periodically(cl->cl_option);
                set_freq_mid(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
            else if (strcmp(opt_name, "midweekly") == 0) {
                set_freq_dow(cl->cl_option);
                set_freq_periodically(cl->cl_option);
                set_freq_mid(cl->cl_option);
                if (debug_opt)
                    fprintf(stderr, "  Opt : \"%s\"\n", opt_name);
            }
        }

        else {
            fprintf(stderr, "%s:%d: Option \"%s\" unknown: "
                    "skipping option.\n", file_name, line, opt_name);
            need_correction = 1;
        }

        if (in_brackets) {
            if (*ptr != ')') {
            Handle_err}
            else
                ptr++;
        }

    } while (*ptr == ',' && ptr++);

    Skip_blanks(ptr);
    return ptr;
}


char *
get_time(char *ptr, time_t * time, int zero_allowed)
    /* convert time read in string in time_t format */
{
    time_t sum;

    *time = 0;

    while ((*ptr != ' ') && (*ptr != '\t') && (*ptr != '\0') && (*ptr != ')')) {

        sum = 0;

        while (isdigit((int)*ptr)) {
            sum *= 10;
            sum += *ptr - 48;
            ptr++;
        }

        /* interpret multipliers */
        switch (*ptr) {
        case 'm':              /* months */
            sum *= 4;
        case 'w':              /* weeks */
            sum *= 7;
        case 'd':              /* days */
            sum *= 24;
        case 'h':              /* hours */
            sum *= 3600;
        case 's':              /* seconds */
            ptr++;
            break;
        case ' ':
        case '\t':
        case ')':
            sum *= 60;          /* minutes */
            break;
        default:
            need_correction = 1;
            return NULL;
        }

        *time += sum;

    }

    Skip_blanks(ptr);
    if (*time == 0 && !zero_allowed) {
        need_correction = 1;
        return NULL;
    }
    else
        return ptr;
}


char *
check_username(char *ptr, cf_t * cf, cl_t * cl)
    /* check ptr to see if the first word is a username, returns new ptr */
{
    short int indx = 0;
    char username[USER_NAME_LEN];
    struct passwd *userpwent;

    /* check to see if next word is a username */
    /* we don't allow quotes, to be able to distinguish a user name from
     * a command line (where quotes are allowed) */
    while (isalnum((int)ptr[indx]) || ptr[indx] == '-' || ptr[indx] == '_')
        indx++;
    if (indx >= USER_NAME_LEN)
        indx = USER_NAME_LEN - 1;
    strncpy(username, ptr, indx);
    username[indx] = '\0';

    if ((userpwent = getpwnam(username)) != NULL) {
        /* found the user */
        ptr = ptr + indx;       /* move ptr to the next word */
        Skip_blanks(ptr);

        if (getuid() != rootuid) {
            fprintf(stderr, "must be privileged to run as another user : "
                    "ignoring\n");
        }
        else {
            Set(cl->cl_runas, username);
            if (debug_opt)
                fprintf(stderr, "  Opt : inline_runas %s\n", username);
        }
    }

    return ptr;
}


char *
read_word(char *ptr, char *buf, size_t buf_size)
/* copy a word to buf with a max_size of buf_size and return a pointer
 * to the next char after the word */
{
    int i = 0;

    bzero(buf, buf_size);

    while (isalnum((int)*ptr) && i < buf_size)
        buf[i++] = *ptr++;

    return ptr;
}

int
read_shortcut(char *ptr, cf_t * cf)
/* try to read a shortcut entry, and if it is one then append a line to cf
 * Return 1 if that was a shortcut entry. If it wasn't, return 0 and make sure
 * ptr is back to its orig value. */
{
    cl_t *cl = NULL;
    char shortcut[20];
    char *ptr_orig = ptr;

    cl = dups_cl(&default_line);

    /* skip the @ */
    ptr++;

    ptr = read_word(ptr, shortcut, sizeof(shortcut));

    /* time&date by default -- we'll switch to freq if @reboot */
    set_td(cl->cl_option);
    cl->cl_remain = cl->cl_runfreq;     /* FIXME: necessary?? */

    if (strcmp(shortcut, "reboot") == 0) {
        set_freq(cl->cl_option);
        set_runatreboot(cl->cl_option);
        set_runonce(cl->cl_option);
        clear_volatile(cl->cl_option);
        cl->cl_runfreq = 0;
        cl->cl_first = 0;
        /* the job will not be rescheduled after the first execution (flag is_hasrun),
         * we set timefreq to LONG_MAX just in case */
        cl->cl_timefreq = LONG_MAX;

        if (debug_opt)
            fprintf(stderr, "  Shc : @reboot\n");
    }
    else if (strcmp(shortcut, "yearly") == 0
             || strcmp(shortcut, "annually") == 0) {
        bit_set(cl->cl_mins, 0);
        bit_set(cl->cl_hrs, 0);
        bit_set(cl->cl_days, 1);
        bit_set(cl->cl_mons, 0);
        bit_nset(cl->cl_dow, 0, 7);

        if (debug_opt)
            fprintf(stderr, "  Shc : @yearly\n");
    }
    else if (strcmp(shortcut, "monthly") == 0) {
        bit_set(cl->cl_mins, 0);
        bit_set(cl->cl_hrs, 0);
        bit_set(cl->cl_days, 1);
        bit_nset(cl->cl_mons, 0, 11);
        bit_nset(cl->cl_dow, 0, 7);

        if (debug_opt)
            fprintf(stderr, "  Shc : @monthly\n");
    }
    else if (strcmp(shortcut, "weekly") == 0) {
        bit_set(cl->cl_mins, 0);
        bit_set(cl->cl_hrs, 0);
        bit_nset(cl->cl_days, 0, 31);
        bit_nset(cl->cl_mons, 0, 11);
        bit_set(cl->cl_dow, 0);
        bit_set(cl->cl_dow, 7); /* 0 and 7 are both sunday */

        if (debug_opt)
            fprintf(stderr, "  Shc : @weekly\n");
    }
    else if (strcmp(shortcut, "daily") == 0
             || strcmp(shortcut, "midnight") == 0) {
        bit_set(cl->cl_mins, 0);
        bit_set(cl->cl_hrs, 0);
        bit_nset(cl->cl_days, 0, 31);
        bit_nset(cl->cl_mons, 0, 11);
        bit_nset(cl->cl_dow, 0, 7);

        if (debug_opt)
            fprintf(stderr, "  Shc : @daily\n");
    }
    else if (strcmp(shortcut, "hourly") == 0) {
        bit_set(cl->cl_mins, 0);
        bit_nset(cl->cl_hrs, 0, 23);
        bit_nset(cl->cl_days, 0, 31);
        bit_nset(cl->cl_mons, 0, 11);
        bit_nset(cl->cl_dow, 0, 7);

        if (debug_opt)
            fprintf(stderr, "  Shc : @hourly\n");
    }
    else {
        /* this is not a shortcut line but a normal @-line:  */
        ptr = ptr_orig;
        return 0;
    }

    /* The next char must be a space (no other option allowed when using
     * a shortcut: if the user wants to use options, they should use the 
     * native fcron lines */
    if (!isspace((int)*ptr)) {
        fprintf(stderr, "%s:%d: No space after shortcut: skipping line.\n",
                file_name, line);
        goto exiterr;
    }

    /* skip spaces before the username / shell command */
    while (isspace((int)*ptr))
        ptr++;

    /* check for inline runas */
    ptr = check_username(ptr, cf, cl);

    /* get cl_shell field ( remove trailing blanks ) */
    if ((cl->cl_shell = get_string(ptr)) == NULL) {
        fprintf(stderr, "%s:%d: Mismatched quotes: skipping line.\n",
                file_name, line);
        goto exiterr;
    }
    if (strcmp(cl->cl_shell, "\0") == 0) {
        fprintf(stderr, "%s:%d: No shell command: skipping line.\n",
                file_name, line);
        Free_safe(cl->cl_shell);
        goto exiterr;
    }

#ifndef USE_SENDMAIL
    clear_mail(cl->cl_option);
    clear_mailzerolength(cl->cl_option);
#endif

    cl->cl_next = cf->cf_line_base;
    cf->cf_line_base = cl;

    if (debug_opt)
        fprintf(stderr, "  Cmd \"%s\" (shortcut)\n", cl->cl_shell);
    return 1;

 exiterr:
    Free_safe(cl);
    need_correction = 1;
    return 1;
}

void
read_freq(char *ptr, cf_t * cf)
    /* read a freq entry, and append a line to cf */
{
    cl_t *cl = NULL;

    cl = dups_cl(&default_line);

    cl->cl_first = -1;          /* 0 is a valid value, so we have to use -1 to detect unset */

    /* skip the @ */
    ptr++;

    /* get the time before first execution or the options */
    if (isdigit((int)*ptr)) {
        if ((ptr = get_time(ptr, &(cl->cl_first), 1)) == NULL) {
            fprintf(stderr, "%s:%d: Error while reading first delay:"
                    " skipping line.\n", file_name, line);
            goto exiterr;
        }

        Skip_blanks(ptr);
    }
    else if (isalnum((int)*ptr)) {
        if ((ptr = read_opt(ptr, cl)) == NULL)
            goto exiterr;
    }
    else
        Skip_blanks(ptr);

    /* we set this here, because it may be unset by read_opt (reset option) */
    cl->cl_runfreq = 0;
    set_freq(cl->cl_option);

    /* then cl_timefreq */
    if ((ptr = get_time(ptr, (time_t *) & (cl->cl_timefreq), 0)) == NULL
        || cl->cl_timefreq < 1) {
        fprintf(stderr,
                "%s:%d: Error while reading frequency %s: skipping line.\n",
                file_name, line,
                (cl->cl_timefreq < 10) ? "(lower than 10s) " : "");
        goto exiterr;
    }

    if (cl->cl_timefreq == 0) {
        fprintf(stderr, "%s:%d: no freq specified: skipping line.\n",
                file_name, line);
        goto exiterr;
    }

    if (cl->cl_first == -1)
        /* time before first execution was not specified explicitely */
        cl->cl_first = cl->cl_timefreq;

    /* check for inline runas */
    ptr = check_username(ptr, cf, cl);

    /* get cl_shell field ( remove trailing blanks ) */
    if ((cl->cl_shell = get_string(ptr)) == NULL) {
        fprintf(stderr, "%s:%d: Mismatched quotes: skipping line.\n",
                file_name, line);
        goto exiterr;
    }
    if (strcmp(cl->cl_shell, "\0") == 0) {
        fprintf(stderr, "%s:%d: No shell command: skipping line.\n",
                file_name, line);
        Free_safe(cl->cl_shell);
        goto exiterr;
    }

#ifndef USE_SENDMAIL
    clear_mail(cl->cl_option);
    clear_mailzerolength(cl->cl_option);
#endif

    cl->cl_next = cf->cf_line_base;
    cf->cf_line_base = cl;

    if (debug_opt)
        fprintf(stderr, "  Cmd \"%s\", timefreq %ld, first %ld\n",
                cl->cl_shell, cl->cl_timefreq, (long int)cl->cl_first);

    return;

 exiterr:
    free_line(cl);
    need_correction = 1;
    return;
}



#define R_field(PTR, ARY, MAX, ARYCONST, DESCRP) \
  if((PTR = read_field(PTR, ARY, MAX, ARYCONST)) == NULL) { \
      if (debug_opt) \
          fprintf(stderr, "\n"); \
      fprintf(stderr, "%s:%d: Error while reading " DESCRP " field: " \
             "skipping line.\n", file_name, line); \
      Free_safe(cl); \
      return; \
  }


void
read_arys(char *ptr, cf_t * cf)
    /* read a run freq number plus a normal fcron line */
{
    cl_t *cl = NULL;
    int i = 0;

    cl = dups_cl(&default_line);

    /* set cl_remain if not specified */
    if (*ptr == '&') {
        ptr++;
        if (isdigit((int)*ptr)) {
            if ((ptr = get_num(ptr, &i, USHRT_MAX, 0, NULL)) == NULL) {
                fprintf(stderr, "%s:%d: Error while reading runfreq:"
                        " skipping line.\n", file_name, line);
                goto exiterr;
            }
            else {
                if (i <= 1) {
                    fprintf(stderr, "%s:%d: runfreq must be 2 or more :"
                            " skipping line.\n", file_name, line);
                    goto exiterr;
                }
                cl->cl_runfreq = (unsigned short)i;
            }
        }
        else if (isalnum((int)*ptr))
            if ((ptr = read_opt(ptr, cl)) == NULL) {
                goto exiterr;
            }
        Skip_blanks(ptr);
    }

    cl->cl_remain = cl->cl_runfreq;

    /* we set this here, because it may be unset by read_opt (reset option) */
    set_td(cl->cl_option);

    if (debug_opt)
        fprintf(stderr, "     ");

    /* get the fields (check for errors) */
    R_field(ptr, cl->cl_mins, 59, NULL, "minutes");
    R_field(ptr, cl->cl_hrs, 23, NULL, "hours");
    R_field(ptr, cl->cl_days, 31, NULL, "days");
    /* month are defined by user from 1 to 12 : max is 12 */
    R_field(ptr, cl->cl_mons, 12, mons_ary, "months");
    R_field(ptr, cl->cl_dow, 7, dows_ary, "days of week");

    if (debug_opt)
        /* if debug_opt is set, we print informations in read_field function,
         *  but no end line : we print it here */
        fprintf(stderr, " remain %d\n", cl->cl_remain);

    /* check for inline runas */
    ptr = check_username(ptr, cf, cl);

    /* get the shell command (remove trailing blanks) */
    if ((cl->cl_shell = get_string(ptr)) == NULL) {
        fprintf(stderr, "%s:%d: Mismatched quotes: skipping line.\n",
                file_name, line);
        goto exiterr;
    }
    if (strcmp(cl->cl_shell, "\0") == 0) {
        fprintf(stderr, "%s:%d: No shell command: skipping line.\n",
                file_name, line);
        Free_safe(cl->cl_shell);
        goto exiterr;
    }

#ifndef USE_SENDMAIL
    clear_mail(cl->cl_option);
    clear_mailzerolength(cl->cl_option);
#endif

    cl->cl_next = cf->cf_line_base;
    cf->cf_line_base = cl;

    if (debug_opt)
        fprintf(stderr, "  Cmd \"%s\"\n", cl->cl_shell);
    return;

 exiterr:
    need_correction = 1;
    free_line(cl);
    return;

}

void
read_period(char *ptr, cf_t * cf)
    /* read a line to run periodically (i.e. once a day, once a week, etc) */
{
    cl_t *cl = NULL;
    short int remain = 8;

    cl = dups_cl(&default_line);

    /* skip the % */
    ptr++;

    /* a runfreq set to 1 means : this is a periodical line 
     * (runfreq cannot be changed by read_opt() if already set to 1) */
    cl->cl_remain = cl->cl_runfreq = 1;

    /* set cl_remain if not specified */
    if ((ptr = read_opt(ptr, cl)) == NULL) {
        goto exiterr;
    }
    Skip_blanks(ptr);

    /* we set this here, because it may be unset by read_opt (reset option) */
    set_td(cl->cl_option);

    if (debug_opt)
        fprintf(stderr, "     ");

    if (is_freq_periodically(cl->cl_option)) {
        if (is_freq_mins(cl->cl_option))
            remain = 0;
        else if (is_freq_hrs(cl->cl_option))
            remain = 1;
        else if (is_freq_days(cl->cl_option))
            remain = 2;
        else if (is_freq_mons(cl->cl_option))
            remain = 3;
        else if (is_freq_dow(cl->cl_option))
            remain = 2;
    }

    /* get the fields (check for errors) */
    if (remain-- > 0) {
    R_field(ptr, cl->cl_mins, 59, NULL, "minutes")}
    else
        bit_nset(cl->cl_mins, 0, 59);
    if (remain-- > 0) {
    R_field(ptr, cl->cl_hrs, 23, NULL, "hours")}
    else
        bit_nset(cl->cl_hrs, 0, 23);
    if (remain-- > 0) {
    R_field(ptr, cl->cl_days, 31, NULL, "days")}
    else
        bit_nset(cl->cl_days, 1, 32);
    /* month are defined by user from 1 to 12 : max is 12 */
    if (remain-- > 0) {
    R_field(ptr, cl->cl_mons, 12, mons_ary, "months")}
    else
        bit_nset(cl->cl_mons, 0, 11);
    if (remain-- > 0) {
    R_field(ptr, cl->cl_dow, 7, dows_ary, "days of week")}
    else
        bit_nset(cl->cl_dow, 0, 7);

    if (debug_opt)
        /* if debug_opt is set, we print informations in read_field function,
         *  but no end line : we print it here */
        fprintf(stderr, " remain %d\n", cl->cl_remain);

    /* check for inline runas */
    ptr = check_username(ptr, cf, cl);

    /* get the shell command (remove trailing blanks) */
    if ((cl->cl_shell = get_string(ptr)) == NULL) {
        fprintf(stderr, "%s:%d: Mismatched quotes: skipping line.\n",
                file_name, line);
        goto exiterr;
    }
    if (strcmp(cl->cl_shell, "\0") == 0) {
        fprintf(stderr, "%s:%d: No shell command: skipping line.\n",
                file_name, line);
        Free_safe(cl->cl_shell);
        goto exiterr;
    }
    else if (cl->cl_shell[0] == '*' || isdigit((int)cl->cl_shell[0]))
        fprintf(stderr, "%s:%d: Warning : shell command beginning by '%c'.\n",
                file_name, line, cl->cl_shell[0]);

    /* check for non matching if option runfreq is set to 1 */
    if (!is_freq_periodically(cl->cl_option)) {
        const size_t s_mins = 60, s_hrs = 24;
        const size_t s_days = 32, s_mons = 12;
        const size_t s_dow = 8;
        int j = 0;

        if (!is_freq_mins(cl->cl_option)) {
            bit_ffc(cl->cl_mins, s_mins, &j);
            if (j != -1 && j < s_mins)
                goto ok;
        }
        if (!is_freq_hrs(cl->cl_option)) {
            bit_ffc(cl->cl_hrs, s_hrs, &j);
            if (j != -1 && j < s_hrs)
                goto ok;
        }
        if (!is_freq_days(cl->cl_option)) {
            bit_ffc(cl->cl_days, s_days, &j);
            if (j != -1 && j < s_days) {
                if (is_dayand(cl->cl_option))
                    goto ok;
                else {
                    if (!is_freq_dow(cl->cl_option)) {
                        bit_ffc(cl->cl_dow, s_dow, &j);
                        if (j != -1 && j < s_dow)
                            goto ok;
                    }
                }
            }
        }
        if (!is_freq_mons(cl->cl_option)) {
            bit_ffc(cl->cl_mons, s_mons, &j);
            if (j != -1 && j < s_mons)
                goto ok;
        }
        if (!is_freq_dow(cl->cl_option)) {
            bit_ffc(cl->cl_dow, s_dow, &j);
            if (j != -1 && j < s_dow && is_dayand(cl->cl_option))
                goto ok;
        }

        fprintf(stderr, "%s:%d: periodical line with no intervals: "
                "skipping line.\n", file_name, line);
        goto exiterr;
    }

 ok:
#ifndef USE_SENDMAIL
    clear_mail(cl->cl_option);
    clear_mailzerolength(cl->cl_option);
#endif

    cl->cl_next = cf->cf_line_base;
    cf->cf_line_base = cl;

    if (debug_opt)
        fprintf(stderr, "  Cmd \"%s\"\n", cl->cl_shell);
    return;

 exiterr:
    need_correction = 1;
    free_line(cl);
    return;

}

char *
get_num(char *ptr, int *num, int max, short int decimal, const char **names)
    /* read a string's number and return it under int format.
     *  Also check if that number is less than max */
{
    int i = 0;
    *num = 0;

    if (isalpha((int)*ptr)) {

        if (names == NULL) {
            need_correction = 1;
            return NULL;
        }

        /* set string to lower case */
        for (i = 0; i < strlen(names[0]); i++)
            *(ptr + i) = tolower(*(ptr + i));

        for (i = 0; names[i]; ++i)
            if (strncmp(ptr, names[i], strlen(names[i])) == 0) {
                *num = i;
                ptr += strlen(names[i]);
                return ptr;
                break;
            }

        /* string is not in name list */
        need_correction = 1;
        return NULL;

    }
    else {

        while (isdigit((int)*ptr) || *ptr == '.') {

            if (*ptr == '.' && ptr++ && i++ > 0)
                return NULL;
            if (i > 0 && --decimal < 0) {
                /* the decimal number is exceeded : we round off,
                 * skip the other decimals and return */
                if (*ptr >= '5')
                    *num += 1;
                while (isdigit((int)*(++ptr))) ;
                ptr--;
            }
            else {
                *num *= 10;
                *num += *ptr - 48;
            }

            if (*num > max) {
                need_correction = 1;
                return NULL;
            }

            ptr++;

        }

        if (decimal > 0)
            *num *= 10 * decimal;

    }

    return ptr;
}


char *
read_field(char *ptr, bitstr_t * ary, int max, const char **names)
    /* read a field like "2,5-8,10-20/2,21-30~25" and fill ary */
{
    int start = 0;
    int stop = 0;
    int step = 0;
    int rm = 0;
    int i = 0;

    while ((*ptr != ' ') && (*ptr != '\t') && (*ptr != '\0')) {

        start = stop = step = 0;

        /* there may be a "," */
        if (*ptr == ',')
            ptr++;

        if (*ptr == '*') {
            /* we have to fill everything (may be modified by a step ) */
            start = 0;
            /* user set month from 1 to 12, but we manage it internally
             * as a number from 0 to 11 */
            stop = (max == 12) ? 11 : max;
            ptr++;
        }
        else {

            ptr = get_num(ptr, &start, max, 0, names);
            if (ptr == NULL)
                return NULL;
            if (max == 12)
                /* this number is part of the month field.
                 * user set it from 1 to 12, but we manage it internally
                 * as a number from 0 to 11 : we remove 1 to start */
                start -= 1;

            if (*ptr == ',' || *ptr == ' ' || *ptr == '\t') {
                /* this is a single number : set up array and continue */
                if (debug_opt)
                    fprintf(stderr, " %d", start);
                bit_set(ary, start);
                continue;
            }

            /* check for a dash */
            else if (*ptr == '-') {
                ptr++;
                ptr = get_num(ptr, &stop, max, 0, names);
                if (ptr == NULL)
                    /* we reached the end of the string to parse */
                    return NULL;
                if (max == 12)
                    /* this number is part of the month field.
                     * user set it from 1 to 12, but we manage it internally
                     * as a number from 0 to 11 : we remove 1 to stop */
                    stop -= 1;
            }
            else {
                /* syntax error */
                need_correction = 1;
                return NULL;
            }
        }

        /* check for step size */
        if (*ptr == '/') {
            ptr++;
            if ((ptr = get_num(ptr, &step, max, 0, names)) == NULL || step == 0)
                return NULL;
        }
        else
            /* step undefined : default is 0 */
            step = 1;

        /* fill array */
        if (debug_opt)
            fprintf(stderr, " %d-%d/%d", start, stop, step);

        if (start < stop)
            for (i = start; i <= stop; i += step)
                bit_set(ary, i);
        else {
            short int field_max = (max == 12) ? 11 : max;
            /* this is a field like (for hours field) "22-3" :
             * we should set from 22 to 3 (not from 3 to 22 or nothing :)) ) */
            for (i = start; i <= field_max; i += step)
                bit_set(ary, i);
            for (i -= (field_max + 1); i <= stop; i += step)
                bit_set(ary, i);
        }

        /* finally, remove unwanted values */
        while (*ptr == '~') {
            ptr++;
            rm = 0;
            if ((ptr = get_num(ptr, &rm, max, 0, names)) == NULL)
                return NULL;
            if (max == 12)
                /* this number is part of the month field.
                 * user set it from 1 to 12, but we manage it internally
                 * as a number from 0 to 11 : we remove 1 to rm */
                rm -= 1;

            if (debug_opt)
                fprintf(stderr, " ~%d", rm);
            bit_clear(ary, rm);

            /* if we remove one value of Sunday, remove the other */
            if (max == 7 && rm == 0) {
                bit_clear(ary, 7);
                if (debug_opt)
                    fprintf(stderr, " ~%d", 7);
            }
            else if (max == 7 && rm == 7) {
                bit_clear(ary, 0);
                if (debug_opt)
                    fprintf(stderr, " ~%d", 0);
            }

        }

    }

    /* Sunday is both 0 and 7 : if one is set, set the other */
    if (max == 7) {
        if (bit_test(ary, 0))
            bit_set(ary, 7);
        else if (bit_test(ary, 7))
            bit_set(ary, 0);
    }

    Skip_blanks(ptr);

    if (debug_opt)
        fprintf(stderr, " #");

    return ptr;
}

void
delete_file(const char *user_name)
    /* free a file if user_name is not null 
     *   otherwise free all files */
{
    cf_t *file = NULL;
    cf_t *prev_file = NULL;
    cl_t *line = NULL;
    cl_t *cur_line = NULL;

    file = file_base;
    while (file != NULL) {
        if (strcmp(user_name, file->cf_user) == 0) {

            /* free lines */
            cur_line = file->cf_line_base;
            while ((line = cur_line) != NULL) {
                cur_line = line->cl_next;
                free_line(line);
            }
            break;

        }
        else {
            prev_file = file;
            file = file->cf_next;
        }
    }

    if (file == NULL)
        /* file not in list */
        return;

    /* remove file from list */
    if (prev_file == NULL)
        file_base = file->cf_next;
    else
        prev_file->cf_next = file->cf_next;

    /* free env variables */
    env_list_destroy(file->cf_env_list);

    /* finally free file itself */
    Free_safe(file->cf_user);
    Free_safe(file);

}

int
save_file(char *path)
    /* Store the informations relatives to the executions
     * of tasks at a defined frequency of system's running time */
{
    cf_t *file = NULL;

    if (debug_opt)
        fprintf(stderr, "Saving ...\n");

    for (file = file_base; file; file = file->cf_next) {

        /* save_file() is run under user's rights.
         * If fcrontab is run by root for a normal user, we must change the file's
         * ownership to this user, in order to make fcron check the runas fields.
         * (a malicious user could put a runas(root) and wait for the fcrontab to be
         * installed by root) */
#ifdef USE_SETE_ID
        if (save_file_safe(file, path, "fcrontab", asuid, fcrontab_gid, 0) ==
            ERR)
            return ERR;
#else
        if (save_file_safe
            (file, path, "fcrontab", fcrontab_uid, fcrontab_gid, 0) == ERR)
            return ERR;
#endif

    }

    return OK;
}
