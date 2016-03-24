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
 * The goal of this program is simple : giving a user interface to fcron
 * daemon, by allowing each user to see, modify, append or remove his 
 * fcrontabs. 
 * Fcron daemon use a specific formated format of file, so fcrontab generate
 * that kind of file from human readable files. In order allowing users to
 * see and modify their fcrontabs, the source file is always saved with the
 * formated one. 
 * Fcrontab makes a temporary formated file, and then sends a signal 
 * to the daemon to force it to update its configuration, remove the temp
 * file and save a new and final formated file.
 * That way, not the simple, allows the daemon to keep a maximum of 
 * informations like the time remaining before next execution, or the date
 * and hour of next execution.
 */

#include "fcrontab.h"

#include "allow.h"
#include "fileconf.h"
#include "temp_file.h"
#include "read_string.h"


void info(void);
void usage(void);


/* used in temp_file() */
char *tmp_path = "/tmp/";

/* command line options */
char rm_opt = 0;
char list_opt = 0;
char edit_opt = 0;
char reinstall_opt = 0;
char ignore_prev = 0;
int file_opt = 0;

/* uid/gid of users/groups 
 * (we don't use the static UID or GID as we ask for user and group names
 * in the configure script) */
char *user = NULL;
char *runas = NULL;
uid_t useruid = 0;              /* uid of the user */
gid_t usergid = 0;              /* gid of the user */
uid_t asuid = 0;                /* uid of the user whose fcrontab we are working on */
gid_t asgid = 0;                /* gid of the user whose fcrontab we are working on */
uid_t fcrontab_uid = 0;         /* uid of the fcron user */
gid_t fcrontab_gid = 0;         /* gid of the fcron user */
uid_t rootuid = 0;              /* uid of root */
gid_t rootgid = 0;              /* gid of root */

char need_sig = 0;              /* do we need to signal fcron daemon */

char orig_dir[PATH_LEN];
cf_t *file_base = NULL;
char buf[PATH_LEN];
char file[PATH_LEN];

/* needed by log part : */
char *prog_name = NULL;
char foreground = 1;
pid_t daemon_pid = 0;

#ifdef HAVE_LIBPAM
int conv_pam(int num_msg, const struct pam_message **msgm,
             struct pam_response **response, void *appdata_ptr);
pam_handle_t *pamh = NULL;
const struct pam_conv apamconv = { conv_pam, NULL };
#endif                          /* HAVE_LIBPAM */

void
info(void)
    /* print some informations about this program :
     * version, license */
{
    fprintf(stderr,
            "fcrontab " VERSION_QUOTED " - user interface to daemon fcron\n"
            "Copyright " COPYRIGHT_QUOTED " Thibault Godouet <fcron@free.fr>\n"
            "This program is free software distributed WITHOUT ANY WARRANTY.\n"
            "See the GNU General Public License for more details.\n");

    exit(EXIT_OK);

}


void
usage(void)
  /*  print a help message about command line options and exit */
{
    fprintf(stderr,
            "fcrontab [-n] file [user|-u user]\n"
            "fcrontab { -l | -r | -e | -z } [-n] [user|-u user]\n"
            "fcrontab -h\n"
            "  -u user    specify user name.\n"
            "  -l         list user's current fcrontab.\n"
            "  -r         remove user's current fcrontab.\n"
            "  -e         edit user's current fcrontab.\n"
            "  -z         reinstall user's fcrontab from source code.\n"
            "  -n         ignore previous version of file.\n"
            "  -c f       make fcrontab use config file f.\n"
            "  -d         set up debug mode.\n"
            "  -h         display this help message.\n"
            "  -V         display version & infos about fcrontab.\n" "\n");

    exit(EXIT_ERR);
}


