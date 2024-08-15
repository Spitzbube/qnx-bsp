/*
 *  Copyright (c) Texas Instruments Incorporated 2018-2021
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

/**
 *  \file sem_test.c
 *
 *  \brief Test code for semphore osal api
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <hw/inout.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <unistd.h>
#include <pthread.h>
#include "ti/osal/SemaphoreP.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#ifndef EOK
#define EOK             0
#endif
#ifndef EERROR
#define EERROR          -1
#endif

#ifndef EXIT_SUCCESS
#define	EXIT_SUCCESS	0
#endif

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
SemaphoreP_Params sem_params;
SemaphoreP_Handle *semHandle;
static int g_exit = 0;

uint32_t gtest = 0;
uint32_t gVerbose = 0;

struct threadResults_t {
    uint32_t t1, t2, t3;
};

typedef struct threadResults_t ThreadResults;

//Test results have been verified to be consistent.
const ThreadResults gExpectedPends[] = {
    {3, 2, 0},
    {2, 2, 2},
    {3, 2, 0},
    {2, 2, 2},
    {0, 0, 0},
    {1, 0, 0},
    {3, 0, 0}
};

ThreadResults gThreadPends = {0, 0, 0};

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

int32_t printPendResults(uint32_t testNumber)
{
    int32_t status = EERROR;

    //The tests numbers are 1 indexed, -= 1 for 0 indexing
    testNumber -= 1;

    printf("Results:\nThread 1: %u\nThread 2: %u\nThread 3: %u\n\n",
           gThreadPends.t1,
           gThreadPends.t2,
           gThreadPends.t3);

    if ((testNumber < (sizeof(gExpectedPends)/sizeof(ThreadResults))) &&
        (gThreadPends.t1 == gExpectedPends[testNumber].t1) &&
        (gThreadPends.t2 == gExpectedPends[testNumber].t2) &&
        (gThreadPends.t3 == gExpectedPends[testNumber].t3))
    {
        status = EOK;
        printf("Test passed!\n");
    }

    if ((status == EERROR) &&
        (testNumber < sizeof(gExpectedPends)/sizeof(ThreadResults)))
    {
        printf("ERROR - Expected thread pends:\nThread 1: %u\nThread2: %u\nThread 3: %u\n\n",
               gExpectedPends[testNumber].t1,
               gExpectedPends[testNumber].t2,
               gExpectedPends[testNumber].t3);
    }

    gThreadPends = (ThreadResults){0, 0, 0};

    return status;
}

void* thread1( void* arg )
{
    SemaphoreP_Status ret_code;

    while (g_exit == 0) {
        printf("thread1: Wait to Acquired\n" );
        ret_code = SemaphoreP_pend(semHandle, SemaphoreP_WAIT_FOREVER);
        if (ret_code == EOK)
        {
            gThreadPends.t1++;
            printf("thread1: Acquired\n" );
            sleep(1);
            ret_code = SemaphoreP_post(semHandle);
            if (ret_code != EOK)
            {
                printf("thread1: SemaphoreP_post() failed: %d\n",ret_code);
            }
            printf("thread1: Released\n" );

        }
        else
        {
            printf("thread1: SemaphoreP_pend() failed: %d\n", ret_code);
        }

        /* Snooze for 1 second */
        sleep(1);
    }
    printf("thread1: exiting\n");
    return 0;
}

void* thread2( void* arg )
{
    SemaphoreP_Status ret_code;

    while (g_exit == 0) {
        printf("thread2: Wait to Acquired\n" );
        ret_code = SemaphoreP_pend(semHandle, SemaphoreP_WAIT_FOREVER);
        if (ret_code == EOK)
        {
            gThreadPends.t2++;
            printf("thread2: Acquired\n" );
            sleep(1);
            ret_code = SemaphoreP_post(semHandle);
            if (ret_code != EOK)
            {
                printf("thread2: SemaphoreP_post() failed: %d\n",ret_code);
            }
            printf("thread2: Released\n" );
        }
        else
        {
            printf("thread2:SemaphoreP_pend() failed: %d\n", ret_code);
        }

        /* Snooze for 1 seconds */
        sleep(1);
    }
    printf("thread2: exiting\n");
    return 0;
}

void* thread3( void* arg )
{
    SemaphoreP_Status ret_code;

    while (g_exit == 0) {
        printf("thread3: Wait to Acquired\n" );
        ret_code = SemaphoreP_pend(semHandle, SemaphoreP_WAIT_FOREVER);
        if (ret_code == EOK)
        {
            gThreadPends.t3++;
            printf("thread3: Acquired\n" );
            sleep(1);
            ret_code = SemaphoreP_post(semHandle);
            if (ret_code != EOK)
            {
                printf("thread3: SemaphoreP_post() failed: %d\n",ret_code);
            }
            printf("thread3: Released\n" );
        }
        else
        {
            printf("thread3:SemaphoreP_pend() failed: %d\n", ret_code);
        }

        /* Snooze for 1 seconds */
        sleep(1);
    }
    printf("thread3: exiting\n");
    return 0;
}

