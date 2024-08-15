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
 * \brief    Main file for CPSW Test.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
typedef uint32_t (*cpsw_testFxn)();

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct CPSW_TESTS_s
{
    cpsw_testFxn  testFxn;
    char*         testName;
} CPSW_TESTS;

typedef struct PTP_CMD_s
{
    struct ifdrv ifd;
    ptp_time_t   time;
} PTP_CMD;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */
void print_help();
void list_tests();

uint32_t ptp_get_time_test();

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* Driver options */
static CPSW_TESTS gCpswTests[] = {
    { ptp_get_time_test, "ptp get time test"}
};

uint32_t gTestNum = 0;
char     gTestInterface[IFNAMSIZ] = "\0";

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */
void print_help()
{
    printf("For usage of cpsw_test, call \"use cpsw_test\"\n");
}

void list_tests()
{
    int i = 0;
    uint32_t maxTests = sizeof(gCpswTests)/ sizeof(CPSW_TESTS);

    printf("List of test to run:\n");
    for (i = 0;i < maxTests; i++)
    {
        printf("%d : %s\n", i+1, gCpswTests[i].testName);
    }
}

uint32_t ptp_get_time_test()
{
    int s;
    time_t t;
    PTP_CMD ptp_get_time;
    ptp_time_t *data = &ptp_get_time.time;
    int ret;
    uint32_t test_completed = 1;

    printf("PTP get time test - Every 10s for 20 times:\n");
    s = socket(AF_INET, SOCK_DGRAM, 0);

    strncpy(ptp_get_time.ifd.ifd_name, gTestInterface, sizeof(ptp_get_time.ifd.ifd_name));
    ptp_get_time.ifd.ifd_cmd = PTP_GET_TIME;
    ptp_get_time.ifd.ifd_data = data;
    ptp_get_time.ifd.ifd_len = sizeof(ptp_get_time.time);
    for (int i=0; i<20; i++)
    {
        ret = devctl(s, SIOCGDRVSPEC, &ptp_get_time, sizeof(ptp_get_time), NULL);
        if (ret == EOK)
        {
            t = (time_t) ptp_get_time.time.sec;
            printf(" CPTS Time = %ld.%ld\n UTC Time = %s\n",
                (long int)ptp_get_time.time.sec, (long int)ptp_get_time.time.nsec, ctime(&t));
            delay(10000);
        }
        else
        {
            fprintf(stderr,"devctl returned error: %d\n",ret);
            test_completed = 0;
            break;
        }
    }

    return test_completed;
}


int main(int argc, char *argv[])
{

    int option;
    extern char *optarg;
    uint32_t maxTests = sizeof(gCpswTests)/ sizeof(CPSW_TESTS);
    int ret;


    while ( (option = getopt(argc, argv, "hlt:i:")) != -1)
    {
        switch (option)
        {
            case 'i':
                strcpy(gTestInterface, optarg);
                break;
            case 't':
                gTestNum = strtoul(optarg, 0, 0);
                if (gTestNum > maxTests)
                {
                    fprintf(stderr,"Unsupported test number %d\n",gTestNum);
                    return 0;
                }
                break;
            case 'l':
                list_tests();
                return 0;
            case 'h':
                print_help();
                return 0;
            default:
                fprintf(stderr,"Unsupported option '-%c'\n",option);
                print_help();
                return 0;
        }
    }

    if (gTestInterface[0] == '\0')
    {
        fprintf(stderr,"Interface not provided \n");
        return 0;
    }
    else
    {
        printf("Using Interface %s\n", gTestInterface);
    }

    if (gTestNum)
    {
        printf("--------------------------------------------------\n");
        printf("Running test: %s\n", gCpswTests[gTestNum - 1].testName);
        printf("--------------------------------------------------\n");

        ret = gCpswTests[gTestNum - 1].testFxn();

        if (ret)
            printf("Test Completed.\n");
        else
            printf("Test failed.\n");
        printf("--------------------------------------------------\n");
    }

    return 0;
}