void
xexit(int exit_val)
    /* launch signal if needed and exit */
{
    pid_t pid = 0;

    /* WARNING: make sure we never call die_e() or something that could call
     *          die_e() here, as die_e() would then call xexit() and we could
     *          go into a loop! */

    if (need_sig == 1) {

        /* fork and exec fcronsighup */
        switch (pid = fork()) {
        case 0:
            /* child */
            if (getegid() != fcrontab_gid && setegid(fcrontab_gid) != 0) {
                error_e("could not change egid to fcrontab_gid[%d]",
                        fcrontab_gid);
                exit(ERR);
            }
            execl(BINDIREX "/fcronsighup", BINDIREX "/fcronsighup", fcronconf,
                  NULL);

            error_e("Could not exec " BINDIREX " fcronsighup");
            exit(ERR);
            break;

        case -1:
            error_e("Could not fork (fcron has not been signaled)");
            exit(ERR);
            break;

        default:
            /* parent */
            waitpid(pid, NULL, 0);
            break;
        }
    }

#ifdef HAVE_LIBPAM
    /* we need those rights for pam to close properly */
    if (geteuid() != fcrontab_uid && seteuid(fcrontab_uid) != 0)
        die_e("could not change euid to %d", fcrontab_uid);
    if (getegid() != fcrontab_gid && setegid(fcrontab_gid) != 0)
        die_e("could not change egid to %d", fcrontab_gid);
    pam_setcred(pamh, PAM_DELETE_CRED | PAM_SILENT);
    pam_end(pamh, pam_close_session(pamh, PAM_SILENT));
#endif

    exit(exit_val);

}

int
copy_src(int from, const char *dest)
    /* copy src file orig (already opened) to dest */
    /* we first copy the file to a temp file name, and then we rename it,
     * so as to avoid data loss if the filesystem is full. */
{
    int to_fd = -1;
    int nb;
    char *copy_buf[LINE_LEN];

    char tmp_filename_str[PATH_LEN + 4];
    int dest_path_len, tmp_filename_index;
    char *tmp_suffix_str = ".tmp";
    int max_dest_len = sizeof(tmp_filename_str) - sizeof(tmp_suffix_str);

    if (from < 0) {
        die("copy_src() called with an invalid 'from' argument");
    }

    /* just in case the file was read in the past... */
    lseek(from, 0, SEEK_SET);

    /* the temp file must be in the same directory as the dest file */
    dest_path_len = strlen(dest);
    strncpy(tmp_filename_str, dest, max_dest_len);
    tmp_filename_index = (dest_path_len > max_dest_len) ?
        max_dest_len : dest_path_len;
    strcpy(&tmp_filename_str[tmp_filename_index], tmp_suffix_str);

    /* create it as fcrontab_uid (to avoid problem if user's uid changed)
     * except for root. Root requires filesystem uid root for security
     * reasons */
    to_fd =
        open_as_user(tmp_filename_str,
                     (asuid == rootuid) ? rootuid : fcrontab_uid, fcrontab_gid,
                     O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
                     S_IRUSR | S_IWUSR | S_IRGRP);
    if (to_fd < 0) {
        error_e("could not open file %s", tmp_filename_str);
        goto exiterr;
    }

    if (asuid == rootuid) {
        if (fchmod(to_fd, S_IWUSR | S_IRUSR) != 0) {
            error_e("Could not fchmod %s to 600", tmp_filename_str);
            goto exiterr;
        }
        if (fchown(to_fd, rootuid, fcrontab_gid) != 0) {
            error_e("Could not fchown %s to root", tmp_filename_str);
            goto exiterr;
        }
    }

    while ((nb = read(from, copy_buf, sizeof(copy_buf))) != -1 && nb != 0)
        if (write(to_fd, copy_buf, nb) != nb) {
            error_e("Error while copying file (no space left ?)."
                    " Aborting : old source file kept");
            goto exiterr;
        }

    xclose_check(&to_fd, dest);

    if (rename_as_user(tmp_filename_str, dest, useruid, fcrontab_gid) < 0) {
        error_e("Unable to rename %s to %s : old source file kept",
                tmp_filename_str, dest);
        goto exiterr;
    }

    return OK;

 exiterr:
    if (to_fd != -1)
        xclose_check(&to_fd, dest);
    return ERR;
}


