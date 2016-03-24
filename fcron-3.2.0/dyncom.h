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


/* This file describe the communication protocol between fcron and fcrondyn */

#ifndef __DYNCOM_H__
#define __DYNCOM_H__

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

/* string which means : "No more data to read" */
/* Warning : should be a short string, otherwise we may need more tests in talk_fcron()
 * (because the string might be recv()ed in 2 pieces) */
#define END_STR "\0\0"

/* arg types */
#define USER 1
#define JOBID 2
#define TIME_AND_DATE 3
#define NICE_VALUE 4
#define SIGNAL 5
#define BOOLEAN 6

#define ALL (-1)
#define CUR_USER (-2)
#define ARG_REQUIRED (-3)

#ifdef SYSFCRONTAB
#define SYSFCRONTAB_UID (-100)
#endif

/* commands : if you change something here, please update fcrondyn.c's cmd_list
 *            and fcron's socket.c . */

#define CMD_LIST_JOBS 101
#define CMD_LIST_LAVGQ 102
#define CMD_LIST_SERIALQ 103
#define CMD_LIST_EXEQ 104
#define CMD_DETAILS 105

#define CMD_RESCHEDULE 201

#define CMD_RUNNOW 301
#define CMD_RUN 302

#define CMD_SEND_SIGNAL 401
#define CMD_RENICE 402

#endif                          /* __DYNCOM_H__ */
