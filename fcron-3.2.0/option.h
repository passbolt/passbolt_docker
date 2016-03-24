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


/* This has been inspired from bitstring(3) : here is the original copyright :
 */

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Vixie.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */


/* read and set options of a line */

/* WARNING : do not change any option number, nor remove any option, nor change
 *           the meaning of a value (i.e. bit set to 1 -> true).
 *           It can lead to errors with fcrontabs already saved to disk
 *           or loaded in memory.
 */

/* WARNING : if you add some options, make sure that OPTION_SIZE is big
 *           enough in global.h */

/*
  
  The options are :
  
  bit:   option:
  0      is this job based on time and date or system up time ?
  1      is this job based on system load average ?
  2      perform a logic OR or a logic AND between load averages ?
  3      perform a logic OR or a logic AND between week day and month day ?
  4      should we run this job at fcron startup if it should have been
           executed during system down?
  5      should this job be run serially ?
  6      should this job be run serially only once (for bootrun) ?
  7      does the output have to be mailed to user ?
  8      does the output (even if zero-length) must be mailed to user ?
  9      if time of execution is exceeded, exec the lavg job or not ?
  10     can this job be executed several times simultaneously
  11     can this job be put several times in the serial queue simultaneously
  12     can this job be put several times in the lavg queue simultaneously
  13     should mins field be ignored in goto_non_matching() ?
  14     should hrs field be ignored in goto_non_matching() ?
  15     should days field be ignored in goto_non_matching() ?
  16     should mons field be ignored in goto_non_matching() ?
  17     should dow field be ignored in goto_non_matching() ?
  18     First freq is the freq (*ly) or the first field to take into account ?
  19     Freq (ie daily) is from middle to middle of period (ie nightly) ?
  20     Should we remove a %-job from lavg queue if the interval is exceeded ?
  21     Should user be mailed if a %-job has not run during a period ?
  22     Should fcron log everything about this job or just errors ?
  23     Should this job be run asap, or randomly in its allowed interval of execution ?
  24     Should first value be applied at each fcron startup, or before line first exe ?
  25     if fcron is running in foreground, print jobs output to stderr/stdout or mail ?
  26     should the output of the job be emailed to the user only non-zero exit status ?
  27     rebootreset: if set then apply option first at each system startup
  28     runatreboot: if set then run the job at each system startup
  29     runonce: if set then run the job only once
  30     hasrun: set if the job has been run at least once

*/

#ifndef __OPTIONH__
#define __OPTIONH__

/* we need to include this to get some default values */
#include "config.h"

/* internal macros */

#define	_bit_byte(bit) \
	((bit) >> 3)

#define	_bit_set(opt, bit) \
	((opt)[_bit_byte(bit)] |= (1 << ((bit)&0x7)))

#define	_bit_test(opt, bit) \
	((opt)[_bit_byte(bit)] & (1 << ((bit)&0x7)))

#define	_bit_clear(opt, bit) \
	((opt)[_bit_byte(bit)] &= ~(1 << ((bit)&0x7)))


/* external macros */

/* default value generally corresponds to a bit value of 0 : if you want to
 * change the default value of an option, do it by modifying the following
 * macro (set to 1 the needed bits) */
#define set_default_opt(opt)  \
       { \
          if ( SERIAL_ONCE >= 1 ) clear_serial_sev(opt); \
          if ( LAVG_ONCE == 0 ) set_lavg_sev(opt); \
       }


/*
  bit 0 : set to 1 : line is based on system up time
          set to 0 : line is based on time and date
*/
#define	is_freq(opt) \
	(_bit_test(opt, 0))
#define is_td(opt) \
        ( ! _bit_test(opt, 0))
#define	set_freq(opt) \
	(_bit_set(opt, 0))
#define set_td(opt) \
	(_bit_clear(opt, 0))


/*
  bit 1 : set to 1 : line based on system load average
          set to 0 : line doesn't take care of load average
*/
#define	is_lavg(opt) \
	(_bit_test(opt, 1))
#define	set_lavg(opt) \
	(_bit_set(opt, 1))
#define clear_lavg(opt) \
	(_bit_clear(opt, 1))


/*
  bit 2 : set to 1 : perform a logic OR between load averages
          set to 0 : perform a logic AND between load averages
*/
#define	is_lor(opt) \
	(_bit_test(opt, 2))
#define	is_land(opt) \
	( ! _bit_test(opt, 2))
#define	set_lor(opt) \
	(_bit_set(opt, 2))
#define	set_land(opt) \
	(_bit_clear(opt, 2))

/*
  bit 3 : set to 1 : perform a logic OR between week day and month day
          set to 0 : perform a logic AND between week day and month day
*/
#define	is_dayor(opt) \
	(_bit_test(opt, 3))
#define	is_dayand(opt) \
	( ! _bit_test(opt, 3))
#define	set_dayor(opt) \
	(_bit_set(opt, 3))
#define set_dayand(opt) \
	(_bit_clear(opt, 3))


