/* 
 *  gloadavg.c - get load average for Linux
 *  Copyright (C) 1993  Thomas Koenig
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _POSIX_SOURCE           /* Don't redefine if already exists */
#define _POSIX_SOURCE 1
#endif

#include "fcron.h"

/* Local headers */

#include "getloadavg.h"

/* Global functions */

#ifdef HAVE_KSTAT

#include <kstat.h>

int
getloadavg(double *result, int n)
/* return the current load average as a floating point number,
 * the number of load averages read, or <0 for error
 */
{
    kstat_ctl_t *kc;
    kstat_t *ksp;
    kstat_named_t *knm;
    kstat_named_t knm_buf[20];
    int cnt;
    int ret;

    if (n != 3) {
        return -1;
    }

    ret = 0;

    kc = kstat_open();
    for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next) {
        if (strcmp(ksp->ks_name, "system_misc") == 0) {
            kstat_read(kc, ksp, &knm_buf);
            for (cnt = 0; cnt < ksp->ks_ndata; cnt++) {
                knm = &knm_buf[cnt];
                if (strcmp(knm->name, "avenrun_1min") == 0) {
                    result[0] = knm->value.ui32 / 256.0;
                    ret++;
                }
                else if (strcmp(knm->name, "avenrun_5min") == 0) {
                    result[1] = knm->value.ui32 / 256.0;
                    ret++;
                }
                else if (strcmp(knm->name, "avenrun_15min") == 0) {
                    result[2] = knm->value.ui32 / 256.0;
                    ret++;
                }
            }
        }
    }
    kstat_close(kc);

    return ret;
}

#else                           /* def HAVE_KSTAT */

int
getloadavg(double *result, int n)
/* return the current load average as a floating point number,
 * the number of load averages read, or <0 for error
 */
{
    FILE *fp;
    int i;
    char loadavg_path = PROC "/loadavg";

    if (n > 3)
        n = 3;

    if ((fp = fopen(loadavg_path, "r")) == NULL) {
        error_e("could not open '%s' (make sure procfs is mounted)",
                loadavg_path);
        i = -1;
    }
    else {
        for (i = 0; i < n; i++) {
            if (fscanf(fp, "%lf", result) != 1)
                goto end;
            result++;
        }
    }
 end:
    xfclose_check(&fp, loadavg_path);
    return (i < 0) ? i : i;
}

#endif                          /* def HAVE_KSTAT */
