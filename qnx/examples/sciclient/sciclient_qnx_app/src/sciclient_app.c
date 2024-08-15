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
#elif defined (SOC_AM62X)
#include "ti/drv/sciclient/soc/V5/sciclient_fmwMsgParams.h"
#elif defined (SOC_J784S4)
#include "ti/drv/sciclient/soc/V6/sciclient_fmwMsgParams.h"
#elif defined (SOC_AM62A)
#include "ti/drv/sciclient/soc/V7/sciclient_fmwMsgParams.h"
#elif defined (SOC_J722S)
#include "ti/drv/sciclient/soc/V9/sciclient_fmwMsgParams.h"
#else
#error "unsupported SOC"
#endif

// Required by cslinit library
unsigned int __bss_start__, __bss_end__;
unsigned int __data_load__, __data_start__, __data_end__;
uint64_t TTBR3_BASE_ADDR, TTBR2_BASE_ADDR, TTBR1_BASE_ADDR;

#define HZ_TO_MHZ(freq) ((freq)/1000000)

/* Retrieve clock frequency for given deviceID and ClockID */
void setDeviceState(uint32_t deviceId, uint32_t state)
{
    int32_t status = CSL_PASS;
    struct tisci_msg_set_device_req reqState;

    /* Fill in payload */
    reqState.id = deviceId;
    reqState.reserved  = 0;
    reqState.state = state;

    /* Create Request message */
    const Sciclient_ReqPrm_t      reqPrm =
    {
        TISCI_MSG_SET_DEVICE,
        TISCI_MSG_FLAG_AOP,
        (uint8_t *) &reqState,
        sizeof(reqState),
        SCICLIENT_SERVICE_WAIT_FOREVER
    };

    /* Create response buffer */
    Sciclient_RespPrm_t           respPrm =
    {
        0,
        NULL,
        0
    };

    /* Send request */
    status = Sciclient_service(&reqPrm, &respPrm);
    if (CSL_PASS == status)
    {
        if (respPrm.flags == TISCI_MSG_FLAG_ACK)
        {
            status = CSL_PASS;
        }
        else
        {
            printf("%s (%d): Set Device State, failed\n",__FUNCTION__,__LINE__);delay(200);
        }
    }
    else
    {
        printf("%s (%d): DMSC Firmware Set Device state, failed\n",__FUNCTION__,__LINE__);delay(200);
    }

    return;
}

/* Retrieve clock frequency for given deviceID and ClockID */
uint64_t  getClockState(uint32_t deviceId, uint32_t clockId)
{
    int32_t status = CSL_PASS;
    struct tisci_msg_get_clock_req reqState;
    struct tisci_msg_get_clock_resp respState = {0};

    /* Fill in payload */
    reqState.device = deviceId;
    
    if (clockId >= 255U)
    {
        reqState.clk32 = clockId;
        reqState.clk   = (uint8_t) 255U;
    }
    else
    {
        reqState.clk    = (uint8_t) clockId;
    }

    /* Create Request message */
    const Sciclient_ReqPrm_t      reqPrm =
    {
        TISCI_MSG_GET_CLOCK,
        TISCI_MSG_FLAG_AOP,
        (uint8_t *) &reqState,
        sizeof(reqState),
        SCICLIENT_SERVICE_WAIT_FOREVER
    };

    /* Create response buffer */
    Sciclient_RespPrm_t           respPrm =
    {
        0,
        (uint8_t *) &respState,
        sizeof (respState)
    };

    /* Send request */
    status = Sciclient_service(&reqPrm, &respPrm);
    if (CSL_PASS == status)
    {
        if (respPrm.flags == TISCI_MSG_FLAG_ACK)
        {
            status = CSL_PASS;
        }
        else
        {
            printf("%s (%d): Get Clock State, failed\n",__FUNCTION__,__LINE__);delay(200);
        }
    }
    else
    {
        printf("%s (%d): DMSC Firmware Get Clock Frequency, failed\n",__FUNCTION__,__LINE__);delay(200);
    }

    return respState.current_state;
}

