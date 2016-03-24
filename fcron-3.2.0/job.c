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

#include "job.h"
#include "temp_file.h"

void sig_dfl(void);
void end_job(cl_t * line, int status, FILE * mailf, short mailpos,
             char **sendmailenv);
void end_mailer(cl_t * line, int status);
#ifdef HAVE_LIBPAM
void die_mail_pame(cl_t * cl, int pamerrno, struct passwd *pas, char *str,
                   env_list_t * env);
#endif
#define PIPE_READ 0
#define PIPE_WRITE 1
int read_write_pipe(int fd, void *buf, size_t size, int action);
int read_pipe(int fd, void *to, size_t size);
int write_pipe(int fd, void *buf, size_t size);
void become_user(struct cl_t *cl, struct passwd *pas, char *home);

#ifdef HAVE_LIBPAM
void
die_mail_pame(cl_t * cl, int pamerrno, struct passwd *pas, char *str,
              env_list_t * env)
/* log an error in syslog, mail user if necessary, and die */
{
    char buf[MAX_MSG];

    snprintf(buf, sizeof(buf), "%s for user '%s'", str, pas->pw_name);

    if (is_mail(cl->cl_option)) {
        char **envp = env_list_export_envp(env);
        FILE *mailf =
            create_mail(cl, "Could not run fcron job", NULL, NULL, envp);

        /* print the error in both syslog and a file, in order to mail it to user */
        if (dup2(fileno(mailf), 1) != 1 || dup2(1, 2) != 2)
            die_e("dup2() error");      /* dup2 also clears close-on-exec flag */

        foreground = 1;
        error_pame(pamh, pamerrno, buf, cl->cl_shell);
        error("Job '%s' has *not* run.", cl->cl_shell);
        foreground = 0;

        pam_end(pamh, pamerrno);

        become_user(cl, pas, "/");

        launch_mailer(cl, mailf, envp);
        /* launch_mailer() does not return : we never get here */
    }
    else
        die_pame(pamh, pamerrno, buf, cl->cl_shell);
}
#endif

void
become_user(struct cl_t *cl, struct passwd *pas, char *home)
/* Become the user who owns the job: change privileges, check PAM authorization,
 * and change dir to HOME. */
{

#ifndef RUN_NON_PRIVILEGED
    if (pas == NULL)
        die("become_user() called with a NULL struct passwd");

    /* Change running state to the user in question */
    if (initgroups(pas->pw_name, pas->pw_gid) < 0)
        die_e("initgroups failed: %s", pas->pw_name);

    if (setgid(pas->pw_gid) < 0)
        die("setgid failed: %s %d", pas->pw_name, pas->pw_gid);

    if (setuid(pas->pw_uid) < 0)
        die("setuid failed: %s %d", pas->pw_name, pas->pw_uid);
#endif                          /* not RUN_NON_PRIVILEGED */

    /* make sure HOME is defined and change dir to it */
    if (chdir(home) != 0) {
        error_e("Could not chdir to HOME dir '%s'. Trying to chdir to '/'.",
                home);
        if (chdir("/") < 0)
            die_e("Could not chdir to HOME dir /");
    }

}

void
setup_user_and_env(struct cl_t *cl, struct passwd *pas,
                   char ***sendmailenv, char ***jobenv, char **curshell,
                   char **curhome, char **content_type, char **encoding)
/* Check PAM authorization, and setup the environment variables
 * to run sendmail and to run the job itself. Change dir to HOME and check if SHELL is ok */
/* (*curshell) and (*curhome) will be allocated and should thus be freed
 * if curshell and curhome are not NULL. */