int
remove_fcrontab(char rm_orig)
    /* remove user's fcrontab and tell daemon to update his conf */
    /* note : the binary fcrontab is removed by fcron */
{
    int return_val = OK;
    int fd;

    if (rm_orig)
        explain("removing %s's fcrontab", user);

    /* remove source and formated file */
    if ((rm_orig && remove_as_user(buf, fcrontab_uid, fcrontab_gid)) != 0) {
        if (errno == ENOENT)
            return_val = ENOENT;
        else
            error_e("could not remove %s", buf);
    }

    /* try to remove the temp file in case he has not
     * been read by fcron daemon */
    snprintf(buf, sizeof(buf), "new.%s", user);
    remove_as_user(buf, useruid, fcrontab_gid);

    /* finally create a file in order to tell the daemon
     * a file was removed, and launch a signal to daemon */
    snprintf(buf, sizeof(buf), "rm.%s", user);
    fd = open_as_user(buf, fcrontab_uid, fcrontab_gid,
                      O_CREAT | O_TRUNC | O_EXCL, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        if (errno != EEXIST)
            error_e("Can't create file %s", buf);
    }
    else if (asuid == rootuid && fchown(fd, rootuid, fcrontab_gid) != 0)
        error_e("Could not fchown %s to root", buf);
    xclose_check(&fd, buf);

    need_sig = 1;

    return return_val;

}


int
write_file(int fd)
{
    int return_val = OK;

    if (ignore_prev == 1)
        /* if user wants to ignore previous version, we remove it *
         * ( fcron daemon remove files no longer wanted before
         *   adding new ones ) */
        remove_fcrontab(0);

    /* copy original file to fcrontabs dir */
    snprintf(buf, sizeof(buf), "%s.orig", user);
    if (copy_src(fd, buf) == ERR) {
        return_val = ERR;
    }
    else {

        if (file_base->cf_line_base == NULL) {
            /* no entries */
            explain("%s's fcrontab contains no entries : removed.", user);
            remove_fcrontab(0);
        }
        else {
            /* write the binary fcrontab on disk */
            snprintf(buf, sizeof(buf), "new.%s", user);
            if (save_file(buf) != OK)
                return_val = ERR;
        }

    }

    return return_val;
}

int
make_file(char *file, int fd)
{
    explain("installing file %s for user %s", file, user);

    /* read file and create a list in memory */
    switch (read_file(file, fd)) {
    case 2:
    case OK:

        if (write_file(fd) == ERR)
            return ERR;
        else
            /* tell daemon to update the conf */
            need_sig = 1;

        /* free memory used to store the list */
        delete_file(user);

        break;

    case ERR:
        return ERR;
    }

    return OK;

}


void
list_file(const char *file)
{
    FILE *f = NULL;
    int c;
    int fd = -1;

    explain("listing %s's fcrontab", user);

    fd = open_as_user(file, useruid, fcrontab_gid, O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT) {
            explain("user %s has no fcrontab.", user);
            return;
        }
        else
            die_e("User %s could not read file \"%s\"", user, file);
    }

    f = fdopen(fd, "r");
    if (f == NULL) {
        xclose_check(&fd, file);
        die_e("User %s could not read file \"%s\"", user, file);
    }

    while ((c = getc(f)) != EOF)
        putchar(c);

    /* also closes the underlying file descriptor fd: */
    xfclose_check(&f, file);

}