/* Retrieve device current state for given deviceID */
uint64_t  getDeviceState(uint32_t deviceId)
{
    int32_t status = CSL_PASS;
    struct tisci_msg_get_device_req reqState;
    struct tisci_msg_get_device_resp respState = {0};

    /* Fill in payload */
    reqState.id = deviceId;

    /* Create Request message */
    const Sciclient_ReqPrm_t      reqPrm =
    {
        TISCI_MSG_GET_DEVICE,
        TISCI_MSG_FLAG_AOP,
        (uint8_t *) &reqState,
        sizeof(reqState),
        SCICLIENT_SERVICE_WAIT_FOREVER
    };

    /* Create response buffer */
    Sciclient_RespPrm_t           respPrm =
    {
        0,
        (uint8_t *) &respState,
        sizeof (respState)
    };

    /* Send request */
    status = Sciclient_service(&reqPrm, &respPrm);
    if (CSL_PASS == status)
    {
        if (respPrm.flags == TISCI_MSG_FLAG_ACK)
        {
            status = CSL_PASS;
        }
        else
        {
            printf("%s (%d): DeviceId State failed \n",__FUNCTION__,__LINE__);delay(200);
        }
    }
    else
    {
        printf("%s (%d): DMSC Firmware Get device state, failed \n",__FUNCTION__,__LINE__);delay(200);
    }

    return respState.current_state;
}


/* Retrieve clock frequency for given deviceID and ClockID */
int32_t getClockFreq(uint32_t deviceId, uint32_t clockId, uint64_t *clockFreq)
{
    int32_t status = CSL_PASS;

    struct tisci_msg_get_freq_req reqFreq ;
    struct tisci_msg_get_freq_resp respFreq = {0};

    /* Fill in payload */
    reqFreq.device = deviceId;

    if (clockId >= 255U)
    {
        reqFreq.clk32 = clockId;
        reqFreq.clk   = (uint8_t) 255U;
    }
    else
    {
        reqFreq.clk    = (uint8_t) clockId;
    }

    /* Create Request message */
    const Sciclient_ReqPrm_t      reqPrm =
    {
        TISCI_MSG_GET_FREQ,
        TISCI_MSG_FLAG_AOP,
        (uint8_t *) &reqFreq,
        sizeof(reqFreq),
        SCICLIENT_SERVICE_WAIT_FOREVER
    };

    /* Create response buffer */
    Sciclient_RespPrm_t           respPrm =
    {
        0,
        (uint8_t *) &respFreq,
        sizeof (respFreq)
    };

    /* Send request */
    status = Sciclient_service(&reqPrm, &respPrm);
    if (CSL_PASS == status)
    {
        if (respPrm.flags == TISCI_MSG_FLAG_ACK)
        {
            status = CSL_PASS;
        }
        else
        {
            printf("%s (%d): DMSC Firmware Get Clock Frequency, sci ACK flag not recieved\n",__FUNCTION__,__LINE__);delay(200);
            status = CSL_EFAIL;
        }
    }
    else
    {
        printf("%s (%d): DMSC Firmware Get Clock Frequency, Sciclient_service() failed\n",__FUNCTION__,__LINE__);delay(200);
    }

    if (status == CSL_PASS) 
    {
        *clockFreq = respFreq.freq_hz;
    }

    return status;
}