void* thread2_post( void* arg )
{
    SemaphoreP_Status ret_code;

    printf("thread2_post: Posting the semaphore\n" );
    ret_code = SemaphoreP_post(semHandle);
    if (ret_code != EOK)
    {
        printf("thread2_post: SemaphoreP_post() failed: %d\n",ret_code);
    }

    printf("thread2_post: exiting\n");
    return 0;
}

#define PRINT_HELP                                                \
        "\n Syntax:"                                              \
        "\n osal_sem_testapp [-h] [-t <test_no>]"                 \
        "\n"                                                      \
        "\n Options:"                                             \
        "\n  -t test no - 0 will run all test"                    \
        "\n"                                                      \
        "\n  1 - Counting Semaphore: with count 1 and 2 threads"  \
        "\n  2 - Counting Semaphore: with count 1 and 3 threads"  \
        "\n  3 - Binary Semaphore: with count 1 and 2 threads"    \
        "\n  4 - Binary Semaphore: with count 1 and 3 threads"    \
        "\n  5 - Binary Semaphore: with count 0 and 1 thread - no post"    \
        "\n  6 - Binary Semaphore: with count 0 and 1 thread - post from same thread"    \
        "\n  7 - Binary Semaphore: with count 0 and 1 thread - post from thread 2"    \
        "\n"

void print_help()
{
    printf("%s", PRINT_HELP);
}