void
edit_file(const char *fcron_orig)
    /* copy file to a temp file, edit that file, and install it
     * if necessary */
{
    char *cureditor = NULL;
    char editorcmd[PATH_LEN];
    pid_t pid;
    int status;
    struct stat st;
    time_t mtime = 0;
    char *tmp_str = NULL;
    FILE *f = NULL, *fi = NULL;
    int file = -1, origfd = -1;
    int c;
    char correction = 0;
    short return_val = EXIT_OK;

    explain("fcrontab : editing %s's fcrontab", user);

    if ((cureditor = getenv("VISUAL")) == NULL || strcmp(cureditor, "\0") == 0)
        if ((cureditor = getenv("EDITOR")) == NULL
            || strcmp(cureditor, "\0") == 0)
            cureditor = editor;

    /* temp_file() dies on error, so tmp_str is always set */
    file = temp_file(&tmp_str);
    if ((fi = fdopen(file, "w")) == NULL) {
        error_e("could not fdopen");
        goto exiterr;
    }
#ifndef USE_SETE_ID
    if (fchown(file, asuid, asgid) != 0) {
        error_e("Could not fchown %s to asuid and asgid", tmp_str);
        goto exiterr;
    }
#endif
    /* copy user's fcrontab (if any) to a temp file */
    origfd = open_as_user(fcron_orig, useruid, fcrontab_gid, O_RDONLY);
    if (origfd < 0) {
        if (errno != ENOENT) {
            error_e("could not open file %s", fcron_orig);
            goto exiterr;
        }
        else
            fprintf(stderr, "no fcrontab for %s - using an empty one\n", user);
    }
    else {
        f = fdopen(origfd, "r");
        if (f == NULL) {
            error_e("could not fdopen file %s", fcron_orig);
            goto exiterr;
        }
        /* copy original file to temp file */
        while ((c = getc(f)) != EOF) {
            if (putc(c, fi) == EOF) {
                error_e("could not write to file %s", tmp_str);
                goto exiterr;
            }
        }
        xfclose_check(&f, fcron_orig);

        if (ferror(fi))
            error_e("Error while writing new fcrontab to %s");
    }

    /* Don't close fi, because we still need the file descriptor 'file' */
    if (fflush(fi) != 0)
        die_e("Could not fflush(%s)", fi);
    fi = NULL;

    do {

        if (fstat(file, &st) == 0)
            mtime = st.st_mtime;
        else {
            error_e("could not stat \"%s\"", tmp_str);
            goto exiterr;
        }

#ifndef USE_SETE_ID
        /* chown the file (back if correction) to asuid/asgid so as user can edit it */
        if (fchown(file, asuid, asgid) != 0
            || fchmod(file, S_IRUSR | S_IWUSR) != 0) {
            fprintf(stderr, "Can't chown or chmod %s.\n", tmp_str);
            goto exiterr;
        }
#endif
        /* close the file before the user edits it */
        xclose_check(&file, tmp_str);

        switch (pid = fork()) {
        case 0:
            /* child */
            if (useruid != rootuid) {
                if (setgid(asgid) < 0) {
                    error_e("setgid(asgid)");
                    goto exiterr;
                }
                if (setuid(asuid) < 0) {
                    error_e("setuid(asuid)");
                    goto exiterr;
                }
            }
            else {
                /* Some programs, like perl, require gid=egid : */
                if (setgid(getgid()) < 0) {
                    error_e("setgid(getgid())");
                    goto exiterr;
                }
            }
            snprintf(editorcmd, sizeof(editorcmd), "%s %s", cureditor, tmp_str);
            if (chdir(tmp_path) != 0)
                error_e("Could not chdir to %s", tmp_path);
            execlp(shell, shell, "-c", editorcmd, tmp_str, NULL);
            error_e("Error while running \"%s\"", cureditor);
            goto exiterr;

        case -1:
            error_e("fork");
            goto exiterr;

        default:
            /* parent */
            break;
        }

        /* only reached by parent */
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status)) {
            fprintf(stderr,
                    "Editor exited abnormally:" " fcrontab is unchanged.\n");
            goto exiterr;
        }

        /* re-open the file that has just been edited */
        file = open_as_user(tmp_str, useruid, usergid, O_RDONLY);
        if (file < 0) {
            error_e("Could not open file %s", tmp_str);
            goto exiterr;
        }

#ifndef USE_SETE_ID
        /* chown the file back to rootuid/rootgid */
        if (fchown(file, rootuid, rootgid) != 0
            || fchmod(file, S_IRUSR | S_IWUSR) != 0) {
            fprintf(stderr, "Can't chown or chmod %s.\n", tmp_str);
            goto exiterr;
        }
