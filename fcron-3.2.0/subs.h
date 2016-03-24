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


#ifndef __SUBS_H__
#define __SUBS_H__


/* functions prototypes */
extern uid_t get_user_uid_safe(char *username);
extern gid_t get_group_gid_safe(char *groupname);
extern void seteuid_safe(uid_t euid);
extern void setegid_safe(uid_t egid);
extern int remove_as_user(const char *pathname, uid_t removeuid,
                          gid_t removegid);
extern int open_as_user(const char *pathname, uid_t openuid, gid_t opengid,
                        int flags, ...);
extern int rename_as_user(const char *oldpath, const char *newpath,
                          uid_t renameuid, gid_t renamegid);

extern int remove_blanks(char *str);
extern int strcmp_until(const char *left, const char *right, char until);
extern int get_word(char **str);
extern void my_unsetenv(const char *name);
extern void my_setenv_overwrite(const char *name, const char *value);

#endif                          /* __SUBS_H__ */