/* Return the the two env var sets, the shell to use to execle() commands and the home dir */
{
    env_list_t *env_list = env_list_init();
    env_t *e = NULL;
    char *path = NULL;
    char *myshell = NULL;
#ifdef HAVE_LIBPAM
    int retcode = 0;
    char **env;
#endif

    if (pas == NULL)
        die("setup_user_and_env() called with a NULL struct passwd");

    env_list_setenv(env_list, "USER", pas->pw_name, 1);
    env_list_setenv(env_list, "LOGNAME", pas->pw_name, 1);
    env_list_setenv(env_list, "HOME", pas->pw_dir, 1);
    /* inherit fcron's PATH for sendmail. We will later change it to DEFAULT_JOB_PATH
     * or a user defined PATH for the job itself */
    path = getenv("PATH");
    env_list_setenv(env_list, "PATH", (path != NULL) ? path : DEFAULT_JOB_PATH,
                    1);

    if (cl->cl_tz != NULL)
        env_list_setenv(env_list, "TZ", cl->cl_tz, 1);
    /* To ensure compatibility with Vixie cron, we don't use the shell defined
     * in /etc/passwd by default, but the default value from fcron.conf instead: */
    if (shell != NULL && shell[0] != '\0')
        /* default: use value from fcron.conf */
        env_list_setenv(env_list, "SHELL", shell, 1);
    else
        /* shell is empty, ie. not defined: fail back to /etc/passwd's value */
        env_list_setenv(env_list, "SHELL", pas->pw_shell, 1);

#if ( ! defined(RUN_NON_PRIVILEGED)) && defined(HAVE_LIBPAM)
    /* Open PAM session for the user and obtain any security
     * credentials we might need */

    retcode = pam_start("fcron", pas->pw_name, &apamconv, &pamh);
    if (retcode != PAM_SUCCESS)
        die_pame(pamh, retcode, "Could not start PAM for '%s'", cl->cl_shell);
    /* Some system seem to need that pam_authenticate() call.
     * Anyway, we have no way to authentificate the user :
     * we must set auth to pam_permit. */
    retcode = pam_authenticate(pamh, PAM_SILENT);
    if (retcode != PAM_SUCCESS)
        die_mail_pame(cl, retcode, pas,
                      "Could not authenticate PAM user", env_list);
    retcode = pam_acct_mgmt(pamh, PAM_SILENT);  /* permitted access? */
    if (retcode != PAM_SUCCESS)
        die_mail_pame(cl, retcode, pas,
                      "Could not init PAM account management", env_list);
    retcode = pam_setcred(pamh, PAM_ESTABLISH_CRED | PAM_SILENT);
    if (retcode != PAM_SUCCESS)
        die_mail_pame(cl, retcode, pas, "Could not set PAM credentials",
                      env_list);
    retcode = pam_open_session(pamh, PAM_SILENT);
    if (retcode != PAM_SUCCESS)
        die_mail_pame(cl, retcode, pas, "Could not open PAM session", env_list);

    for (env = pam_getenvlist(pamh); env && *env; env++) {
        env_list_putenv(env_list, *env, 1);
    }

    /* Close the log here, because PAM calls openlog(3) and
     * our log messages could go to the wrong facility */
    xcloselog();
#endif                          /* ( ! defined(RUN_NON_PRIVILEGED)) && defined(HAVE_LIBPAM) */

    /* export the environment for sendmail before we apply user customization */
    if (sendmailenv != NULL)
        *sendmailenv = env_list_export_envp(env_list);

    /* Now add user customizations to the environment to form jobenv */

    if (jobenv != NULL) {

        /* Make sure we don't keep fcron daemon's PATH (which we used for sendmail) */
        env_list_setenv(env_list, "PATH", DEFAULT_JOB_PATH, 1);

        for (e = env_list_first(cl->cl_file->cf_env_list); e != NULL;
             e = env_list_next(cl->cl_file->cf_env_list)) {
            env_list_putenv(env_list, e->e_envvar, 1);
        }

        /* make sure HOME is defined */
        env_list_putenv(env_list, "HOME=/", 0); /* don't overwrite if already defined */
        if (curhome != NULL) {
            (*curhome) = strdup2(env_list_getenv(env_list, "HOME"));
        }

        /* check that SHELL is valid */
        myshell = env_list_getenv(env_list, "SHELL");
        if (myshell == NULL || myshell[0] == '\0') {
            myshell = shell;
        }
        else if (access(myshell, X_OK) != 0) {
            if (errno == ENOENT)
                error("shell \"%s\" : no file or directory. SHELL set to %s",
                      myshell, shell);
            else
                error_e("shell \"%s\" not valid : SHELL set to %s", myshell,
                        shell);

            myshell = shell;
        }
        env_list_setenv(env_list, "SHELL", myshell, 1);
        if (curshell != NULL)
            *curshell = strdup2(myshell);

        *jobenv = env_list_export_envp(env_list);

    }

    if (content_type != NULL) {
        (*content_type) = strdup2(env_list_getenv(env_list, "CONTENT_TYPE"));
    }
    if (encoding != NULL) {
        (*encoding) =
            strdup2(env_list_getenv(env_list, "CONTENT_TRANSFER_ENCODING"));
    }

    env_list_destroy(env_list);

}