#endif

        /* check if file has been modified */
        if (fstat(file, &st) != 0) {
            error_e("could not stat %s", tmp_str);
            goto exiterr;
        }

        else if (st.st_mtime > mtime || correction == 1) {

            correction = 0;

            switch (read_file(tmp_str, file)) {
            case ERR:
                goto exiterr;
            case 2:
                fprintf(stderr, "\nFile contains some errors. "
                        "Ignore [i] or Correct [c] ? ");
                while ((c = getchar())) {
                    /* consume the rest of the line, e.g. the newline char (\n) */
                    while (c != '\n' && (getchar() != '\n')) ;

                    if (c == 'i') {
                        break;
                    }
                    else if (c == 'c') {
                        /* free memory used to store the list */
                        delete_file(user);
                        correction = 1;
                        break;
                    }
                    else {
                        fprintf(stderr,
                                "Please press c to correct, "
                                "or i to ignore: ");
                    }
                }
                break;
            default:
                break;
            }

        }
        else {
            fprintf(stderr,
                    "Fcrontab is unchanged :" " no need to install it.\n");
            goto end;
        }

    } while (correction == 1);

    if (write_file(file) != OK)
        return_val = EXIT_ERR;
    else
        /* tell daemon to update the conf */
        need_sig = 1;


    /* free memory used to store the list */
    delete_file(user);

 end:
    xclose_check(&file, tmp_str);
    if (remove_as_user(tmp_str, useruid, fcrontab_gid) != 0)
        error_e("could not remove %s", tmp_str);
    Free_safe(tmp_str);
    xexit(return_val);

 exiterr:
    xfclose_check(&fi, tmp_str);
    xclose_check(&file, tmp_str);
    if (remove_as_user(tmp_str, useruid, fcrontab_gid) != 0)
        error_e("could not remove %s", tmp_str);
    xfclose_check(&f, fcron_orig);
    Free_safe(tmp_str);
    xexit(EXIT_ERR);
}


int
install_stdin(void)
    /* install what we get through stdin */
{
    int tmp_fd = 0;
    FILE *tmp_file = NULL;
    char *tmp_str = NULL;
    int c;
    short return_val = EXIT_OK;

    tmp_fd = temp_file(&tmp_str);

    if ((tmp_file = fdopen(tmp_fd, "w")) == NULL)
        die_e("Could not fdopen file %s", tmp_str);

    while ((c = getc(stdin)) != EOF)
        putc(c, tmp_file);
    /* // */
    debug("Copied stdin to %s, about to parse file %s...", tmp_str, tmp_str);

    /* don't closes tmp_fd as it will be used for make_file(): */
    if (fflush(tmp_file) != 0)
        die_e("Could not fflush(%s)", tmp_file);

    if (make_file(tmp_str, tmp_fd) == ERR)
        goto exiterr;
    else
        goto exit;

 exiterr:
    return_val = EXIT_ERR;
 exit:
    if (remove(tmp_str) != 0)
        error_e("Could not remove %s", tmp_str);
    free(tmp_str);
    return return_val;

}

void
reinstall(char *fcron_orig)
{
    int i = -1;

    explain("reinstalling %s's fcrontab", user);

    if ((i = open_as_user(fcron_orig, useruid, fcrontab_gid, O_RDONLY)) < 0) {
        if (errno == ENOENT) {
            fprintf(stderr, "Could not reinstall: user %s has no fcrontab\n",
                    user);
        }
        else
            fprintf(stderr, "Could not open \"%s\": %s\n", fcron_orig,
                    strerror(errno));

        xexit(EXIT_ERR);
    }

    close(0);
    dup2(i, 0);
    xclose(&i);

    xexit(install_stdin());

}