int semtest()
{
    int ret_code;
    int32_t testStatus = EOK;

    if ((gtest == 0) || (gtest == 1))
    {
        ///// counting semaphore - start
        printf("Test 1: Testing Counting Semaphore: with count 1 and 2 threads\n");
        SemaphoreP_Params_init(&sem_params);
        g_exit = 0;

        semHandle = SemaphoreP_create(1, &sem_params);
        if (semHandle == NULL)
        {
            printf("Couldn't create first sem\n");
            return 1;
        }

        ret_code = pthread_create(NULL, NULL, &thread1, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create first thread: %d\n", ret_code);
        }
        ret_code = pthread_create(NULL, NULL, &thread2, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create second thread: %d\n", ret_code);
        }

        /* Let the threads run for 5 seconds. */
        sleep(5);
        printf("Sending exit command\n");
        g_exit = 1;
        sleep(2);
        SemaphoreP_delete(semHandle);
        semHandle = NULL;
        printf("Testing Binary Semaphore - Done\n");
        testStatus += printPendResults(1);
        ///////////////////////////// end
    }

    if ((gtest == 0) || (gtest == 2))
    {
        ///// counting semaphore - start
        printf("Test 2: Testing Counting Semaphore: with count 1 and 3 threads\n");
        SemaphoreP_Params_init(&sem_params);
        g_exit = 0;

        semHandle = SemaphoreP_create(1, &sem_params);
        if (semHandle == NULL)
        {
            printf("Couldn't create first sem\n");
            return 1;
        }

        ret_code = pthread_create(NULL, NULL, &thread1, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create first thread: %d\n", ret_code);
        }
        ret_code = pthread_create(NULL, NULL, &thread2, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create second thread: %d\n", ret_code);
        }
        ret_code = pthread_create(NULL, NULL, &thread3, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create third thread: %d\n", ret_code);
        }

        /* Let the threads run for 5 seconds. */
        sleep(5);
        printf("Sending exit command\n");
        g_exit = 1;
        sleep(3);
        SemaphoreP_delete(semHandle);
        semHandle = NULL;
        printf("Testing Binary Semaphore - Done\n");
        testStatus += printPendResults(2);
        ///////////////////////////// end
    }

    if ((gtest == 0) || (gtest == 3))
    {
        ///// binary semaphore - start
        printf("Test 3: Testing Binary Semaphore: with count 1 and 2 threads\n");
        SemaphoreP_Params_init(&sem_params);
        g_exit = 0;
        sem_params.mode = SemaphoreP_Mode_BINARY;

        semHandle = SemaphoreP_create(1, &sem_params);
        if (semHandle == NULL)
        {
            printf("Couldn't create first sem\n");
            return 1;
        }

        ret_code = pthread_create(NULL, NULL, &thread1, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create first thread: %d\n", ret_code);
        }
        ret_code = pthread_create(NULL, NULL, &thread2, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create second thread: %d\n", ret_code);
        }

        /* Let the threads run for 5 seconds. */
        sleep(5);
        printf("Sending exit command\n");
        g_exit = 1;
        sleep(3);
        SemaphoreP_delete(semHandle);
        semHandle = NULL;
        printf("Testing Binary Semaphore - Done\n");
        testStatus += printPendResults(3);
        ///////////////////////////// end
    }

    if ((gtest == 0) || (gtest == 4))
    {
        ///// binary semaphore - start
        printf("Test 4: Testing Binary Semaphore: with count 1 and 3 threads\n");
        SemaphoreP_Params_init(&sem_params);
        g_exit = 0;
        sem_params.mode = SemaphoreP_Mode_BINARY;

        semHandle = SemaphoreP_create(1, &sem_params);
        if (semHandle == NULL)
        {
            printf("Couldn't create first sem\n");
            return 1;
        }

        ret_code = pthread_create(NULL, NULL, &thread1, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create first thread: %d\n", ret_code);
        }
        ret_code = pthread_create(NULL, NULL, &thread2, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create second thread: %d\n", ret_code);
        }
        ret_code = pthread_create(NULL, NULL, &thread3, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create third thread: %d\n", ret_code);
        }

        /* Let the threads run for 5 seconds. */
        sleep(5);
        printf("Sending exit command\n");
        g_exit = 1;
        sleep(3);
        SemaphoreP_delete(semHandle);
        //sleep(1);
        semHandle = NULL;
        printf("Testing Binary Semaphore - Done\n");
        testStatus += printPendResults(4);
        ///////////////////////////// end
    }

    if ((gtest == 0) || (gtest == 5))
    {
        ///// binary semaphore - start
        printf("Test 5: Testing Binary Semaphore: with count 0 and 1 thread - no post\n");
        SemaphoreP_Params_init(&sem_params);
        g_exit = 0;
        sem_params.mode = SemaphoreP_Mode_BINARY;

        semHandle = SemaphoreP_create(0, &sem_params);
        if (semHandle == NULL)
        {
            printf("Couldn't create first sem\n");
            return 1;
        }

        ret_code = pthread_create(NULL, NULL, &thread1, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create first thread: %d\n", ret_code);
        }

        /* Let the threads run for 2 seconds. */
        sleep(2);
        printf("Sending exit command\n");
        g_exit = 1;
        sleep(3);
        SemaphoreP_delete(semHandle);
        semHandle = NULL;
        sleep(1);
        printf("Testing Binary Semaphore - Done\n");
        testStatus += printPendResults(5);
        ///////////////////////////// end
    }

    if ((gtest == 0) || (gtest == 6))
    {
        ///// binary semaphore - start
        printf("Test 6: Testing Binary Semaphore: with count 0 and 1 thread - post from parent thread\n");
        SemaphoreP_Params_init(&sem_params);
        g_exit = 0;
        sem_params.mode = SemaphoreP_Mode_BINARY;

        semHandle = SemaphoreP_create(0, &sem_params);
        if (semHandle == NULL)
        {
            printf("Couldn't create first sem\n");
            return 1;
        }

        printf("Posting to semaphore\n");
        SemaphoreP_post(semHandle);

        printf("Creating child thread\n");
        ret_code = pthread_create(NULL, NULL, &thread1, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create first thread: %d\n", ret_code);
        }

        /* Let the threads run for 2 seconds. */
        sleep(2);
        printf("Sending exit command\n");
        g_exit = 1;
        sleep(3);
        SemaphoreP_delete(semHandle);
        semHandle = NULL;
        printf("Testing Binary Semaphore - Done\n");
        testStatus += printPendResults(6);
        ///////////////////////////// end
    }

    if ((gtest == 0) || (gtest == 7))
    {
        ///// binary semaphore - start
        printf("Test 7: Testing Binary Semaphore: with count 0 and 2 thread - post from thread 2\n");
        SemaphoreP_Params_init(&sem_params);
        g_exit = 0;
        sem_params.mode = SemaphoreP_Mode_BINARY;

        semHandle = SemaphoreP_create(0, &sem_params);
        if (semHandle == NULL)
        {
            printf("Couldn't create first sem\n");
            return 1;
        }

        ret_code = pthread_create(NULL, NULL, &thread1, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create first thread: %d\n", ret_code);
        }

        ret_code = pthread_create(NULL, NULL, &thread2_post, NULL);
        if (ret_code != EOK)
        {
            printf("Couldn't create second thread: %d\n", ret_code);
        }

        /* Let the threads run for 5 seconds. */
        sleep(5);
        printf("Sending exit command\n");
        g_exit = 1;
        sleep(3);
        SemaphoreP_delete(semHandle);
        semHandle = NULL;
        printf("Testing Binary Semaphore - Done\n");
        testStatus += printPendResults(7);
        ///////////////////////////// end
    }

    return testStatus;
}



int main(int argc, char *argv[])
{
    int option;
    int32_t status;

    //ThreadCtl(_NTO_TCTL_IO, 0);

    while ((option = getopt(argc, argv, "ht:")) != -1)
    {
        switch (option)
        {
            case 't':
                gtest = strtoul(optarg, 0, 0);
                break;
            case 'h':
                print_help();
                return EXIT_SUCCESS;
            default:
                fprintf(stderr,"Unsupported option '-%c'\n",option);
                print_help();
        }
    }

    status = semtest();
    if (status == EOK)
    {
        printf("All tests passed!\n");
    }
    else
    {
        fprintf(stderr, "Some test(s) failed! (status = %d)\n", status);
    }
    return status;
}