void
change_user_setup_env(struct cl_t *cl,
                      char ***sendmailenv, char ***jobenv, char **curshell,
                      char **curhome, char **content_type, char **encoding)
/* call setup_user_and_env() and become_user().
 * As a result, *curshell and *curhome will be allocated and should thus be freed
 * if curshell and curhome are not NULL. */
{
    struct passwd *pas;

    errno = 0;
    pas = getpwnam(cl->cl_runas);
    if (pas == NULL)
        die_e("failed to get passwd fields for user \"%s\"", cl->cl_runas);

    setup_user_and_env(cl, pas, sendmailenv, jobenv, curshell, curhome,
                       content_type, encoding);

    become_user(cl, pas, (curhome != NULL) ? *curhome : "/");
}

void
sig_dfl(void)
    /* set signals handling to its default */
{
    signal(SIGTERM, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGUSR1, SIG_DFL);
    signal(SIGUSR2, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
}


FILE *
create_mail(cl_t * line, char *subject, char *content_type, char *encoding,
            char **env)
    /* create a temp file and write in it a mail header */
{
    /* create temporary file for stdout and stderr of the job */
    int mailfd = temp_file(NULL);
    FILE *mailf = fdopen(mailfd, "r+");
    char hostname[USER_NAME_LEN];
    /* is this a complete mail address ? (ie. with a "@", not only a username) */
    char add_hostname = 0;
    int i = 0;

    if (mailf == NULL)
        die_e("Could not fdopen() mailfd");

#ifdef HAVE_GETHOSTNAME
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        error_e("Could not get hostname");
        hostname[0] = '\0';
    }
    else {
        /* it is unspecified whether a truncated hostname is NUL-terminated */
        hostname[USER_NAME_LEN - 1] = '\0';

        /* check if mailto is a complete mail address */
        add_hostname = (strchr(line->cl_mailto, '@') == NULL) ? 1 : 0;
    }
#else                           /* HAVE_GETHOSTNAME */
    hostname[0] = '\0';
#endif                          /* HAVE_GETHOSTNAME */

    /* write mail header */
    if (add_hostname)
        fprintf(mailf, "To: %s@%s\n", line->cl_mailto, hostname);
    else
        fprintf(mailf, "To: %s\n", line->cl_mailto);

    if (subject)
        fprintf(mailf, "Subject: fcron <%s@%s> %s: %s\n",
                line->cl_file->cf_user, (hostname[0] != '\0') ? hostname : "?",
                subject, line->cl_shell);
    else
        fprintf(mailf, "Subject: fcron <%s@%s> %s\n", line->cl_file->cf_user,
                (hostname[0] != '\0') ? hostname : "?", line->cl_shell);

    if (content_type == NULL) {
        fprintf(mailf, "Content-Type: text/plain; charset=%s\n",
                default_mail_charset);
    }
    else {
        /* user specified Content-Type header. */
        char *c = NULL;

        /* Remove new-lines or users could specify arbitrary mail headers!
         * (fcrontab should already prevent that, but better safe than sorry) */
        for (c = content_type; *c != '\0'; c++) {
            if (*c == '\n')
                *c = ' ';
        }
        fprintf(mailf, "Content-Type: %s\n", content_type);
    }

    if (encoding != NULL) {
        char *c = NULL;

        /* Remove new-lines or users could specify arbitrary mail headers!
         * (fcrontab should already prevent that, but better safe than sorry) */
        for (c = encoding; *c != '\0'; c++) {
            if (*c == '\n')
                *c = ' ';
        }
        fprintf(mailf, "Content-Transfer-Encoding: %s\n", encoding);
    }

    /* Add headers so as automated systems can identify that this message
     * is an automated one sent by fcron.
     * That's useful for example for vacation auto-reply systems: no need
     * to send such an automated response to fcron! */

    /* The Auto-Submitted header is
     * defined (and suggested by) RFC3834. */
    fprintf(mailf, "Auto-Submitted: auto-generated\n");

    /* See environ(7) and execle(3) to get documentation on environ:
     * it is an array of NULL-terminated strings, whose last entry is NULL */
    if (env != NULL) {
        for (i = 0; env[i] != NULL; i++) {
            fprintf(mailf, "X-Cron-Env: <%s>\n", env[i]);
        }
    }

    /* Final line return to end the header section: */
    fprintf(mailf, "\n");

    return mailf;
}