#ifdef HAVE_LIBPAM
int
conv_pam(int num_msg, const struct pam_message **msgm,
         struct pam_response **response, void *appdata_ptr)
    /* text based conversation for pam. */
{
    int count = 0;
    struct pam_response *reply;

    if (num_msg <= 0)
        return PAM_CONV_ERR;

    reply = (struct pam_response *)calloc(num_msg, sizeof(struct pam_response));
    if (reply == NULL) {
        debug("no memory for responses");
        return PAM_CONV_ERR;
    }

    for (count = 0; count < num_msg; ++count) {
        char *string = NULL;

        switch (msgm[count]->msg_style) {
        case PAM_PROMPT_ECHO_OFF:
            string = read_string(CONV_ECHO_OFF, msgm[count]->msg);
            if (string == NULL) {
                goto failed_conversation;
            }
            break;
        case PAM_PROMPT_ECHO_ON:
            string = read_string(CONV_ECHO_ON, msgm[count]->msg);
            if (string == NULL) {
                goto failed_conversation;
            }
            break;
        case PAM_ERROR_MSG:
            if (fprintf(stderr, "%s\n", msgm[count]->msg) < 0) {
                goto failed_conversation;
            }
            break;
        case PAM_TEXT_INFO:
            if (fprintf(stdout, "%s\n", msgm[count]->msg) < 0) {
                goto failed_conversation;
            }
            break;
        default:
            fprintf(stderr, "erroneous conversation (%d)\n",
                    msgm[count]->msg_style);
            goto failed_conversation;
        }

        if (string) {           /* must add to reply array */
            /* add string to list of responses */

            reply[count].resp_retcode = 0;
            reply[count].resp = string;
            string = NULL;
        }
    }

    /* New (0.59+) behavior is to always have a reply - this is
     * compatable with the X/Open (March 1997) spec. */
    *response = reply;
    reply = NULL;

    return PAM_SUCCESS;

 failed_conversation:

    if (reply) {
        for (count = 0; count < num_msg; ++count) {
            if (reply[count].resp == NULL) {
                continue;
            }
            switch (msgm[count]->msg_style) {
            case PAM_PROMPT_ECHO_ON:
            case PAM_PROMPT_ECHO_OFF:
                Overwrite(reply[count].resp);
                free(reply[count].resp);
                break;
            case PAM_ERROR_MSG:
            case PAM_TEXT_INFO:
                /* should not actually be able to get here... */
                free(reply[count].resp);
            }
            reply[count].resp = NULL;
        }
        /* forget reply too */
        free(reply);
        reply = NULL;
    }

    return PAM_CONV_ERR;
}
#endif                          /* HAVE_LIBPAM */


