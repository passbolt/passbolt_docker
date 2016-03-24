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


/* This file contains all fcron's code (server) to handle communication with fcrondyn */


#include "fcron.h"
#include "socket.h"
#include "getloadavg.h"
#include "database.h"
#include "fcronconf.h"


void remove_connection(struct fcrondyn_cl **client,
                       struct fcrondyn_cl *prev_client);
void exe_cmd(struct fcrondyn_cl *client);
#ifdef SO_PEERCRED              /* linux */
void auth_client_so_peercred(struct fcrondyn_cl *client);
#elif defined(HAVE_GETPEERUCRED) || defined(HAVE_GETPEEREID)    /* resp. solaris 10 and Free/OpenBSD) */
void auth_client_getpeer(struct fcrondyn_cl *client);
#endif
void auth_client_password(struct fcrondyn_cl *client);
void cmd_ls(struct fcrondyn_cl *client, long int *cmd, int fd, int is_root);
void print_fields(int fd, unsigned char *details);
void print_line(int fd, struct cl_t *line, unsigned char *details, pid_t pid,
                int index, time_t until);
void cmd_on_exeq(struct fcrondyn_cl *client, long int *cmd, int fd,
                 int is_root);
void cmd_renice(struct fcrondyn_cl *client, long int *cmd, int fd, exe_t * e,
                int is_root);
void cmd_send_signal(struct fcrondyn_cl *client, long int *cmd, int fd,
                     exe_t * e);
void cmd_run(struct fcrondyn_cl *client, long int *cmd, int fd, int is_root);
void add_to_select_set(int fd, fd_set * set, int *max_fd);
void remove_from_select_set(int fd, fd_set * set, int *max_fd);

fcrondyn_cl *fcrondyn_cl_base;  /* list of connected fcrondyn clients */
int fcrondyn_cl_num = 0;        /* number of fcrondyn clients currently connected */
fd_set read_set;                /* client fds list : cmd waiting ? */
fd_set master_set;              /* master set : needed since select() modify read_set */
int set_max_fd = 0;             /* needed by select() */
int listen_fd = -1;             /* fd which catches incoming connection */
int auth_fail = 0;              /* number of auth failure */
time_t auth_nofail_since = 0;   /* we refuse auth since x due to too many failures */

/* some error messages ... */
char err_no_err_str[] = "Command successfully completed.\n";
char err_unknown_str[] =
    "Fcron has encountered an error : command not completed.\n";
char err_cmd_unknown_str[] = "Unknown command.\n";
char err_job_nfound_str[] = "No corresponding job found.\n";
char err_rjob_nfound_str[] =
    "No corresponding running job found.\n (The job may have "
    "just finished its execution.)\n";
char err_invalid_user_str[] = "Invalid user : unable to find a passwd entry.\n";
char err_invalid_args_str[] = "Invalid arguments.\n";
char err_job_nallowed_str[] = "You are not allowed to see/change this line.\n";
char err_all_nallowed_str[] = "You are not allowed to list all jobs.\n";
char err_others_nallowed_str[] =
    "You are not allowed to list other users' jobs.\n";

/* Send an error message to fcrondyn */
#define Send_err_msg(FD, MSG) \
          send(FD, MSG, sizeof(MSG), 0);

/* to tell fcrondyn there's no more data to wait */
#define Tell_no_more_data(FD) \
	    send(FD, END_STR, sizeof(END_STR), 0);

/* Send an error message to fcrondyn and return */
#define Send_err_msg_end(FD, MSG) \
        { \
          send(FD, MSG, sizeof(MSG), 0); \
          Tell_no_more_data(FD); \
        }

/* which bit corresponds to which field ? */
#define FIELD_USER 0
#define FIELD_RQ 1
#define FIELD_PID 2
#define FIELD_SCHEDULE 3
#define FIELD_LAVG 4
#define FIELD_INDEX 5
#define FIELD_OPTIONS 6

/* the number of char we need (8 bits (i.e. fields) per char) */
#define FIELD_NUM_SIZE 1


void
add_to_select_set(int fd, fd_set * set, int *max_fd)
    /* add fd to set, and update max_fd if necessary (for select()) */
{
    FD_SET(fd, set);
    if (fd > *max_fd)
        *max_fd = fd;
}


