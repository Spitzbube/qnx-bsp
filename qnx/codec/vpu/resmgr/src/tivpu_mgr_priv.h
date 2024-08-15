/*
 * Copyright 2022, QNX Software Systems.
 * Copyright 2022, Texas Instruments Incorporated - http://www.ti.com/
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _TI_VPUMGR_PRIV_H_INCLUDED
#define _TI_VPUMGR_PRIV_H_INCLUDED

#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resmgr.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <errno.h>
#include <sys/procmgr.h>
#include <drvr/hwinfo.h>
#include <string.h>
#include <stdarg.h>



typedef struct enc_stats_s {
    uint32_t enc_frame_cnt;
} enc_stats_t;

typedef struct dec_stats_s {
    uint32_t dec_frame_cnt;
} dec_stats_t;


typedef struct ti_vpu_ocb {
    iofunc_ocb_t       ocb;
    pid_t              pid;
    bool               created;
    uint32_t           ch_id;
    pthread_mutex_t    lock;
    int32_t            nitems;
    iofunc_notify_t    notify[3];
    union {
        dec_stats_t d_stats;
        enc_stats_t e_stats;
    }codec_stats;
    //for log performance
    int                plog_fd;
    uint64_t           start_ts;
} ti_vpu_ocb_t;

#include "tivpu_codec.h"

#define MAX_CODEC_HANDLES 32 /* TODO: Makes sure this can take care of worst case sizes */


#endif /* _TI_VPUMGR_PRIV_H_INCLUDED */