void
parseopt(int argc, char *argv[])
  /* set options */
{

    int c;
    extern char *optarg;
    extern int optind, opterr, optopt;
    struct passwd *pass;
#ifdef SYSFCRONTAB
    char is_sysfcrontab = 0;
#endif

    /* constants and variables defined by command line */

    while (1) {
        c = getopt(argc, argv, "u:lrezdnhVc:");
        if (c == EOF)
            break;
        switch (c) {

        case 'V':
            info();
            break;

        case 'h':
            usage();
            break;

        case 'u':
            if (useruid != rootuid) {
                fprintf(stderr, "must be privileged to use -u\n");
                xexit(EXIT_ERR);
            }
            user = strdup2(optarg);
            break;

        case 'd':
            debug_opt = 1;
            break;

        case 'l':
            if (rm_opt || edit_opt || reinstall_opt) {
                fprintf(stderr, "Only one of the options -l, -r, -e and -z"
                        "may be used simultaneously.\n");
                xexit(EXIT_ERR);
            }
            list_opt = 1;
            rm_opt = edit_opt = reinstall_opt = 0;
            break;

        case 'r':
            if (list_opt || edit_opt || reinstall_opt) {
                fprintf(stderr, "Only one of the options -l, -r, -e and -z"
                        "may be used simultaneously.\n");
                xexit(EXIT_ERR);
            }
            rm_opt = 1;
            list_opt = edit_opt = reinstall_opt = 0;
            break;

        case 'e':
            if (list_opt || rm_opt || reinstall_opt) {
                fprintf(stderr, "Only one of the options -l, -r, -e and -z"
                        "may be used simultaneously.\n");
                xexit(EXIT_ERR);
            }
            edit_opt = 1;
            list_opt = rm_opt = reinstall_opt = 0;
            break;

        case 'z':
            if (list_opt || rm_opt || edit_opt) {
                fprintf(stderr, "Only one of the options -l, -r, -e and -z"
                        "may be used simultaneously.\n");
                xexit(EXIT_ERR);
            }
            reinstall_opt = ignore_prev = 1;
            list_opt = rm_opt = edit_opt = 0;
            break;

        case 'n':
            ignore_prev = 1;
            break;

        case 'c':
            if (optarg[0] == '/') {
                Set(fcronconf, optarg);
            }
            else {
                char buf[PATH_LEN];
                snprintf(buf, sizeof(buf), "%s/%s", orig_dir, optarg);
                Set(fcronconf, buf);
            }
            break;

        case ':':
            fprintf(stderr, "(setopt) Missing parameter.\n");
            usage();

        case '?':
            usage();

        default:
            fprintf(stderr, "(setopt) Warning: getopt returned %c.\n", c);
        }
    }

    /* read fcron.conf and update global parameters */
    read_conf();

    /* read the file name and/or user and check validity of the arguments */
    if (argc - optind > 2)
        usage();
    else if (argc - optind == 2) {
        if (list_opt + rm_opt + edit_opt + reinstall_opt == 0)
            file_opt = optind++;
        else
            usage();

        if (useruid != rootuid) {
            fprintf(stderr, "must be privileged to use -u\n");
            xexit(EXIT_ERR);
        }
        Set(user, argv[optind]);
    }
    else if (argc - optind == 1) {
        if (list_opt + rm_opt + edit_opt + reinstall_opt == 0)
            file_opt = optind;
        else {
            if (useruid != rootuid) {
                fprintf(stderr, "must be privileged to use [user|-u user]\n");
                xexit(EXIT_ERR);
            }
            Set(user, argv[optind]);
        }
    }
    else if (list_opt + rm_opt + edit_opt + reinstall_opt != 1)
        usage();

    if (user == NULL) {
        /* get user's name using getpwuid() */
        if (!(pass = getpwuid(useruid)))
            die_e("user \"%s\" is not in passwd file. Aborting.", USERNAME);
        /* we need to strdup2 the value given by getpwuid() because we free
         * file->cf_user in delete_file */
        user = strdup2(pass->pw_name);
        asuid = pass->pw_uid;
        asgid = pass->pw_gid;
    }
    else {
#ifdef SYSFCRONTAB
        if (strcmp(user, SYSFCRONTAB) == 0) {
            is_sysfcrontab = 1;
            asuid = rootuid;
            asgid = rootgid;
        }
        else
#endif                          /* def SYSFCRONTAB */
        {
            errno = 0;
            if ((pass = getpwnam(user))) {
                asuid = pass->pw_uid;
                asgid = pass->pw_gid;
            }
            else
                die_e("user \"%s\" is not in passwd file. Aborting.", user);
        }
    }

    if (
#ifdef SYSFCRONTAB
           !is_sysfcrontab &&
#endif
           !is_allowed(user)) {
        die("User \"%s\" is not allowed to use %s. Aborting.", user, prog_name);
    }

#ifdef SYSFCRONTAB
    if (is_sysfcrontab)
        runas = ROOTNAME;
    else
#endif
        runas = user;

}