/*
  bit 4 : set to 1 : run this line at fcron's startup if it should have been
                     executed during system down
          set to 0 : do not run it at fcron's startup
*/
#define	is_bootrun(opt) \
	(_bit_test(opt, 4))
#define	set_bootrun(opt) \
	(_bit_set(opt, 4))
#define clear_bootrun(opt) \
	(_bit_clear(opt, 4))


/*
  bit 5 : set to 1 : run this line serially
          set to 0 : do not run it serially
*/
#define	is_serial(opt) \
	(_bit_test(opt, 5))
#define	set_serial(opt) \
	(_bit_set(opt, 5))
#define clear_serial(opt) \
	(_bit_clear(opt, 5))


/*
  bit 6 : set to 1 : job is being serialized once
          set to 0 : job is not being serialized once
*/
#define	is_serial_once(opt) \
	(_bit_test(opt, 6))
#define	set_serial_once(opt) \
	(_bit_set(opt, 6))
#define clear_serial_once(opt) \
	(_bit_clear(opt, 6))


/*
  bit 7 : set to 1 : do not mail output
          set to 0 : mail output to user
*/
#define	is_mail(opt) \
	( ! _bit_test(opt, 7))
#define	set_mail(opt) \
	(_bit_clear(opt, 7))
#define clear_mail(opt) \
	(_bit_set(opt, 7))


/*
  bit 8 : set to 1 : mail output even if it is zero-length to user
          set to 0 : mail output only if it is non-zero length
*/
#define	is_mailzerolength(opt) \
	(_bit_test(opt, 8))
#define	set_mailzerolength(opt) \
	(_bit_set(opt, 8))
#define clear_mailzerolength(opt) \
	(_bit_clear(opt, 8))


/*
  bit 9 : set to 1 : exec the job now if time of execution is exceeded
          set to 0 : do not exec the job if time of execution is exceeded
*/
#define	is_run_if_late(opt) \
	(_bit_test(opt, 9))
#define	set_run_if_late(opt) \
	(_bit_set(opt, 9))
#define clear_run_if_late(opt) \
	(_bit_clear(opt, 9))


/*
  bit 10 : set to 1 : line can be executed several times simultaneously
           set to 0 : line can only be executed once simultaneously
*/
#define	is_exe_sev(opt) \
	(_bit_test(opt, 10))
#define	set_exe_sev(opt) \
	(_bit_set(opt, 10))
#define clear_exe_sev(opt) \
	(_bit_clear(opt, 10))


/*
  bit 11 : set to 1 : can only be put once in serial queue simultaneously
           set to 0 : can be put several times in serial queue simultaneously
*/
#define	is_serial_sev(opt) \
	( ! _bit_test(opt, 11))
#define	set_serial_sev(opt) \
	(_bit_clear(opt, 11))
#define clear_serial_sev(opt) \
	(_bit_set(opt, 11))


/*
  bit 12 : set to 1 : can only be put once in lavg queue simultaneously
           set to 0 : can be put several times in lavg queue simultaneously
*/
#define	is_lavg_sev(opt) \
	(_bit_test(opt, 12))
#define	set_lavg_sev(opt) \
	(_bit_set(opt, 12))
#define clear_lavg_sev(opt) \
	(_bit_clear(opt, 12))


/*
  bit 13 : set to 1 : mins field is the limit
           set to 0 : mins field is not the limit
*/
#define	is_freq_mins(opt) \
	(_bit_test(opt, 13))
#define	set_freq_mins(opt) \
	(_bit_set(opt, 13))
#define clear_freq_mins(opt) \
	(_bit_clear(opt, 13))


/*
  bit 14 : set to 1 : hrs field is the limit
           set to 0 : hrs field is not the limit
*/
#define	is_freq_hrs(opt) \
	(_bit_test(opt, 14))
#define	set_freq_hrs(opt) \
	(_bit_set(opt, 14))
#define clear_freq_hrs(opt) \
	(_bit_clear(opt, 14))


/*
  bit 15 : set to 1 : days field is the limit
           set to 0 : days field is not the limit
*/
#define	is_freq_days(opt) \
	(_bit_test(opt, 15))
#define	set_freq_days(opt) \
	(_bit_set(opt, 15))
#define clear_freq_days(opt) \
	(_bit_clear(opt, 15))


/*
  bit 16 : set to 1 : mons field is the limit
           set to 0 : mons field is not the limit
*/
#define	is_freq_mons(opt) \
	(_bit_test(opt, 16))
#define	set_freq_mons(opt) \
	(_bit_set(opt, 16))
#define clear_freq_mons(opt) \
	(_bit_clear(opt, 16))


/*
  bit 17 : set to 1 : dow field is the limit
           set to 0 : dow field is not the limit
*/
#define	is_freq_dow(opt) \
	(_bit_test(opt, 17))
#define	set_freq_dow(opt) \
	(_bit_set(opt, 17))
#define clear_freq_dow(opt) \
	(_bit_clear(opt, 17))


/*
  bit 18 : set to 1 : limit field is freq to run the line (once a hour, etc)
           set to 0 : run once per interval of the limit field
*/
#define	is_freq_periodically(opt) \
	(_bit_test(opt, 18))