int
read_write_pipe(int fd, void *buf, size_t size, int action)
    /* Read/write data from/to pipe.
     * action can either be PIPE_WRITE or PIPE_READ.
     * Handles signal interruptions, and read in several passes.
     * Returns ERR in case of a closed pipe, the errno from errno
     * for other errors, and OK if everything was read successfully */
{
    int size_processed = 0;
    int ret;
    int num_retry = 0;

    while (size_processed < size) {
        errno = 0;
        if (action == PIPE_READ)
            ret = read(fd, (char *)buf + size_processed, size);
        else if (action == PIPE_WRITE)
            ret = write(fd, (char *)buf + size_processed, size);
        else {
            error("Invalid action parameter for function read_write_pipe():"
                  " %d", action);
            return ERR;
        }
        if (ret > 0)
            /* some data read correctly -- we still may need
             * one or several calls of read() to read the rest */
            size_processed += ret;
        else if (ret < 0 && errno == EINTR)
            /* interrupted by a signal : let's try again */
            continue;
        else {
            /* error */

            if (ret == 0) {
                /* is it really an error when writing ? should we continue
                 * in this case ? */
                if (num_retry < 3) {
                    num_retry++;
                    error_e
                        ("read_write_pipe(): read/write returned 0: retrying... (size: %d, size_processed: %d, num_retry: %d)",
                         size, size_processed, num_retry);
                    sleep(1);
                    continue;
                }
                else
                    return ERR;
            }
            else
                return errno;
        }
    }

    return OK;
}

int
read_pipe(int fd, void *buf, size_t size)
    /* Read data from pipe. 
     * Handles signal interruptions, and read in several passes.
     * Returns ERR in case of a closed pipe, the errno from read
     * for other errors, and OK if everything was read successfully */
{
    return read_write_pipe(fd, buf, size, PIPE_READ);
}

int
write_pipe(int fd, void *buf, size_t size)
    /* Read data from pipe. 
     * Handles signal interruptions, and read in several passes.
     * Returns ERR in case of a closed pipe, the errno from write
     * for other errors, and OK if everything was read successfully */
{
    return read_write_pipe(fd, buf, size, PIPE_WRITE);
}