int32_t getRevision(void)
{
    int32_t status = CSL_PASS;

    /* Fill in version request message */
    struct tisci_msg_version_req request;
    const Sciclient_ReqPrm_t      reqPrm =
    {
        TISCI_MSG_VERSION,
        TISCI_MSG_FLAG_AOP,
        (uint8_t *) &request,
        sizeof(request),
        SCICLIENT_SERVICE_WAIT_FOREVER
    };

    struct tisci_msg_version_resp response;
    Sciclient_RespPrm_t           respPrm =
    {
        0,
        (uint8_t *) &response,
        sizeof (response)
    };

    /* Request version */
    status = Sciclient_service(&reqPrm, &respPrm);
    if (CSL_PASS == status)
    {
        if (respPrm.flags == TISCI_MSG_FLAG_ACK)
        {
            status = CSL_PASS;
            printf(" DMSC Firmware Version %s\n", (char *) response.str);delay(200);
            printf(" Firmware revision 0x%x\n", response.version);delay(200);
            printf(" ABI revision %d.%d\n", response.abi_major, response.abi_minor);
        }
        else
        {
            printf("%s (%d): DMSC Firmware Get Version failed, sci ACK flag not recieved\n", __FUNCTION__,__LINE__);delay(200);
            status = CSL_EFAIL;
        }
    }
    else
    {
        printf("%s (%d): DMSC Firmware Get Version failed\n", __FUNCTION__,__LINE__);delay(200);
    }

    return status;
}

