/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/neutrino.h>

#include "ipc_utils.h"

#ifdef IPC_SUPPORT_SCICLIENT
#include <ti/drv/sciclient/sciclient.h>
#include <ti/drv/ipc/ipc.h>
#endif

extern uint32_t gNumMsgs;
extern uint32_t gVerbose;
extern uint32_t gSkipRps;
extern uint32_t gTimeOut;
extern uint32_t gNegative;
extern uint32_t gThreadedSendReceive;

extern int32_t Ipc_echo_test(void);

#ifdef IPC_SUPPORT_SCICLIENT
void ipc_initSciclient()
{
  /* Sciclient initialization done by resource manager */

}
#endif

int main(int argc, char *argv[])
{
    int option;
    extern char *optarg;

    ThreadCtl(_NTO_TCTL_IO, 0);

    while ( (option = getopt(argc, argv, "n:t:vspm")) != -1)
    {
        switch (option)
        {
            case 'n':
                gNumMsgs = strtoul(optarg, 0, 0);
                break;
            case 't':
                gTimeOut = strtoul(optarg, 0, 0);
                if(gTimeOut < 0) {
                    gTimeOut = -1;
                    fprintf(stderr,"Warning, invalid option '-%c'. Test will wait forever\n",option);
                }
                break;
            case 'v':
                gVerbose++;
                break;
            case 's':
                gSkipRps = 1;
                break;
            case 'p':
                gNegative = 1;
                break;
            case 'm':
                gThreadedSendReceive = 1;
                break;
            default:
                fprintf(stderr,"Unsupported option '-%c'\n",option);
        }
    }

#ifdef IPC_SUPPORT_SCICLIENT
    ipc_initSciclient();
#endif

    return Ipc_echo_test();
}