int
main(int argc, char **argv)
{

#ifdef HAVE_LIBPAM
    int retcode = 0;
    const char *const *env;
#endif
    struct passwd *pass;

    rootuid = get_user_uid_safe(ROOTNAME);
    rootgid = get_group_gid_safe(ROOTGROUP);

    memset(buf, 0, sizeof(buf));
    memset(file, 0, sizeof(file));

    if (strrchr(argv[0], '/') == NULL)
        prog_name = argv[0];
    else
        prog_name = strrchr(argv[0], '/') + 1;

    useruid = getuid();
    usergid = getgid();

#ifdef USE_SETE_ID
    /* drop any suid privilege (that we use to write files) but keep sgid
     * one for now: we need it for read_conf() and is_allowed() */
    seteuid_safe(useruid);
#endif

    errno = 0;
    if (!(pass = getpwnam(USERNAME)))
        die_e("user \"%s\" is not in passwd file. Aborting.", USERNAME);
    fcrontab_uid = pass->pw_uid;
    fcrontab_gid = pass->pw_gid;

    /* get current dir */
    orig_dir[0] = '\0';
    if (getcwd(orig_dir, sizeof(orig_dir)) == NULL)
        die_e("getcwd");

    /* interpret command line options */
    parseopt(argc, argv);

#ifdef USE_SETE_ID
    /* drop any privilege we may have: we will only get them back
     * temporarily every time we need it. */
    seteuid_safe(useruid);
    setegid_safe(usergid);
#endif

#ifdef HAVE_LIBPAM
    /* Open PAM session for the user and obtain any security
     * credentials we might need */

    debug("username: %s, runas: %s", user, runas);
    retcode = pam_start("fcrontab", runas, &apamconv, &pamh);
    if (retcode != PAM_SUCCESS)
        die_pame(pamh, retcode, "Could not start PAM");
    retcode = pam_authenticate(pamh, 0);        /* is user really user? */
    if (retcode != PAM_SUCCESS)
        die_pame(pamh, retcode, "Could not authenticate user using PAM (%d)",
                 retcode);
    retcode = pam_acct_mgmt(pamh, 0);   /* permitted access? */
    if (retcode != PAM_SUCCESS)
        die_pame(pamh, retcode, "Could not init PAM account management (%d)",
                 retcode);
    retcode = pam_setcred(pamh, PAM_ESTABLISH_CRED);
    if (retcode != PAM_SUCCESS)
        die_pame(pamh, retcode, "Could not set PAM credentials");
    retcode = pam_open_session(pamh, 0);
    if (retcode != PAM_SUCCESS)
        die_pame(pamh, retcode, "Could not open PAM session");

    env = (const char *const *)pam_getenvlist(pamh);
    while (env && *env) {
        if (putenv((char *)*env))
            die_e("Could not copy PAM environment");
        env++;
    }

    /* Close the log here, because PAM calls openlog(3) and
     * our log messages could go to the wrong facility */
    xcloselog();
#endif                          /* USE_PAM */

#ifdef USE_SETE_ID
    seteuid_safe(fcrontab_uid);
    /* change directory */
    if (chdir(fcrontabs) != 0) {
        error_e("Could not chdir to %s", fcrontabs);
        xexit(EXIT_ERR);
    }
    seteuid_safe(useruid);
#else                           /* USE_SETE_ID */

    if (setuid(rootuid) != 0)
        die_e("Could not change uid to rootuid");
    if (setgid(rootgid) != 0)
        die_e("Could not change gid to rootgid");
    /* change directory */
    if (chdir(fcrontabs) != 0) {
        error_e("Could not chdir to %s", fcrontabs);
        xexit(EXIT_ERR);
    }
#endif                          /* USE_SETE_ID */

    /* this program is seteuid : we set default permission mode
     * to 640 for a normal user, 600 for root, for security reasons */
    if (asuid == rootuid)
        umask(066);             /* octal : '0' + number in octal notation */
    else
        umask(026);

    snprintf(buf, sizeof(buf), "%s.orig", user);

    /* determine what action should be taken */
    if (file_opt) {

        if (strcmp(argv[file_opt], "-") == 0)

            xexit(install_stdin());

        else {
            int fd = -1;

            if (*argv[file_opt] != '/')
                /* this is just the file name, not the path : complete it */
                snprintf(file, sizeof(file), "%s/%s", orig_dir, argv[file_opt]);
            else {
                strncpy(file, argv[file_opt], sizeof(file) - 1);
                file[sizeof(file) - 1] = '\0';
            }

            fd = open_as_user(file, useruid, usergid, O_RDONLY);
            if (fd < 0)
                die_e("Could not open file %s", file);
            if (make_file(file, fd) == OK)
                xexit(EXIT_OK);
            else
                xexit(EXIT_ERR);

        }

    }

    /* remove user's entries */
    if (rm_opt == 1) {
        if (remove_fcrontab(1) == ENOENT)
            fprintf(stderr, "no fcrontab for %s\n", user);
        xexit(EXIT_OK);
    }

    /* list user's entries */
    if (list_opt == 1) {
        list_file(buf);
        xexit(EXIT_OK);
    }


    /* edit user's entries */
    if (edit_opt == 1) {
        edit_file(buf);
        xexit(EXIT_OK);
    }

    /* reinstall user's entries */
    if (reinstall_opt == 1) {
        reinstall(buf);
        xexit(EXIT_OK);
    }

    /* never reached */
    return EXIT_OK;
}
