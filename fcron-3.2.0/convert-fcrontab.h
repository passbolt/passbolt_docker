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


#ifndef __CONVERT_FCRONTAB_H__
#define __CONVERT_FCRONTAB_H__

/* We create a .h (even if empty !)  because we have a generic rule
   in Makefile to create .o files which needs the corresponding .h to exist. */

#include "global.h"

/* needed for parsing a conf file */
#include "fcronconf.h"

extern uid_t rootuid;
extern gid_t rootgid;

#endif                          /* __CONVERT_FCRONTAB_H__ */