void
run_job_grand_child_setup_stderr_stdout(cl_t * line, int *pipe_fd)
    /* setup stderr and stdout correctly so as the mail containing
     * the output of the job can be send at the end of the job.
     * Close the pipe (both ways). */
{

    if (is_mail(line->cl_option)) {
        /* we can't dup2 directly to mailfd, since a "cmd > /dev/stderr" in
         * a script would erase all previously collected message */
        if (dup2(pipe_fd[1], 1) != 1 || dup2(1, 2) != 2)
            die_e("dup2() error");      /* dup2 also clears close-on-exec flag */
        /* we close the pipe_fd[]s : the resources remain, and the pipe will
         * be effectively close when the job stops */
        xclose_check(&(pipe_fd[0]), "pipe_fd[0] in setup_stderr_stdout");
        xclose_check(&(pipe_fd[1]), "pipe_fd[1] in setup_stderr_stdout");
        /* Standard buffering results in unwanted behavior (some messages,
         * at least error from fcron process itself, are lost) */
#ifdef HAVE_SETLINEBUF
        setlinebuf(stdout);
        setlinebuf(stderr);
#else
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
#endif
    }
    else if (foreground) {
        if (freopen("/dev/null", "w", stdout) == NULL)
            error_e("could not freopen /dev/null as stdout");
        if (freopen("/dev/null", "w", stderr) == NULL)
            error_e("could not freopen /dev/null as stderr");
    }

}

void
run_job_grand_child_setup_nice(cl_t * line)
    /* set the nice value for the job */
{
    if (line->cl_nice != 0) {
        errno = 0;              /* so that it works with any libc and kernel */
        if (nice(line->cl_nice) == -1 && errno != 0)
            error_e("could not set nice value");
    }
}

