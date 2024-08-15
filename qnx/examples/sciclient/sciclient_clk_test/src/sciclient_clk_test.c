/*
 *  Copyright (c) Texas Instruments Incorporated 2023
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

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/neutrino.h>

#include "ti/drv/sciclient/sciclient.h"
#include "ti/csl/csl_types.h"
#if defined (SOC_J721E)
#include "ti/drv/sciclient/soc/V1/sciclient_fmwMsgParams.h"
#elif defined (SOC_J7200)
#include "ti/drv/sciclient/soc/V2/sciclient_fmwMsgParams.h"
#elif defined (SOC_J721S2)
#include "ti/drv/sciclient/soc/V4/sciclient_fmwMsgParams.h"
#elif defined (SOC_J784S4)
#include "ti/drv/sciclient/soc/V6/sciclient_fmwMsgParams.h"
#elif defined (SOC_J722S)
#include "ti/drv/sciclient/soc/V9/sciclient_fmwMsgParams.h"
#else
#error "unsupported SOC"
#endif

void print_usage(void)
{
    printf("TestApp to get GPU frequency                                              \n");
    printf("                                                                          \n");
    printf("Usage:                                                                    \n");
    printf("    sciclient_clk_test <frequency Mhz>                                    \n");
    printf("                                                                          \n");
    printf("Options:                                                                  \n");
    printf("    frequency:   Optional, 750 MHz(J721E), 800 Mhz(J721S2, J784S4, J722S) \n");
    printf("                                                                          \n");
    exit(0);
}

#if defined (SOC_J721E)
#define GPU_MODULE_ID   TISCI_DEV_GPU0_GPU_0
#define GPU_CLOCK_ID    TISCI_DEV_GPU0_GPU_0_GPU_PLL_CLK
#elif defined (SOC_J7200)
#define GPU_MODULE_ID   0 // Not available
#define GPU_CLOCK_ID    0
#elif defined (SOC_J721S2)
#define GPU_MODULE_ID   TISCI_DEV_J7AEP_GPU_BXS464_WRAP0_GPU_SS_0
#define GPU_CLOCK_ID    TISCI_DEV_J7AEP_GPU_BXS464_WRAP0_GPU_SS_0_GPU_PLL_CLK
#elif defined (SOC_J784S4)
#define GPU_MODULE_ID   TISCI_DEV_J7AEP_GPU_BXS464_WRAP0_GPU_SS_0
#define GPU_CLOCK_ID    TISCI_DEV_J7AEP_GPU_BXS464_WRAP0_GPU_SS_0_GPU_PLL_CLK
#elif defined (SOC_J722S)
#define GPU_MODULE_ID   TISCI_DEV_GPU0
#define GPU_CLOCK_ID    TISCI_DEV_GPU0_GPU_PLL_CLK
#endif


int main(int argc, char *argv[])
{
    int32_t status = CSL_PASS;
#if defined (SOC_J721E)
    uint64_t newFreqHz = 750000000; // 750 Mhz
#elif defined (SOC_J7200)
    uint64_t newFreqHz = 0;
    perror("The GPU not present on this J7200 SOC, exiting!");
    return status;
#elif defined (SOC_J721S2) || defined(SOC_J784S4) || defined(SOC_J722S)
    uint64_t newFreqHz = 800000000; // 800 Mhz
#endif

    if(argc == 2)
    {
        newFreqHz = (uint64_t) (atoi(argv[1])) * 1000000;
    }
    else if (argc != 1)
    {
        print_usage();
    }

    /* Get IO privelege */
    if (ThreadCtl(_NTO_TCTL_IO, NULL) == -1) {
        perror("ThreadCtl(_NTO_TCTL_IO");
        return 1;
    }

    /* Get GPU Clock Frequency */
    uint64_t freqHz;
    status = Sciclient_pmGetModuleClkFreq(GPU_MODULE_ID,
            GPU_CLOCK_ID,
            &freqHz,
            SCICLIENT_SERVICE_WAIT_FOREVER);
    if(status == CSL_PASS) {
        printf("Getting: GPU Module's GPU clock = %ld MHz\n", freqHz/1000000);
    }
    else {
        printf("Error getting GPU Frequency\n");
        exit(EXIT_FAILURE);
    }

    /* Set GPU Clock Frequency */
    status = Sciclient_pmSetModuleClkFreq(GPU_MODULE_ID,
            GPU_CLOCK_ID,
            newFreqHz,
            TISCI_MSG_FLAG_CLOCK_ALLOW_FREQ_CHANGE,
            SCICLIENT_SERVICE_WAIT_FOREVER);
    if(status == CSL_PASS) {
        printf("Setting: GPU Module's GPU clock  = %ld MHz\n",newFreqHz/1000000);
    }
    else {
        printf("Error setting GPU Frequency\n");
        exit(EXIT_FAILURE);
    }

    /* Get GPU Clock Frequency */
    status = Sciclient_pmGetModuleClkFreq(GPU_MODULE_ID,
            GPU_CLOCK_ID,
            &freqHz,
            SCICLIENT_SERVICE_WAIT_FOREVER);

    if(status == CSL_PASS) {
        printf("Getting: GPU Module's GPU clock  = %ld MHz\n", freqHz/1000000);
    }
    else {
        printf("Error getting GPU Frequency\n");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