int main(void)
{
    int32_t status = CSL_PASS;
    uint64_t clkFreq;
#if 0
    int32_t mode = SCICLIENT_SERVICE_OPERATION_MODE_POLLED;

    /* Lets test interrupt */
    mode = SCICLIENT_SERVICE_OPERATION_MODE_INTERRUPT;

    /* Configuration parameters, interrupt or polled */
    Sciclient_ConfigPrms_t config =
    {
        mode,
        NULL
    };
#endif

    /* Get IO priveleges */
    if (ThreadCtl(_NTO_TCTL_IO, NULL) == -1)
    {
        perror("ThreadCtl(_NTO_TCTL_IO");
        return 1;
    }

    /* Initialize SCI */
#if 0
    status = Sciclient_init(&config);
    printf("%s: Sciclient_init complete\n",__FUNCTION__);delay(200);
#endif

    if (status == CSL_PASS)
    {

        /* Get the Revision */
        status = getRevision();
    }

    if (status == CSL_PASS)
    {

        /* Get Some Clock Frequencies */

#if defined(SOC_J784S4)
        /* A72 */
        status |= getClockFreq(TISCI_DEV_A72SS0_CORE0, TISCI_DEV_A72SS0_CORE0_ARM0_CLK_CLK, &clkFreq);
        printf("A72 Core0 Freq = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        delay(200);
#elif defined(SOC_AM62X) || defined(SOC_AM62A) || defined(SOC_J722S)
        /* A53 */
        status |= getClockFreq(TISCI_DEV_A53SS0_CORE_0, TISCI_DEV_A53SS0_CORE_0_A53_CORE0_ARM_CLK_CLK, &clkFreq);
        printf("A53 Core0 Freq = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        delay(200);
#else
        /* A72 */
        status |= getClockFreq(TISCI_DEV_A72SS0_CORE0, TISCI_DEV_A72SS0_CORE0_ARM_CLK_CLK, &clkFreq);
        printf("A72 Core0 Freq = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        delay(200);
#endif

#if !defined(SOC_AM62X) && !defined(SOC_AM62A) && !defined(SOC_J722S)
        /* UDMA  */
        status |= getClockFreq(TISCI_DEV_NAVSS0_UDMAP_0, TISCI_DEV_NAVSS0_UDMASS_VD2CLK, &clkFreq);
        printf("TISCI_DEV_NAVSS0_BUS_UDMASS_VD2CLK = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        status |= getClockFreq(TISCI_DEV_NAVSS0_UDMAP_0, TISCI_DEV_NAVSS0_MODSS_VD2CLK, &clkFreq);
        printf("TISCI_DEV_NAVSS0_BUS_MODSS_VD2CLK = %ld MHz\n", HZ_TO_MHZ(clkFreq));

        /* CPSW2G */
        status |= getClockFreq(TISCI_DEV_MCU_CPSW0, TISCI_DEV_MCU_CPSW0_GMII1_MR_CLK, &clkFreq);
        printf("CPSW2G GMII1_MR_CLK Freq      = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        status |= getClockFreq(TISCI_DEV_MCU_CPSW0, TISCI_DEV_MCU_CPSW0_RGMII_MHZ_250_CLK, &clkFreq);
        printf("CPSW2G RGMII_MHZ_250_CLK Freq = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        status |= getClockFreq(TISCI_DEV_MCU_CPSW0, TISCI_DEV_MCU_CPSW0_CPTS_RFT_CLK, &clkFreq);
        printf("CPSW2G CPTS_RFT_CLK Freq      = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        status |= getClockFreq(TISCI_DEV_MCU_CPSW0, TISCI_DEV_MCU_CPSW0_GMII1_MT_CLK, &clkFreq);
        printf("CPSW2G GMII1_MT_CLK Freq      = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        status |= getClockFreq(TISCI_DEV_MCU_CPSW0, TISCI_DEV_MCU_CPSW0_RGMII_MHZ_5_CLK, &clkFreq);
        printf("CPSW2G RGMII_MHZ_5_CLK Freq   = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        status |= getClockFreq(TISCI_DEV_MCU_CPSW0, TISCI_DEV_MCU_CPSW0_RGMII_MHZ_50_CLK, &clkFreq);
        printf("CPSW2G RGMII_MHZ_50_CLK Freq  = %ld MHz\n", HZ_TO_MHZ(clkFreq));

        status |= getClockFreq(TISCI_DEV_MCU_CPSW0, TISCI_DEV_MCU_CPSW0_GMII_RFT_CLK, &clkFreq);
        printf("CPSW2G GMII_RFT_CLK Freq = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        status |= getClockFreq(TISCI_DEV_MCU_CPSW0, TISCI_DEV_MCU_CPSW0_CPPI_CLK_CLK, &clkFreq);
        printf("CPSW2G CPPI_CLK Freq     = %ld MHz\n", HZ_TO_MHZ(clkFreq));
        status |= getClockFreq(TISCI_DEV_MCU_CPSW0, TISCI_DEV_MCU_CPSW0_CPTS_GENF0, &clkFreq);
        printf("CPSW2G CPTS_GENF0_0 Freq = %ld MHz\n", HZ_TO_MHZ(clkFreq));
#endif

#if 0
        /* Read some Device States */
        printf("TISCI_DEV_NAVSS0_UDMAP_0 state = %ld\n", getDeviceState(TISCI_DEV_NAVSS0_UDMAP_0));
        printf("TISCI_DEV_MCU_NAVSS0_UDMAP0 state = %ld\n", getDeviceState(TISCI_DEV_MCU_NAVSS0_UDMAP0));
        printf("TISCI_DEV_MCU_CPSW0 state = %ld\n", getDeviceState(TISCI_DEV_MCU_CPSW0));
        printf("TISCI_DEV_I2C3 state = %ld\n", getDeviceState(TISCI_DEV_I2C3));
        printf("TISCI_DEV_MCU_MCAN1 state = %ld\n", getDeviceState(TISCI_DEV_MCU_MCAN1));
        printf("TISCI_DEV_PRU_ICSSG2 state = %ld\n", getDeviceState(TISCI_DEV_PRU_ICSSG2));

        /* Read some Clock States */
        printf("TISCI_DEV_NAVSS0_BUS_MODSS_VD2CLK state = %ld\n", getClockState(TISCI_DEV_NAVSS0_UDMAP_0, TISCI_DEV_NAVSS0_BUS_MODSS_VD2CLK));
        printf("GMII1_MR_CLK state = %ld\n", getClockState(TISCI_DEV_MCU_CPSW0, TISCI_DEV_MCU_CPSW0_BUS_GMII1_MR_CLK));

        /* Try setting device States On */
        setDeviceState(TISCI_DEV_NAVSS0_UDMAP_0,2);  // 0:off, 1:retention, 2:On
        setDeviceState(TISCI_DEV_MCU_CPSW0,2);      // 0:off, 1:retention,  2:On
#endif
    }

    if (status == CSL_PASS)
    {
        status = Sciclient_deinit();
    }
    return status;
}