int
run_job(struct exe_t *exeent)
    /* fork(), redirect outputs to a temp file, and execl() the task.
     * Return ERR if it could not fork() the first time, OK otherwise. */
{

    pid_t pid;
    cl_t *line = exeent->e_line;
    int pipe_pid_fd[2];
    int ret = 0;

    /* prepare the job execution */
    if (pipe(pipe_pid_fd) != 0) {
        error_e("pipe(pipe_pid_fd) : setting job_pid to -1");
        exeent->e_job_pid = -1;
        pipe_pid_fd[0] = pipe_pid_fd[1] = -1;
    }

#ifdef CHECKRUNJOB
    debug
        ("run_job(): first pipe created successfully : about to do first fork()");
#endif                          /* CHECKRUNJOB */

    switch (pid = fork()) {
    case -1:
        error_e("Fork error : could not exec '%s'", line->cl_shell);
        return ERR;
        break;

    case 0:
        /* child */
        {
            struct passwd *pas = NULL;
            char **jobenv = NULL;
            char **sendmailenv = NULL;
            char *curshell = NULL;
            char *curhome = NULL;
            char *content_type = NULL;
            char *encoding = NULL;
            FILE *mailf = NULL;
            int status = 0;
            int to_stdout = foreground && is_stdout(line->cl_option);
            int pipe_fd[2];
            short int mailpos = 0;      /* 'empty mail file' size */
#ifdef WITH_SELINUX
            int flask_enabled = is_selinux_enabled();
#endif

            /* // */
            debug("run_job(): child: %s, output to %s, %s, %s\n",
                  is_mail(line->cl_option) ? "mail" : "no mail",
                  to_stdout ? "stdout" : "file",
                  foreground ? "running in foreground" :
                  "running in background",
                  is_stdout(line->cl_option) ? "stdout" : "normal");
            /* // */

            errno = 0;
            pas = getpwnam(line->cl_runas);
            if (pas == NULL)
                die_e("failed to get passwd fields for user \"%s\"",
                      line->cl_runas);

            setup_user_and_env(line, pas, &sendmailenv, &jobenv, &curshell,
                               &curhome, &content_type, &encoding);

            /* close unneeded READ fd */
            xclose_check(&(pipe_pid_fd[0]), "child's pipe_pid_fd[0]");

            pipe_fd[0] = pipe_fd[1] = -1;
            if (!to_stdout && is_mail(line->cl_option)) {
                /* we create the temp file (if needed) before change_user(),
                 * as temp_file() needs root privileges */
                /* if we run in foreground, stdout and stderr point to the console.
                 * Otherwise, stdout and stderr point to /dev/null . */
                mailf = create_mail(line, NULL, content_type, encoding, jobenv);
                mailpos = ftell(mailf);
                if (pipe(pipe_fd) != 0)
                    die_e("could not pipe() (job not executed)");
            }

            become_user(line, pas, curhome);
            Free_safe(curhome);

            /* restore umask to default */
            umask(saved_umask);

            sig_dfl();

#ifdef CHECKRUNJOB
            debug
                ("run_job(): child: change_user() done -- about to do 2nd fork()");
#endif                          /* CHECKRUNJOB */

            /* now, run the job */
            switch (pid = fork()) {
            case -1:
                error_e("Fork error : could not exec '%s'", line->cl_shell);
                if (write(pipe_pid_fd[1], &pid, sizeof(pid)) < 0)
                    error_e("could not write child pid to pipe_pid_fd[1]");
                xclose_check(&(pipe_fd[0]), "child's pipe_fd[0]");
                xclose_check(&(pipe_fd[1]), "child's pipe_fd[1]");
                xclose_check(&(pipe_pid_fd[1]), "child's pipe_pid_fd[1]");
                exit(EXIT_ERR);
                break;

            case 0:
                /* grand child (child of the 2nd fork) */

                /* the grand child does not use this pipe: close remaining fd */
                xclose_check(&(pipe_pid_fd[1]), "grand child's pipe_pid_fd[1]");

                if (!to_stdout)
                    /* note : the following closes the pipe */
                    run_job_grand_child_setup_stderr_stdout(line, pipe_fd);

                foreground = 1;
                /* now, errors will be mailed to the user (or to /dev/null) */

                run_job_grand_child_setup_nice(line);

                xcloselog();

#if defined(CHECKJOBS) || defined(CHECKRUNJOB)
                /* this will force to mail a message containing at least the exact
                 * and complete command executed for each execution of all jobs */
                debug("run_job(): grand-child: Executing \"%s -c %s\"",
                      curshell, line->cl_shell);
#endif                          /* CHECKJOBS OR CHECKRUNJOB */

#ifdef WITH_SELINUX
                if (flask_enabled
                    && setexeccon(line->cl_file->cf_user_context) < 0)
                    die_e("Can't set execute context '%s' for user '%s'.",
                          line->cl_file->cf_user_context, line->cl_runas);
#else
                if (setsid() == -1) {
                    die_e("setsid(): errno %d", errno);
                }
#endif
                execle(curshell, curshell, "-c", line->cl_shell, NULL, jobenv);
                /* execle returns only on error */
                die_e("Couldn't exec shell '%s'", curshell);

                /* execution never gets here */

            default:
                /* child (parent of the 2nd fork) */

                /* close unneeded WRITE pipe and READ pipe */
                xclose_check(&(pipe_fd[1]), "child's pipe_fd[1]");

#ifdef CHECKRUNJOB
                debug("run_job(): child: pipe_fd[1] and pipe_pid_fd[0] closed"
                      " -- about to write grand-child pid to pipe");
#endif                          /* CHECKRUNJOB */

                /* give the pid of the child to the parent (main) fcron process */
                ret = write_pipe(pipe_pid_fd[1], &pid, sizeof(pid));
                if (ret != OK) {
                    if (ret == ERR)
                        error
                            ("run_job(): child: Could not write job pid to pipe");
                    else {
                        errno = ret;
                        error_e
                            ("run_job(): child: Could not write job pid to pipe");
                    }
                }

#ifdef CHECKRUNJOB
                debug("run_job(): child: grand-child pid written to pipe");
#endif                          /* CHECKRUNJOB */

                if (!is_nolog(line->cl_option))
                    explain("Job '%s' started for user %s (pid %d)",
                            line->cl_shell, line->cl_file->cf_user, pid);

                if (!to_stdout && is_mail(line->cl_option)) {
                    /* user wants a mail : we use the pipe */
                    char mailbuf[TERM_LEN];
                    FILE *pipef = fdopen(pipe_fd[0], "r");

                    if (pipef == NULL)
                        die_e("Could not fdopen() pipe_fd[0]");

                    mailbuf[sizeof(mailbuf) - 1] = '\0';
                    while (fgets(mailbuf, sizeof(mailbuf), pipef) != NULL)
                        if (fputs(mailbuf, mailf) < 0)
                            warn("fputs() failed to write to mail file for job '%s' (pid %d)", line->cl_shell, pid);
                    /* (closes also pipe_fd[0]): */
                    xfclose_check(&pipef, "child's pipef");
                }

                /* FIXME : FOLLOWING HACK USELESS ? */
                /* FIXME : HACK
                 * this is a try to fix the bug on sorcerer linux (no jobs
                 * exectued at all, and
                 * "Could not read job pid : setting it to -1: No child processes"
                 * error messages) */
                /* use a select() or similar to know when parent has read
                 * the pid (with a timeout !) */
                /* // */
                sleep(2);
                /* // */
#ifdef CHECKRUNJOB
                debug("run_job(): child: closing pipe with parent");
#endif                          /* CHECKRUNJOB */
                xclose_check(&(pipe_pid_fd[1]), "child's pipe_pid_fd[1]");

                /* we use a while because of a possible interruption by a signal */
                while ((pid = wait3(&status, 0, NULL)) > 0) {
#ifdef CHECKRUNJOB
                    debug("run_job(): child: ending job pid %d", pid);
#endif                          /* CHECKRUNJOB */
                    end_job(line, status, mailf, mailpos, sendmailenv);
                }

                /* execution never gets here */

            }

            /* execution should never gets here, but if it happened we exit with an error */
            exit(EXIT_ERR);
        }

    default:
        /* parent */

        /* close unneeded WRITE fd */
        xclose_check(&(pipe_pid_fd[1]), "parent's pipe_pid_fd[1]");

        exeent->e_ctrl_pid = pid;

#ifdef CHECKRUNJOB
        debug("run_job(): about to read grand-child pid...");
#endif                          /* CHECKRUNJOB */

        /* read the pid of the job */
        ret = read_pipe(pipe_pid_fd[0], &(exeent->e_job_pid), sizeof(pid_t));
        if (ret != OK) {
            if (ret == ERR) {
                error("Could not read job pid because of closed pipe:"
                      " setting it to -1");
            }
            else {
                errno = ret;
                error_e("Could not read job pid : setting it to -1");
            }

            exeent->e_job_pid = -1;
        }
        xclose_check(&(pipe_pid_fd[0]), "parent's pipe_pid_fd[0]");

#ifdef CHECKRUNJOB
        debug
            ("run_job(): finished reading pid of the job -- end of run_job().");
#endif                          /* CHECKRUNJOB */

    }

    return OK;

}

