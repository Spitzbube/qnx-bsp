/*
 *  Copyright (c) Texas Instruments Incorporated 2020-2021
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*!
 * \file     main.c
 *
 * \brief    Main file for PTP Test.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* QNX Include Files */
#include <hw/inout.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/neutrino.h>

#include <sys/socket.h>
#include <netdrvr/ptp.h>
#include <net/if.h>
#include <sys/sockio.h>
#include <devctl.h>
#include <time.h>
/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct PTP_CMD_s
{
    struct ifdrv ifd;
    ptp_time_t   time;
} PTP_CMD;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int main(void)
{
    int s;
    PTP_CMD ptp_get_time;
    ptp_time_t *data = &ptp_get_time.time;
    time_t t;

    printf("PTP Test Every 10s:\n");
    s = socket(AF_INET, SOCK_DGRAM, 0);

    strncpy(ptp_get_time.ifd.ifd_name, "an0", sizeof(ptp_get_time.ifd.ifd_name));
    ptp_get_time.ifd.ifd_cmd = PTP_GET_TIME;
    ptp_get_time.ifd.ifd_data = data;
    ptp_get_time.ifd.ifd_len = sizeof(ptp_get_time.time);
    for (int i=0; i<20; i++)
    {
        devctl(s, SIOCGDRVSPEC, &ptp_get_time, sizeof(ptp_get_time), NULL);
        t = (time_t) ptp_get_time.time.sec;
        printf(" CPTS Time = %ld.%ld\n UTC Time = %s\n",
                (long int)ptp_get_time.time.sec, (long int)ptp_get_time.time.nsec, ctime(&t));
        delay(10000);
    }
    printf("PTP Test Completed.\n");

    return 0;
}