void
remove_from_select_set(int fd, fd_set * set, int *max_fd)
    /* remove fd to set, and update max_fd if necessary (for select()) */
{
    FD_CLR(fd, set);
    if (fd == *max_fd) {
        /* find the biggest fd in order to update max_fd */
        struct fcrondyn_cl *client;
        int tmp_max_fd = listen_fd;

        for (client = fcrondyn_cl_base; client != NULL;
             client = client->fcl_next) {
            if (client->fcl_sock_fd > tmp_max_fd)
                tmp_max_fd = client->fcl_sock_fd;
        }

        /* update max_fd */
        *max_fd = tmp_max_fd;
    }
}


void
init_socket(void)
    /* do everything needed to get a working listening socket */
{
    struct sockaddr_un addr;
    int len = 0;
    int sun_len = 0;

    /* used in fcron.c:main_loop():select() */
    FD_ZERO(&read_set);
    FD_ZERO(&master_set);

    if ((listen_fd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1) {
        error_e("Could not create socket : fcrondyn won't work");
        return;
    }

    addr.sun_family = AF_UNIX;
    len = strlen(fifofile);
    if (len > sizeof(addr.sun_path) - 1) {
        error("Error : fifo file path too long (max is %d)",
              sizeof(addr.sun_path) - 1);
        goto err;
    }
    strncpy(addr.sun_path, fifofile, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
    sun_len = (addr.sun_path - (char *)&addr) + len;
#if HAVE_SA_LEN
    addr.sun_len = sun_len;
#endif

    unlink(fifofile);
    if (bind(listen_fd, (struct sockaddr *)&addr, sun_len) != 0) {
        error_e("Cannot bind socket to '%s'", fifofile);
        goto err;
    }

    if (listen(listen_fd, MAX_CONNECTION) != 0) {
        error_e("Cannot set socket in listen mode");
        goto err;
    }

    /* */
    /* The exec bit is not necessary and ignored on all systems but AIX, where it is
     * needed to allow fcrondyn to open the file */
    if (chmod
        (fifofile,
         S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH |
         S_IWOTH | S_IXOTH) != 0)
        error_e("Cannot chmod() socket file");
    /* */

    fcntl(listen_fd, F_SETFD, 1);
    /* set listen_fd to O_NONBLOCK : we do not want fcron to be stopped on error, etc */
    if (fcntl(listen_fd, F_SETFL, fcntl(listen_fd, F_GETFL) | O_NONBLOCK) == -1) {
        error_e
            ("Could not set listen_fd attribute O_NONBLOCK : no fcrondyn support");
        goto err;
    }

    /* no error */
    add_to_select_set(listen_fd, &master_set, &set_max_fd);

    /* copy master in read_fs, because read_fs will be modified by select() */
    read_set = master_set;
    debug("Socket initialized : listen_fd : %d set_max_fd : %d ", listen_fd,
          set_max_fd);
    return;

 err:
    close_socket();

}

#if defined(HAVE_GETPEERUCRED)
void
auth_client_getpeer(struct fcrondyn_cl *client)
    /* check client identity by reading its credentials from the socket
     * using getpeerucred() (Solaris 10 onward).
     * Sets client->fcl_user on success, don't do anything on failure
     * so that the client stays unauthenticated */
{
    struct passwd *p_entry = NULL;
    ucred_t *ucred = NULL;
    uid_t uid = -1;

    if (getpeerucred(client->fcl_sock_fd, &ucred) < 0) {
        error_e("Could not get client credentials using getpeerucred()");
        return;
    }
    uid = ucred_getruid(ucred);
    if (uid == -1) {
        error_e("Could not get client uid from ucred_t");
        return;
    }
    p_entry = getpwuid(uid);
    if (p_entry == NULL) {
        error_e("Could not find password entry for uid %d", uid);
        return;
    }

    /* Successfully identified user: */
    client->fcl_user = strdup2(p_entry->pw_name);

    explain("Client's pid=%d, uid=%d, username=%s",
            ucred_getpid(ucred), uid, client->fcl_user);

}
#endif                          /* HAVE_GETPEERUCRED */

#if defined(HAVE_GETPEEREID)
/*
 * WARNING: UNTESTED CODE !!!
 */
void
auth_client_getpeer(struct fcrondyn_cl *client)
    /* check client identity by reading its credentials from the socket
     * using getpeerucred() (Solaris 10 onward) or getpeereid(open/freeBSD).
     * Sets client->fcl_user on success, don't do anything on failure
     * so that the client stays unauthenticated */
{
    struct passwd *p_entry = NULL;
    uid_t euid = -1;
    gid_t egid = -1;

    if (getpeereid(client->fcl_sock_fd, &euid, &egid) < 0) {
        error_e("Could not get client credentials using getpeereid()");
        return;
    }
    p_entry = getpwuid(euid);
    if (p_entry == NULL) {
        error_e("Could not find password entry for uid %d", euid);
        return;
    }

    /* Successfully identified user: */
    client->fcl_user = strdup2(p_entry->pw_name);

    explain("Client's uid=%d, gid=%d username=%s", euid, egid,
            client->fcl_user);

}
#endif                          /* HAVE_GETPEEREID */



#ifdef SO_PEERCRED
void
auth_client_so_peercred(struct fcrondyn_cl *client)
    /* check client identity by reading its credentials from the socket
     * using getsockopt(SO_PEERCRED) (Linux)
     * Sets client->fcl_user on success, don't do anything on failure
     * so that the client stays unauthenticated */
{
    const int true = 1;
    /* There is no ucred.h (or equivalent) on linux to define struct ucred (!!)
     * so we do it here */
#if ! ( defined(HAVE_CRED_H) && defined(HAVE_UCRED_H) \
		&& defined(HAVE_SYS_CRED_H) && defined(HAVE_SYS_UCRED_H) )
    struct ucred {
        pid_t pid;
        uid_t uid;
        gid_t gid;
    };
#endif                          /* struct ucred not defined */
    struct ucred cred;
    socklen_t cred_size = sizeof(cred);
    struct passwd *p_entry = NULL;

    setsockopt(client->fcl_sock_fd, SOL_SOCKET, SO_PASSCRED, &true,
               sizeof(true));
    if (getsockopt
        (client->fcl_sock_fd, SOL_SOCKET, SO_PEERCRED, &cred,
         &cred_size) != 0) {
        error_e
            ("Could not get client credentials using getsockopt(SO_PEERCRED)");
        return;
    }

    p_entry = getpwuid(cred.uid);
    if (p_entry == NULL) {
        error_e("Could not find password entry for uid %d", cred.uid);
        return;
    }

    /* Successfully identified user: */
    client->fcl_user = strdup2(p_entry->pw_name);

    explain("Client's pid=%d, uid=%d, gid=%d username=%s", cred.pid, cred.uid,
            cred.gid, client->fcl_user);

}
#endif                          /* SO_PEERCRED */

void
auth_client_password(struct fcrondyn_cl *client)
    /* check client identity by asking him to input his password */
{
    char *pass_cry = NULL;
    char *pass_sys = NULL;
    char *pass_str = NULL;

#ifdef HAVE_GETSPNAM
    struct spwd *pass_sp = NULL;
    if ((pass_sp = getspnam((char *)client->fcl_cmd)) == NULL) {
        error_e("could not getspnam %s", (char *)client->fcl_cmd);
        send(client->fcl_sock_fd, "0", sizeof("0"), 0);
        return;
    }
    pass_sys = pass_sp->sp_pwdp;
#else
    struct passwd *pass = NULL;
    errno = 0;
    if ((pass = getpwnam((char *)client->fcl_cmd)) == NULL) {
        error_e("could not getpwnam %s", (char *)client->fcl_cmd);
        send(client->fcl_sock_fd, "0", sizeof("0"), 0);
        return;
    }
    pass_sys = pass->pw_passwd;
#endif

    /* */
    debug("auth_client_password() : socket : %d", client->fcl_sock_fd);
    /* */

    /* we need to limit auth failures : otherwise fcron may be used to "read"
     * shadow password !!! (or to crack it using a test-all-possible-password attack) */
    if (auth_fail > 0 && auth_nofail_since + AUTH_WAIT <= now)
        /* no auth time exceeded : set counter to 0 */
        auth_fail = 0;
    if (auth_fail >= MAX_AUTH_FAIL) {
        error("Too many authentication failures : try to connect later.");
        send(client->fcl_sock_fd, "0", sizeof("0"), 0);
        auth_fail = auth_nofail_since = 0;
        return;
    }

    /* the password is stored after the user name */
    pass_str = &((char *)client->fcl_cmd)[strlen((char *)client->fcl_cmd) + 1];
    if ((pass_cry = crypt(pass_str, pass_sys)) == NULL) {
        error_e("could not crypt()");
        send(client->fcl_sock_fd, "0", sizeof("0"), 0);
        Overwrite(pass_str);
        return;
    }

/*      debug("pass_sp->sp_pwdp : %s", pass_sp->sp_pwdp); */
/*      debug("pass_cry : %s", pass_cry); */
    if (strcmp(pass_cry, pass_sys) == 0) {
        client->fcl_user = strdup2((char *)client->fcl_cmd);
        send(client->fcl_sock_fd, "1", sizeof("1"), 0);
    }
    else {
        auth_fail++;
        auth_nofail_since = now;
        error("Invalid passwd for %s from socket %d",
              (char *)client->fcl_cmd, client->fcl_sock_fd);
        send(client->fcl_sock_fd, "0", sizeof("0"), 0);
    }

    Overwrite(pass_str);
}


#define Test_add_field(FIELD_NB, FIELD_STR) \
    if ( (bit_test(details, FIELD_NB)) ) { \
        strncat(fields, FIELD_STR, sizeof(fields)-1 - len); \
        len += (sizeof(FIELD_STR)-1); \
    }
#define Add_field(FIELD_STR) \
    strncat(fields, FIELD_STR, sizeof(fields) - len - 1); \
    len += (sizeof(FIELD_STR)-1);

void
print_fields(int fd, unsigned char *details)
    /* print a line describing the field types used in print_line() */
{
    char fields[TERM_LEN];
    char field_id[] = "ID   ";
    char field_user[] = "|USER     ";
    char field_rq[] = "|R&Q ";
    char field_options[] = "|OPTIONS  ";
    char field_schedule[] = "|SCHEDULE        ";
    char field_until[] = "|LAVG 1,5,15 UNTIL       STRICT";
    char field_pid[] = "|PID    ";
    char field_index[] = "|INDEX";
    char field_cmd[] = "|CMD";
    char field_endline[] = "\n";
    int len = 0;

    fields[0] = '\0';

    Add_field(field_id);
    Test_add_field(FIELD_USER, field_user);
    Test_add_field(FIELD_PID, field_pid);
    Test_add_field(FIELD_INDEX, field_index);
    Test_add_field(FIELD_RQ, field_rq);
    Test_add_field(FIELD_OPTIONS, field_options);
    Test_add_field(FIELD_LAVG, field_until);
    Test_add_field(FIELD_SCHEDULE, field_schedule);
    Add_field(field_cmd);
    Add_field(field_endline);

    fields[TERM_LEN - 1] = '\0';

    if (send(fd, fields, (len < sizeof(fields)) ? len : sizeof(fields), 0) < 0)
        error_e("error in send()");

}


void
print_line(int fd, struct cl_t *line, unsigned char *details, pid_t pid,
           int index, time_t until)
    /* print some basic fields of a line, and some more if details == 1 */
{
    char buf[TERM_LEN];
    int len = 0;
    struct tm *ftime;


    len = snprintf(buf, sizeof(buf), "%-5ld", line->cl_id);
    if (bit_test(details, FIELD_USER))
        len +=
            snprintf(buf + len, sizeof(buf) - len, "|%-9s",
                     line->cl_file->cf_user);
    if (bit_test(details, FIELD_PID))
        len += snprintf(buf + len, sizeof(buf) - len, "|%-7d", (int)pid);
    if (bit_test(details, FIELD_INDEX))
        len += snprintf(buf + len, sizeof(buf) - len, "|%-5d", index);
    if (bit_test(details, FIELD_RQ))
        len += snprintf(buf + len, sizeof(buf) - len, "|%-4d", line->cl_numexe);
    if (bit_test(details, FIELD_OPTIONS)) {
        char opt[9];
        int i = 0;
        opt[0] = '\0';
        if (is_lavg(line->cl_option))
            i += snprintf(opt + i, sizeof(opt) - i, "L%.*s",
                          (is_lavg_sev(line->cl_option)) ? 0 : 1, "O");
        if (is_serial(line->cl_option))
            i += snprintf(opt + i, sizeof(opt) - i, "%.*sS%.*s", i, ",",
                          (is_serial_sev(line->cl_option)) ? 0 : 1, "O");
        if (is_exe_sev(line->cl_option))
            i += snprintf(opt + i, sizeof(opt) - i, "%.*sES", i, ",");

        len += snprintf(buf + len, sizeof(buf) - len, "|%-9s", opt);
    }
    if (bit_test(details, FIELD_LAVG)) {
        len += snprintf(buf + len, sizeof(buf) - len, "|%.1f,%.1f,%.1f",
                        ((double)((line->cl_lavg)[0])) / 10,
                        ((double)((line->cl_lavg)[1])) / 10,
                        ((double)((line->cl_lavg)[2])) / 10);
        if (until > 0) {
            ftime = localtime(&until);
            len +=
                snprintf(buf + len, sizeof(buf) - len,
                         " %04d-%02d-%02d %02d:%02d %s",
                         (ftime->tm_year + 1900), (ftime->tm_mon + 1),
                         ftime->tm_mday, ftime->tm_hour, ftime->tm_min,
                         (is_strict(line->cl_option)) ? "Y" : "N");
        }
        else
            len +=
                snprintf(buf + len, sizeof(buf) - len, " %18s",
                         " (no until set) ");
    }
    if (bit_test(details, FIELD_SCHEDULE)) {
        ftime = localtime(&(line->cl_nextexe));
        len +=
            snprintf(buf + len, sizeof(buf) - len, "|%04d-%02d-%02d %02d:%02d",
                     (ftime->tm_year + 1900), (ftime->tm_mon + 1),
                     ftime->tm_mday, ftime->tm_hour, ftime->tm_min);
    }
    len += snprintf(buf + len, sizeof(buf) - len, "|%s\n", line->cl_shell);

    if (send(fd, buf, (len < sizeof(buf)) ? len : sizeof(buf), 0) < 0)
        error_e("error in send()");

}


#define Test_line(LINE, PID, INDEX, UNTIL) \
            { \
                if (all || strcmp(user, LINE->cl_file->cf_user) == 0 ) { \
		    print_line(fd, LINE, fields, PID, INDEX, UNTIL); \
		    found = 1; \
                } \
            }
void
cmd_ls(struct fcrondyn_cl *client, long int *cmd, int fd, int is_root)
    /* run a command which lists some jobs */
{
    int found = 0;
    int all = (cmd[1] == ALL) ? 1 : 0;
    char *user = NULL;
    struct job_t *j;
    int i;
    unsigned char fields[FIELD_NUM_SIZE];
    exe_t *e = NULL;
    lavg_t *l = NULL;

    for (i = 0; i < FIELD_NUM_SIZE; i++)
        fields[i] = 0;

    switch (cmd[0]) {
    case CMD_DETAILS:
        bit_set(fields, FIELD_SCHEDULE);
        bit_set(fields, FIELD_RQ);
        bit_set(fields, FIELD_USER);
        bit_set(fields, FIELD_OPTIONS);
        print_fields(fd, fields);
        for (j = queue_base; j != NULL; j = j->j_next) {
            if (cmd[1] == j->j_line->cl_id) {
                if (strcmp(client->fcl_user, j->j_line->cl_file->cf_user) == 0
                    || is_root)
                    print_line(fd, j->j_line, fields, 0, 0, 0);
                else
                    Send_err_msg(fd, err_job_nfound_str);
                found = 1;
                break;
            }
        }
        break;

    case CMD_LIST_JOBS:
    case CMD_LIST_LAVGQ:
    case CMD_LIST_SERIALQ:
    case CMD_LIST_EXEQ:
        if (cmd[0] == CMD_LIST_LAVGQ) {
            double lavg[3] = { 0, 0, 0 };
            char lavg_str[TERM_LEN];
            getloadavg(lavg, 3);
            i = snprintf(lavg_str, sizeof(lavg_str), "Current load average : "
                         "%.1f, %.1f, %.1f\n", lavg[0], lavg[1], lavg[2]);
            send(fd, lavg_str, i, 0);

            bit_set(fields, FIELD_LAVG);
        }
        else
            bit_set(fields, FIELD_SCHEDULE);

        if (cmd[0] == CMD_LIST_SERIALQ)
            bit_set(fields, FIELD_INDEX);

        if (cmd[0] == CMD_LIST_EXEQ)
            bit_set(fields, FIELD_PID);

        if (all && !is_root) {
            warn("User %s tried to list *all* jobs.", client->fcl_user);
            Send_err_msg_end(fd, err_all_nallowed_str);
            return;
        }
        if (all)
            bit_set(fields, FIELD_USER);
        print_fields(fd, fields);

        if (!all) {
            struct passwd *pass;

#ifdef SYSFCRONTAB
            if (cmd[1] == SYSFCRONTAB_UID)
                user = SYSFCRONTAB;
            else {
#endif
                if ((pass = getpwuid((uid_t) cmd[1])) == NULL) {
                    warn_e("Unable to find passwd entry for %ld", cmd[1]);
                    Send_err_msg_end(fd, err_invalid_user_str);
                    return;
                }
                if (!is_root && strcmp(pass->pw_name, client->fcl_user) != 0) {
                    warn_e("%s is not allowed to see %s's jobs. %ld",
                           client->fcl_user, pass->pw_name);
                    Send_err_msg_end(fd, err_others_nallowed_str);
                    return;
                }
                user = pass->pw_name;
#ifdef SYSFCRONTAB
            }
#endif
        }

        /* list all jobs one by one and find the corresponding ones */
        switch (cmd[0]) {
        case CMD_LIST_JOBS:
            for (j = queue_base; j != NULL; j = j->j_next)
                Test_line(j->j_line, 0, 0, 0);
            break;

        case CMD_LIST_EXEQ:
            for (e = exe_list_first(exe_list); e != NULL;
                 e = exe_list_next(exe_list)) {
                if (e->e_line == NULL) {
                    if (is_root) {
                        send_msg_fd(fd, "job no more in an fcrontab: pid %d",
                                    e->e_job_pid);
                        found = 1;
                    }
                }
                else {
                    Test_line(e->e_line, e->e_job_pid, 0, 0);
                }
            }
            break;

        case CMD_LIST_LAVGQ:
            for (l = lavg_list_first(lavg_list); l != NULL;
                 l = lavg_list_next(lavg_list))
                Test_line(l->l_line, 0, 0, l->l_until);
            break;

        case CMD_LIST_SERIALQ:
            {
                int j;
                i = serial_array_index;
                for (j = 0; j < serial_num; j++) {
                    Test_line(serial_array[i], 0, j, 0);
                    if (++i >= serial_array_size)
                        i -= serial_array_size;
                }
                break;
            }

        }

        break;
    }

    if (!found)
        Send_err_msg(fd, err_job_nfound_str);
    /* to tell fcrondyn there's no more data to wait */
    Tell_no_more_data(fd);

}


void
cmd_on_exeq(struct fcrondyn_cl *client, long int *cmd, int fd, int is_root)
/* common code to all cmds working on jobs in the exeq */
{
    int found = 0;
    char *err_str = NULL;
    exe_t *e = NULL;

    /* find the corresponding job */
    for (e = exe_list_first(exe_list); e != NULL; e = exe_list_next(exe_list)) {
        if (e->e_line != NULL && cmd[2] == e->e_line->cl_id) {

            found = 1;

            /* check if the request is valid */
            if (!is_root &&
                strcmp(client->fcl_user, e->e_line->cl_file->cf_user) != 0) {

                if (cmd[0] == CMD_RENICE)
                    err_str = "%s tried to renice to %ld job id %ld for %s : "
                        "not allowed.";
                else if (cmd[0] == CMD_SEND_SIGNAL)
                    err_str = "%s tried to send signal %ld to id %ld for %s : "
                        "not allowed.";
                else
                    err_str = "cannot run unknown cmd with arg %ld on job id "
                        "%ld for %s : not allowed.";

                warn(err_str, client->fcl_user, cmd[1], cmd[2],
                     client->fcl_user);
                Send_err_msg_end(fd, err_job_nfound_str);
            }
            else {
                /* request is valid : do it */

                if (cmd[0] == CMD_SEND_SIGNAL)
                    cmd_send_signal(client, cmd, fd, e);
                else if (cmd[0] == CMD_RENICE)
                    cmd_renice(client, cmd, fd, e, is_root);
                else {
                    Send_err_msg_end(fd, err_cmd_unknown_str);
                    exe_list_end_iteration(exe_list);
                    return;
                }
            }
        }
    }

    if (!found) {

        if (cmd[0] == CMD_RENICE)
            err_str = "cannot renice job id %ld for %s : no corresponding "
                "running job.";
        else if (cmd[0] == CMD_SEND_SIGNAL)
            err_str = "cannot send signal to job id %ld for %s :"
                " no corresponding running job.";
        else
            err_str = "cannot run unknown cmd on job id %ld for %s :"
                " no corresponding running job.";

        warn(err_str, cmd[2], client->fcl_user);
        Send_err_msg_end(fd, err_rjob_nfound_str);
    }
    else {
        Tell_no_more_data(fd);
    }

}


void
cmd_renice(struct fcrondyn_cl *client, long int *cmd, int fd, exe_t * e,
           int is_root)
/* change nice value of a running job */
{

#ifdef HAVE_SETPRIORITY
    /* check if arguments are valid */
    if (e->e_job_pid <= 0 || ((int)cmd[1] < 0 && !is_root)
        || (int)cmd[1] > 20 || (int)cmd[1] < -20) {
        warn("renice: invalid args : pid: %d nice_value: %d user: %s.",
             e->e_job_pid, (int)cmd[1], client->fcl_user);
        Send_err_msg_end(fd, err_invalid_args_str);
        return;
    }

    /* ok, now setpriority() the job */
    if (setpriority(PRIO_PROCESS, e->e_job_pid, (int)cmd[1]) != 0) {
        error_e("could not setpriority(PRIO_PROCESS, %d, %d)",
                e->e_job_pid, (int)cmd[1]);
        Send_err_msg_end(fd, err_unknown_str);
        return;
    }
    else {
        send_msg_fd(fd, "Command successfully completed on process %d.",
                    e->e_job_pid);
        return;
    }

#else                           /* HAVE_SETPRIORITY */
    warn("System has no setpriority() : cannot renice. pid: %d nice_value: %d user: %s.", e->e_job_pid, (int)cmd[1], client->fcl_user);
    Send_err_msg_end(fd, err_cmd_unknown_str);

#endif                          /* HAVE_SETPRIORITY */
}


void
cmd_send_signal(struct fcrondyn_cl *client, long int *cmd, int fd, exe_t * e)
/* send a signal to a running job */
{
    if (e->e_job_pid <= 0 || (int)cmd[1] <= 0) {
        warn("send_signal: invalid args : pid: %d signal: %d user: %s",
             e->e_job_pid, (int)cmd[1], client->fcl_user);
        Send_err_msg_end(fd, err_invalid_args_str);
        return;
    }

    /* ok, now kill() the job */
    if (kill(e->e_job_pid, (int)cmd[1]) != 0) {
        error_e("could not kill(%d, %d)", e->e_job_pid, (int)cmd[1]);
        Send_err_msg_end(fd, err_unknown_str);
        return;
    }
    else {
        send_msg_fd(fd, "Command successfully completed on process %d.",
                    e->e_job_pid);
        return;
    }
}


void
cmd_run(struct fcrondyn_cl *client, long int *cmd, int fd, int is_root)
    /* Run a job and rescheduled if requested  */
{

    struct job_t *j = NULL;

    for (j = queue_base; j != NULL; j = j->j_next) {
        if (cmd[1] == j->j_line->cl_id) {
            if (strcmp(client->fcl_user, j->j_line->cl_file->cf_user) == 0
                || is_root) {

                if (is_lavg(j->j_line->cl_option))
                    add_lavg_job(j->j_line, fd);
                else if (is_serial(j->j_line->cl_option))
                    add_serial_job(j->j_line, fd);
                else
                    run_normal_job(j->j_line, fd);

                if (cmd[0] == CMD_RUNNOW)
                    set_next_exe(j->j_line, FROM_CUR_NEXTEXE, fd);

                Tell_no_more_data(fd);

                return;

            }
        }
    }

    /* we don't come here if a job has been found */
    Send_err_msg_end(fd, err_job_nfound_str);

}

void
exe_cmd(struct fcrondyn_cl *client)
    /* read command, and call corresponding function */
{
    long int *cmd;
    int fd;
    int is_root = 0;

    is_root = (strcmp(client->fcl_user, ROOTNAME) == 0) ? 1 : 0;

    /* to make things clearer (avoid repeating client->fcl_ all the time) */
    cmd = client->fcl_cmd;
    fd = client->fcl_sock_fd;

    /* */
    debug("exe_cmd [0,1,2] : %d %d %d", cmd[0], cmd[1], cmd[2]);
    /* */

    switch (cmd[0]) {

    case CMD_SEND_SIGNAL:
    case CMD_RENICE:
        cmd_on_exeq(client, cmd, fd, is_root);
        break;

    case CMD_DETAILS:
    case CMD_LIST_JOBS:
    case CMD_LIST_LAVGQ:
    case CMD_LIST_SERIALQ:
    case CMD_LIST_EXEQ:
        cmd_ls(client, cmd, fd, is_root);
        break;

    case CMD_RUN:
    case CMD_RUNNOW:
        cmd_run(client, cmd, fd, is_root);
        break;

    default:
        Send_err_msg_end(fd, err_cmd_unknown_str);
    }
}

void
remove_connection(struct fcrondyn_cl **client, struct fcrondyn_cl *prev_client)
/* close the connection, remove it from the list 
and make client points to the next entry */
{
    debug("closing connection : fd : %d", (*client)->fcl_sock_fd);
    shutdown((*client)->fcl_sock_fd, SHUT_RDWR);
    remove_from_select_set((*client)->fcl_sock_fd, &master_set, &set_max_fd);
    xclose_check(&((*client)->fcl_sock_fd), "client fd");
    if (prev_client == NULL) {
        fcrondyn_cl_base = (*client)->fcl_next;
        Free_safe((*client)->fcl_user);
        Free_safe(*client);
        *client = fcrondyn_cl_base;
    }
    else {
        prev_client->fcl_next = (*client)->fcl_next;
        Free_safe((*client)->fcl_user);
        Free_safe(*client);
        *client = prev_client->fcl_next;
    }
    fcrondyn_cl_num -= 1;
}

void
check_socket(int num)
    /* check for new connection, command, connection closed */
{
    int fd = -1, avoid_fd = -1;
    socklen_t addr_len = sizeof(struct sockaddr_un);
    struct sockaddr_un client_addr;
    long int buf_int[SOCKET_MSG_LEN];
    int read_len = 0;
    struct fcrondyn_cl *client = NULL, *prev_client = NULL;

    if (num <= 0)
        /* no socket to check : go directly to the end of that function */
        goto final_settings;

    debug("Checking socket ...");

    if (FD_ISSET(listen_fd, &read_set)) {
        debug("got new connection ...");
        fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (fd == -1) {
            error_e
                ("could not accept new connection : isset(listen_fd = %d) = %d",
                 listen_fd, FD_ISSET(listen_fd, &read_set));
        }
        else {
            fcntl(fd, F_SETFD, 1);
            /* set fd to O_NONBLOCK : we do not want fcron to be stopped on error, etc */
            if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1) {
                error_e
                    ("Could not set fd attribute O_NONBLOCK : connection rejected.");
                shutdown(fd, SHUT_RDWR);
                xclose_check(&fd, "client fd");
            }
            else {
                Alloc(client, fcrondyn_cl);
                client->fcl_sock_fd = fd;
                /* means : not authenticated yet : */
                client->fcl_user = NULL;
                client->fcl_cmd = NULL;

                /* include new entry in client list */
                client->fcl_next = fcrondyn_cl_base;
                fcrondyn_cl_base = client;
                client->fcl_idle_since = now;
                /* to avoid trying to read from it in this call */
                avoid_fd = fd;

                add_to_select_set(fd, &master_set, &set_max_fd);
                fcrondyn_cl_num += 1;

                debug("Added connection fd : %d - %d connections", fd,
                      fcrondyn_cl_num);

#ifdef SO_PEERCRED
                auth_client_so_peercred(client);
#elif defined(HAVE_GETPEERUCRED) || defined(HAVE_GETPEEREID)
                auth_client_getpeer(client);
#endif                          /* SO_PEERCRED */
            }
        }
    }

    client = fcrondyn_cl_base;
    while (client != NULL) {
        if (!FD_ISSET(client->fcl_sock_fd, &read_set)
            || client->fcl_sock_fd == avoid_fd) {
            /* check if the connection has not been idle for too long ... */
            if (client->fcl_user == NULL
                && now - client->fcl_idle_since > MAX_AUTH_TIME) {
                warn("Connection with no auth for more than %ds : closing it.",
                     MAX_AUTH_TIME);
                remove_connection(&client, prev_client);
            }
            else if (now - client->fcl_idle_since > MAX_IDLE_TIME) {
                warn("Connection of %s is idle for more than %ds : closing it.",
                     client->fcl_user, MAX_IDLE_TIME);
                remove_connection(&client, prev_client);
            }
            else {
                /* nothing to do on this one ... check the next one */
                prev_client = client;
                client = client->fcl_next;
            }
            continue;
        }

        if ((read_len =
             recv(client->fcl_sock_fd, buf_int, sizeof(buf_int), 0)) <= 0) {
            if (read_len == 0) {
                /* connection closed by client */
                remove_connection(&client, prev_client);
            }
            else {
                error_e("error recv() from sock fd %d", client->fcl_sock_fd);
                prev_client = client;
                client = client->fcl_next;
            }
        }
        else {
            client->fcl_cmd_len = read_len;
            client->fcl_cmd = buf_int;
            if (client->fcl_user == NULL)
                /* not authenticated yet */
                auth_client_password(client);
            else {
                /* we've just read a command ... */
                client->fcl_idle_since = now;
                exe_cmd(client);
            }
            prev_client = client;
            client = client->fcl_next;
        }
    }

 final_settings:
    /* copy master_set in read_set, because read_set is modified by select() */
    read_set = master_set;
}


void
close_socket(void)
    /* close connections, close socket, remove socket file */
{
    struct fcrondyn_cl *client, *client_buf = NULL;

    if (listen_fd) {
        shutdown(listen_fd, SHUT_RDWR);
        xclose_check(&listen_fd, "listening fd");
        unlink(fifofile);

        client = fcrondyn_cl_base;
        while (client != NULL) {
            shutdown(client->fcl_sock_fd, SHUT_RDWR);
            xclose_check(&(client->fcl_sock_fd), "client fd");

            client_buf = client->fcl_next;
            Free_safe(client);
            fcrondyn_cl_num -= 1;
            client = client_buf;
        }
    }
}