#define	set_freq_periodically(opt) \
	(_bit_set(opt, 18))
#define clear_freq_periodically(opt) \
	(_bit_clear(opt, 18))


/*
  bit 19 : set to 1 : run once from mid-period to mid-period (i.e. nightly)
           set to 0 : run once from begin to the end of period (i.e. daily)
*/
#define	is_freq_mid(opt) \
	(_bit_test(opt, 19))
#define	set_freq_mid(opt) \
	(_bit_set(opt, 19))
#define clear_freq_mid(opt) \
	(_bit_clear(opt, 19))


/*
  bit 20 : set to 1 : let the job in the %-queue if interval is exceeded
           set to 0 : remove %-job from lavg queue if interval is exceeded
*/
#define	is_strict(opt) \
	( ! _bit_test(opt, 20))
#define	set_strict(opt) \
	(_bit_clear(opt, 20))
#define clear_strict(opt) \
	(_bit_set(opt, 20))


/*
  bit 21 : set to 1 : mail user if a job has not run during a period
           set to 0 : do not mail user if a job has not run during a period
*/
#define	is_notice_notrun(opt) \
	(_bit_test(opt, 21))
#define	set_notice_notrun(opt) \
	(_bit_set(opt, 21))
#define clear_notice_notrun(opt) \
	(_bit_clear(opt, 21))


/*
  bit 22 : set to 1 : do not log normal activity of this job (only errors)
           set to 0 : log everything
*/
#define	is_nolog(opt) \
	(_bit_test(opt, 22))
#define	set_nolog(opt) \
	(_bit_set(opt, 22))
#define clear_nolog(opt) \
	(_bit_clear(opt, 22))


/*
  bit 23 : set to 1 : run this job at a random time in its allowed interval of execution.
           set to 0 : run this job asap (safer)
*/
#define	is_random(opt) \
	(_bit_test(opt, 23))
#define	set_random(opt) \
	(_bit_set(opt, 23))
#define clear_random(opt) \
	(_bit_clear(opt, 23))


/*
  bit 24 : set to 1 : "volatile" system up time, i.e. restart counting each time fcron
                      is started
           set to 0 : continue counting uptime where last fcron instance left of
*/
#define	is_volatile(opt) \
	(_bit_test(opt, 24))
#define	set_volatile(opt) \
	(_bit_set(opt, 24))
#define clear_volatile(opt) \
	(_bit_clear(opt, 24))

/*
  bit 25 : set to 1 : if fcron is running in the forground, then also let jobs print
                      to stderr/stdout instead of mailing or discarding it
           set to 0 : if fcron is not running in the foreground or this bit is not
	              set, then treat it as specified with the other options
*/
#define	is_stdout(opt) \
	(_bit_test(opt, 25))
#define	set_stdout(opt) \
	(_bit_set(opt, 25))
#define clear_stdout(opt) \
	(_bit_clear(opt, 25))

/*
  bit 26 : set to 1 : The ouput of a job will only be emailed to the user if the job
                      exited with a non-zero status.
           set to 0 : The exit-status of the job won't be taken into account to decide
                      if the output should be emailed to the user.
*/
#define	is_erroronlymail(opt) \
	(_bit_test(opt, 26))
#define	set_erroronlymail(opt) \
	(_bit_set(opt, 26))
#define clear_erroronlymail(opt) \
	(_bit_clear(opt, 26))

/*
  bit 27 : set to 1 : at each system startup, run the job after the delay set
                      in the option first
           set to 0 : leave the nextexe time as it
*/
#define	is_rebootreset(opt) \
	(_bit_test(opt, 27))
#define	set_rebootreset(opt) \
	(_bit_set(opt, 27))
#define clear_rebootreset(opt) \
	(_bit_clear(opt, 27))

/*
  bit 28 : set to 1 : run the job immediately after the system startup
           set to 0 : leave the nextexe time as it
*/
#define	is_runatreboot(opt) \
	(_bit_test(opt, 28))
#define	set_runatreboot(opt) \
	(_bit_set(opt, 28))
#define clear_runatreboot(opt) \
	(_bit_clear(opt, 28))

/*
  bit 29 : set to 1 : run the job only once until next system reboot
                      (or next fcron restart if volatile is set)
           set to 0 : don't limit the number of times the job is run
*/
#define	is_runonce(opt) \
	(_bit_test(opt, 29))
#define	set_runonce(opt) \
	(_bit_set(opt, 29))
#define clear_runonce(opt) \
	(_bit_clear(opt, 29))

/*
  bit 30 : set to 1 : the job has run at least once since system reboot
                      (or since fcron restart if volatile is set)
           set to 0 : job hasn't run yet
*/
#define	is_hasrun(opt) \
	(_bit_test(opt, 30))
#define	set_hasrun(opt) \
	(_bit_set(opt, 30))
#define clear_hasrun(opt) \
	(_bit_clear(opt, 30))

#endif                          /* __OPTIONH__ */