void
end_job(cl_t * line, int status, FILE * mailf, short mailpos,
        char **sendmailenv)
    /* if task have made some output, mail it to user */
{

    char mail_output = 0;
    char *m = NULL;

#ifdef USE_SENDMAIL
    if (mailf != NULL && (is_mailzerolength(line->cl_option)
                          || (is_mail(line->cl_option)
                              && (
                                     /* job wrote some output and we wan't it in any case: */
                                     ((fseek(mailf, 0, SEEK_END) == 0
                                       && ftell(mailf) > mailpos)
                                      && !is_erroronlymail(line->cl_option))
                                     ||
                                     /* or we want an email only if the job returned an error: */
                                     !(WIFEXITED(status)
                                       && WEXITSTATUS(status) == 0)
                              )
                          )
        )
        ) {
        /* an output exit : we will mail it */
        mail_output = 1;
    }
    /* or else there is no output to email -- mail_output is already set to 0 */
#endif                          /* USE_SENDMAIL */

    m = (mail_output == 1) ? " (mailing output)" : "";
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        if (!is_nolog(line->cl_option))
            explain("Job '%s' completed%s", line->cl_shell, m);
    }
    else if (WIFEXITED(status)) {
        warn("Job '%s' terminated (exit status: %d)%s",
             line->cl_shell, WEXITSTATUS(status), m);
        /* there was an error : in order to inform the user by mail, we need
         * to add some data to mailf */
        if (mailf != NULL)
            fprintf(mailf, "Job '%s' terminated (exit status: %d)%s",
                    line->cl_shell, WEXITSTATUS(status), m);
    }
    else if (WIFSIGNALED(status)) {
        error("Job '%s' terminated due to signal %d%s",
              line->cl_shell, WTERMSIG(status), m);
        if (mailf != NULL)
            fprintf(mailf, "Job '%s' terminated due to signal %d%s",
                    line->cl_shell, WTERMSIG(status), m);
    }
    else {                      /* is this possible? */
        error("Job '%s' terminated abnormally %s", line->cl_shell, m);
        if (mailf != NULL)
            fprintf(mailf, "Job '%s' terminated abnormally %s", line->cl_shell,
                    m);
    }

