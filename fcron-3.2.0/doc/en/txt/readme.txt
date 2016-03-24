   #Fcron documentation Fcron: how and why? Fcron: how and why? How to
   install fcron

   Copyright © 2000-2014 Thibault Godouet Fcron 3.2.0          Web page :
                                                      http://fcron.free.fr
   _______________________________________________________________________

             Fcron documentation
   Prev Chapter 1. Fcron: how and why? Next
   _______________________________________________________________________

1.1. About fcron

1.1.1. What is fcron?

   Fcron is a scheduler. It aims at replacing Vixie Cron, so it implements
   most of its functionalities.

   But contrary to Vixie Cron, fcron does not need your system to be up 7
   days a week, 24 hours a day: it also works well with systems which are
   not running neither all the time nor regularly (contrary to
   anacrontab).

   In other words, fcron does both the job of Vixie Cron and anacron, but
   does even more and better :)) ...

   To do so, fcron allows you to use the standard mode in which you tell
   it to execute one command at a given date and hour and to make it run a
   command according to its time of execution, which is normally the same
   as system up time. For example:

     Run the task 'save /home/ directory' every 3h15 of system up time.

   and, of course, in order to make it really useful, the time remaining
   until next execution is saved each time the system is stopped. You can
   also say:

     run that command once between 2am and 5am

   which will be done if the system is running at any time in this
   interval.

   Fcron also includes a useful system of options, which can be applied
   either to every lines following the declaration or to a single line.
   Some of the supported options permit to:

     * run jobs one by one (fcrontab option serial),
     * set the max system load average value under which the job should be
       run (fcrontab option lavg),
     * set a nice value for a job (fcrontab option nice),
     * run jobs at fcron's startup if they should have been run during
       system down time (fcrontab option bootrun),
     * mail user to tell him a job has not run and why (fcrontab option
       noticenotrun),
     * a better management of the mailing of outputs ...

1.1.2. License

   Fcron is distributed under GPL license (please read the license in the
   gpl file).

1.1.3. Requirements

     * a Linux/Unix system
       Fcron should work on every POSIX system, but it has been developed
       on Mandrake Linux (so it should work without any problems on
       Redhat).
       Fcron has been reported to work correctly on:
          + Linux Mandrake
          + Linux Debian 3.0
          + LFS
            (take a look at the Beyond LFS book to find the installation
            informations).
          + FreeBSD 4.2
          + OpenBSD 2.8
          + NetBSD 2.0
          + Darwin/MacOS-X
          + Solaris 8
          + AIX 4.3.3
          + HP-UX 11.11
       but fcron should work on other OS as well. Yet, if you have
       troubles making it work on a POSIX system, please contact me at
       <fcron@free.fr>.
     * a running syslog (or you won't have any log)
     * a running mail system ( sendmail or postfix for example) (or users
       will not able to read their jobs output)
     * (optional) a PAM library.
     * (optional) a system with a working SE Linux environment.

1.1.4. Compilation and installation

   See the install file (either install.txt or install.html).

1.1.5. Configuration

   See the fcron(8), fcrontab(5) and fcrontab(1) manpages.

1.1.6. Bug reports, corrections, propositions...

   Please send me the description of any bug you happen to encounter
   (with, even better, the corresponding patch -:) and any propositions,
   congratulations or flames at <fcron@free.fr>

   Please contact Russell Coker directly for problems about SE Linux
   support at <russell@coker.com.au>, since he maintains this part of the
   code.
   _______________________________________________________________________

   Prev                Home                 Next
   Fcron: how and why?  Up  How to install fcron
