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
#include "subs.h"

uid_t
get_user_uid_safe(char *username)
    /* get the uid of user username, and die on error */
{
    struct passwd *pass;

    errno = 0;
    pass = getpwnam(username);
    if (pass == NULL) {
        die_e("Unable to get the uid of user %s (is user in passwd file?)",
              username);
    }

    return pass->pw_uid;

}

gid_t
get_group_gid_safe(char *groupname)
    /* get the gid of group groupname, and die on error */
{
    struct group *grp = NULL;

    errno = 0;
    grp = getgrnam(groupname);
    if (grp == NULL) {
        die_e("Unable to get the gid of group %s", groupname);
    }

    return grp->gr_gid;

}

#ifdef USE_SETE_ID
void
seteuid_safe(uid_t euid)
/* set the euid if different from the current one, and die on error */
{
    /* on BSD, one can only seteuid() to the real UID or saved UID,
     * and NOT the euid, so we don't call seteuid(geteuid()),
     * which is why we need to check if a change is needed */

    if (geteuid() != euid && seteuid(euid) != 0)
        die_e("could not change euid to %d", euid);

}

void
setegid_safe(gid_t egid)
/* set the egid if different from the current one, and die on error */
{
    /* on BSD, one can only setegid() to the real GID or saved GID,
     * and NOT the egid, so we don't call setegid(getegid()),
     * which is why we need to check if a change is needed */

    if (getegid() != egid && setegid(egid) != 0)
        die_e("could not change egid to %d", egid);

}
#endif                          /* def USE_SETE_ID */

#ifdef USE_SETE_ID
int
open_as_user(const char *pathname, uid_t openuid, gid_t opengid, int flags, ...)
/* Become user and call open(), then revert back to who we were.
 * NOTE: when flags & O_CREAT, the 5th argument is mode_t and must be set
 *       -- it is ignored otherwise */
{
    uid_t orig_euid = geteuid();
    gid_t orig_egid = getegid();
    struct stat s;
    int fd = -1;
    va_list ap;
    mode_t mode = (mode_t) 0;
    int saved_errno = 0;

    if (flags & O_CREAT) {
        va_start(ap, flags);
        mode =
            (sizeof(mode_t) < sizeof(int)) ? va_arg(ap, int) : va_arg(ap,
                                                                      mode_t);
        va_end(ap);
    }

    seteuid_safe(openuid);
    setegid_safe(opengid);

    if (flags & O_CREAT) {
        fd = open(pathname, flags, mode);
    }
    else
        fd = open(pathname, flags);

    saved_errno = errno;

    /* change the effective uid/gid back to original values */
    seteuid_safe(orig_euid);
    setegid_safe(orig_egid);

    /* if open() didn't fail make sure we opened a 'normal' file */
    if (fd >= 0) {

        if (fstat(fd, &s) < 0) {
            saved_errno = errno;
            error_e("open_as_user(): could not fstat %s", pathname);
            if (xclose(&fd) < 0)
                error_e("open_as_user: could not xclose() %s", pathname);
            fd = -1;
        }

        if (!S_ISREG(s.st_mode) || s.st_nlink != 1) {
            error_e("open_as_user(): file %s is not a regular file", pathname);
            if (xclose(&fd) < 0)
                error_e("open_as_user: could not xclose() %s", pathname);
            saved_errno = 0;
            fd = -1;
        }

    }

    errno = saved_errno;

    return fd;

}

#else                           /* def USE_SETE_ID */

int
open_as_user(const char *pathname, uid_t openuid, gid_t opengid, int flags, ...)
/* Become user and call open(), then revert back to who we were.
 * As seteuid() is not available on this system attempt to similate that behavior
 * as closely as possible.
 * NOTE: when flags & O_CREAT, the 5th argument is mode_t and must be set
 *       -- it is ignored otherwise */
{
    int fd = -1;
    struct stat s;
    va_list ap;
    mode_t mode = (mode_t) 0;
    int saved_errno = 0;

    if (flags & O_CREAT) {
        va_start(ap, flags);
        mode =
            (sizeof(mode_t) < sizeof(int)) ? va_arg(ap, int) : va_arg(ap,
                                                                      mode_t);
        va_end(ap);
    }

    /* In case a flag as O_TRUNC is set, we should test if the user
     * is allowed to open the file before we open it.
     * There will always be a risk of race-condition between the test
     * and the open but that's the best we can realistically do
     * without seteuid()... */
    if (stat(pathname, &s) == 0) {
        if (!
            (s.st_mode & S_IROTH || (s.st_uid == openuid && s.st_mode & S_IRUSR)
             || (s.st_gid == opengid && s.st_mode & S_IRGRP))) {
            error("open_as_user(): file %s does not pass the security test: "
                  "uid=%d gid=%d mode=%lo openuid=%d opengid=%d",
                  pathname, s.st_uid, s.st_gid, s.st_mode, openuid, opengid);
            errno = EACCES;
            return -1;
        }
    }
    else if (errno == ENOENT) {
        /* the file doesn't exist so no risk to truncate the wrong file! */
        ;
    }
    else {
        saved_errno = errno;
        error_e("open_as_user(): could not stat %s", pathname);
        errno = saved_errno;
        return -1;
    }

    if (flags & O_CREAT) {
        fd = open(pathname, flags, mode);
    }
    else
        fd = open(pathname, flags);

    if (fd < 0)
        /* we couldn't open the file */
        return fd;

    /* if open() didn't fail make sure we opened a 'normal' file */
    if (fstat(fd, &s) < 0) {
        saved_errno = errno;
        error_e("open_as_user(): could not fstat %s", pathname);
        goto err;
    }
    if (!S_ISREG(s.st_mode) || s.st_nlink != 1) {
        saved_errno = errno;
        error_e("open_as_user(): file %s is not a regular file", pathname);
        goto err;
    }

    /* we couldn't become openuid/opengid, so check manually if the user
     * is allowed to read that file
     * We do that again as a malicious user could have replaced the file
     * by another one (e.g. a link) between the stat() and the open() earlier */
    if (!(s.st_mode & S_IROTH || (s.st_uid == openuid && s.st_mode & S_IRUSR)
          || (s.st_gid == opengid && s.st_mode & S_IRGRP))) {
        error("open_as_user(): file %s does not pass the security test: "
              "uid=%d gid=%d mode=%lo openuid=%d opengid=%d",
              pathname, s.st_uid, s.st_gid, s.st_mode, openuid, opengid);
        saved_errno = EACCES;
        goto err;
    }

    /* if we created a new file, change the file ownership:
     * make it as it would be if we had seteuid()
     * NOTE: if O_CREAT was set without O_EXCL and the file existed before
     *       then we will end up changing the ownership even if the seteuid()
     *       version of that function wouldn't have. That shouldn't break
     *       anything though. */
    if ((flags & O_CREAT) && fchown(fd, openuid, opengid) != 0) {
        saved_errno = errno;
        error_e("Could not fchown %s to uid:%d gid:%d", pathname, openuid,
                opengid);
        goto err;
    }

    /* everything went ok: return the file descriptor */
    return fd;

 err:
    if (fd >= 0 && xclose(&fd) < 0)
        error_e("open_as_user: could not xclose() %s", pathname);
    errno = saved_errno;
    return -1;
}