#ifdef HAVE_LIBPAM
    /* we close the PAM session before running the mailer command :
     * it avoids a fork(), and we use PAM anyway to control whether a user command
     * should be run or not.
     * We consider that the administrator can use a PAM compliant mailer to control
     * whether a mail can be sent or not.
     * It should be ok like that, otherwise contact me ... -tg */

    /* Aiee! we may need to be root to do this properly under Linux.  Let's
     * hope we're more l33t than PAM and try it as non-root. If someone
     * complains, I'll fix this :P -hmh */
    pam_setcred(pamh, PAM_DELETE_CRED | PAM_SILENT);
    pam_end(pamh, pam_close_session(pamh, PAM_SILENT));
#endif

    if (mail_output == 1) {
        launch_mailer(line, mailf, sendmailenv);
        /* never reached */
        die_e("Internal error: launch_mailer returned");
    }

    /* if mail is sent, execution doesn't get here : close /dev/null */
    xfclose_check(&mailf, "Can't close file mailf");

    exit(0);

}

void
launch_mailer(cl_t * line, FILE * mailf, char **sendmailenv)
    /* mail the output of a job to user */
{
#ifdef USE_SENDMAIL
    foreground = 0;

    /* set stdin to the job's output */

    /* fseek() should work, but it seems that it is not always the case
     * (users have reported problems on gentoo and LFS).
     * For those users, lseek() works, so I have decided to use both,
     * as I am not sure that lseek(fileno(...)...) will work as expected
     * on non linux systems. */
    if (fseek(mailf, 0, SEEK_SET) != 0)
        die_e("Can't fseek()");
    if (lseek(fileno(mailf), 0, SEEK_SET) != 0)
        die_e("Can't lseek()");
    if (dup2(fileno(mailf), 0) != 0)
        die_e("Can't dup2(fileno(mailf))");

    xcloselog();

    if (chdir("/") < 0)
        die_e("Could not chdir to /");

    /* run sendmail with mail file as standard input */
    /* // */
    debug("execle(%s, %s, %s, %s, NULL, sendmailenv)", sendmail, sendmail,
          SENDMAIL_ARGS, line->cl_mailto);
    /* // */
    execle(sendmail, sendmail, SENDMAIL_ARGS, line->cl_mailto, NULL,
           sendmailenv);
    die_e("Couldn't exec '%s'", sendmail);
#else                           /* defined(USE_SENDMAIL) */
    exit(EXIT_OK);
#endif
}
