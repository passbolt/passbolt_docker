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


#ifndef __FCRONCONF_H__
#define __FCRONCONF_H__


/* global variables */

/* fcron.conf parameters */
extern char *fcronconf;
extern char *fcronallow;
extern char *fcrondeny;
extern char *fcrontabs;
extern char *pidfile;
extern char *fifofile;
extern char *editor;
extern char *shell;
extern char *sendmail;
/* end of global variables */

/* functions prototypes */
extern void read_conf(void);
extern void free_conf(void);

#endif                          /* __FCRONCONF_H__ */