#endif                          /* def USE_SETE_ID */

int
remove_as_user(const char *pathname, uid_t removeuid, gid_t removegid)
/* Become user and call remove(), then revert back to who we were */
{
    int rval = -1;
#ifdef USE_SETE_ID
    uid_t orig_euid = geteuid();
    gid_t orig_egid = getegid();

    seteuid_safe(removeuid);
    setegid_safe(removegid);
#endif                          /* def USE_SETE_ID */

    rval = remove(pathname);

#ifdef USE_SETE_ID
    seteuid_safe(orig_euid);
    setegid_safe(orig_egid);
#endif                          /* def USE_SETE_ID */

    return rval;
}

int
rename_as_user(const char *oldpath, const char *newpath, uid_t renameuid,
               gid_t renamegid)
/* Become user and call rename(), then revert back to who we were */
{
    int rval = -1;
#ifdef USE_SETE_ID
    uid_t orig_euid = geteuid();
    gid_t orig_egid = getegid();

    seteuid_safe(renameuid);
    setegid_safe(renamegid);
#endif                          /* def USE_SETE_ID */

    rval = rename(oldpath, newpath);

#ifdef USE_SETE_ID
    seteuid_safe(orig_euid);
    setegid_safe(orig_egid);
#endif                          /* def USE_SETE_ID */

    return rval;

}

int
remove_blanks(char *str)
    /* remove blanks at the the end of str */
    /* return the length of the new string */
{
    char *c = str;

    /* scan forward to the null */
    while (*c)
        c++;

    /* scan backward to the first character that is not a space */
    do {
        c--;
    }
    while (c >= str && isspace((int)*c));

    /* if last char is a '\n', we remove it */
    if (*c == '\n')
        *c = '\0';
    else
        /* one character beyond where we stopped above is where the null
         * goes. */
        *++c = '\0';

    /* return the new length */
    return (c - str);

}

int
strcmp_until(const char *left, const char *right, char until)
/* compare two strings up to a given char (copied from Vixie cron) */
/* Copyright 1988,1990,1993,1994 by Paul Vixie */
/* Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1997,2000 by Internet Software Consortium, Inc.  */
{
    while (*left != '\0' && *left != until && *left == *right) {
        left++;
        right++;
    }

    if ((*left == '\0' || *left == until)
        && (*right == '\0' || *right == until)) {
        return (0);
    }
    return (*left - *right);
}

int
get_word(char **str)
    /* make str point the next word and return word length */
{
    char *ptr;

    Skip_blanks(*str);
    ptr = *str;

    while ((isalnum((int)*ptr) || *ptr == '_' || *ptr == '-')
           && *ptr != '=' && !isspace((int)*ptr))
        ptr++;

    return (ptr - *str);
}

void
my_unsetenv(const char *name)
/* call unsetenv() if available, otherwise call putenv("var=").
 * Check for errors and log them. */
{

#ifdef HAVE_UNSETENV
    if (unsetenv(name) < 0)
        error_e("could not flush env var %s with unsetenv()", name);
#else
    char buf[PATH_LEN];
    snprintf(buf, sizeof(buf) - 1, "%s=", name);
    buf[sizeof(buf) - 1] = '\0';
    if (putenv(buf) < 0)
        error_e("could not flush env var %s with putenv()", name);
#endif

}

void
my_setenv_overwrite(const char *name, const char *value)
/* call setenv(x, x, 1) if available, otherwise call putenv() with the appropriate
 * constructed string.
 * Check for errors and log them. */
{

#ifdef HAVE_SETENV

    /* // */
    debug("Calling setenv(%s, %s, 1)", name, value);
    /* // */
    if (setenv(name, value, 1) != 0)
        error_e("setenv(%s, %s, 1) failed", name, value);

#else
    char buf[PATH_LEN];

    snprintf(buf, sizeof(buf) - 1, "%s=%s", name, value)

        /* The final \0 may not have been copied because of lack of space:
         * add it to make sure */
        buf[sizeof(buf) - 1] = '\0';

    /* // */
    debug("Calling putenv(%s)", buf);
    /* // */
    if (putenv(buf) != 0)
        error_e("putenv(%s) failed", buf);

#endif

}
