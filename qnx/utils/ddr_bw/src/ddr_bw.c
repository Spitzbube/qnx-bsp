/*
 *  Copyright (c) Texas Instruments Incorporated 2018-2023
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
 *  \file ddr_bw.c
 *
 *  \brief DDR bw performance tool
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <hw/inout.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <stdbool.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/siginfo.h>
#include <sys/procmgr.h>
#include <stddef.h>

#include "ti/csl/csl_types.h"
#include <ti/csl/soc.h>

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#if defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
#define APP_PERF_DDR_MHZ                (2133u)  /* DDR clock speed in MHZ */
#else
#define APP_PERF_DDR_MHZ                (1866u)  /* DDR clock speed in MHZ for AM62A*/
#endif
#define APP_PERF_DDR_BUS_WIDTH          (  32u)  /* in units of bits */
#define APP_PERF_DDR_BURST_SIZE_BYTES   (  64u)  /* in units of bytes */

#define APP_PERF_DDR_STATS_CTR0         (0x00) /* A value of 0x00 configures counter 0 to return number of write transactions  */
#define APP_PERF_DDR_STATS_CTR1         (0x01) /* A value of 0x01 configures counter 1 to return number of read transactions   */
/* Use counter 2 and 3 to provide stats other than read/write transactions */
#define APP_PERF_DDR_STATS_CTR2         (0x03) /* A value of 0x03 configures counter 3 to return number of command activations */
#define APP_PERF_DDR_STATS_CTR3         (0x1C) /* A value of 0x1C configures counter 4 to return number of queue full states   */

#if defined(SOC_J721E) || defined(SOC_J7200) || defined (SOC_J722S)
#define APP_PERF_NUM_DDR_INSTANCES      (1u)
#elif defined (SOC_J721S2)
#define APP_PERF_NUM_DDR_INSTANCES      (2u)
#elif defined (SOC_J784S4)
#define APP_PERF_NUM_DDR_INSTANCES      (4u)
#endif

#if defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
#define PERF_DDR_INST0_BASE 0x02980100
#define PERF_DDR_INST0_SIZE 512
#elif defined(SOC_J722S)
#define PERF_DDR_INST0_BASE 0x00F300100
#define PERF_DDR_INST0_SIZE 512
#endif

#if defined(SOC_J721S2) || defined(SOC_J784S4)
#define PERF_DDR_INST1_BASE 0x029A0100
#define PERF_DDR_INST1_SIZE 512
#endif

#if defined(SOC_J784S4)
#define PERF_DDR_INST2_BASE 0x029C0100
#define PERF_DDR_INST2_SIZE 512
#define PERF_DDR_INST3_BASE 0x029E0100
#define PERF_DDR_INST3_SIZE 512
#endif


/* Define this to print counter2 and counter3 values */
#define APP_PERF_SHOW_DDR_STATS

/* Specify the duration for with counter2 and counter3 values are to be accumulated before printing */
#define APP_PERF_SNAPSHOT_WINDOW_WIDTH (500000 * 4) /* Configured for 2 seconds */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */
/**
 * \brief DDR BW information
 *
 * note, this information is retrived from MCU2-1
 *       EMIF counters are used to sample read, write access every 1ms periodicity
 */
typedef struct {

    uint32_t read_bw_avg;   /**< avg bytes read per second, in units of MB/s */
    uint32_t write_bw_avg;  /**< avg bytes written per second, in units of in MB/s */
    uint32_t read_bw_peak;  /**< peak bytes read in a sampling period, in units of MB/s */
    uint32_t write_bw_peak; /**< peak bytes read in a sampling period, in units of MB/s */
    uint32_t total_available_bw; /**< theoritical bw available to system, in units of MB/s */

    uint32_t counter0_total;  /**< sum total of counter0 values aggregated over time as defined by APP_PERF_SNAPSHOT_WINDOW_WIDTH */
    uint32_t counter1_total;  /**< sum total of counter1 values aggregated over time as defined by APP_PERF_SNAPSHOT_WINDOW_WIDTH */
    uint32_t counter2_total;  /**< sum total of counter2 values aggregated over time as defined by APP_PERF_SNAPSHOT_WINDOW_WIDTH */
    uint32_t counter3_total;  /**< sum total of counter3 values aggregated over time as defined by APP_PERF_SNAPSHOT_WINDOW_WIDTH */

} app_perf_stats_ddr_stats_t;

typedef struct {

    app_perf_stats_ddr_stats_t ddr_stats;
    uint64_t total_time;
    uint64_t last_timestamp;
    uint64_t total_read;
    uint64_t total_write;
    int32_t snapshot_count;

} app_perf_stats_ddrLoad_t;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */
void appPerfStatsMapDDRMemory();
void appPerfStatsUnMapDDRMemory();
void appPerfStatsResetDdrLoadCalcAll();
void appPerfStatsDddrStatsUpdate();
void appPerfStatsDdrStatsReadCounters(uint32_t *val0, uint32_t *val1, uint32_t *val2, uint32_t *val3, bool raw);
int32_t appPerfStatsDdrStatsPrint();

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
static int gVerbose = 0;

static app_perf_stats_ddrLoad_t g_app_perf_stats_ddrLoad_obj;
static uintptr_t gAppDdrPerfBase0 = 0;
#if defined(SOC_J721S2) || defined(SOC_J784S4)
static uintptr_t gAppDdrPerfBase1 = 0;
#endif
#if defined(SOC_J784S4)
static uintptr_t gAppDdrPerfBase2 = 0;
static uintptr_t gAppDdrPerfBase3 = 0;
#endif

/*
 * Definition and Macro for debug vs. logging
 */
#define APP_LOG 1
#define APP_DBG 2
#define APP_PRINT(level, f_, ...)   if(((gVerbose == 1) && (level == APP_DBG)) || \
                                      (level == APP_LOG)) \
                                      printf((f_), ##__VA_ARGS__)

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

/*
 * Map ddr stat memory regions
 */
void appPerfStatsMapDDRMemory()
{
#if defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4) || defined(SOC_J722S)
    gAppDdrPerfBase0 = (uintptr_t) mmap_device_memory(0, PERF_DDR_INST0_SIZE, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, PERF_DDR_INST0_BASE);
    if(gAppDdrPerfBase0 == 0)
    {
        APP_PRINT(APP_LOG,"%s: Memory map of gAppDdrPerfBase0 failed\n",__func__);
        exit(-1);
    }
#endif
#if defined(SOC_J721S2) || defined(SOC_J784S4)
    gAppDdrPerfBase1 = (uintptr_t) mmap_device_memory(0, PERF_DDR_INST1_SIZE, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, PERF_DDR_INST1_BASE);
    if(gAppDdrPerfBase1 == 0)
    {
        APP_PRINT(APP_LOG,"%s: Memory map of gAppDdrPerfBase1 failed\n",__func__);
        exit(-1);
    }
#endif
#if defined(SOC_J784S4)
    gAppDdrPerfBase2 = (uintptr_t) mmap_device_memory(0, PERF_DDR_INST2_SIZE, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, PERF_DDR_INST2_BASE);
    if(gAppDdrPerfBase2 == 0)
    {
        APP_PRINT(APP_LOG,"%s: Memory map of gAppDdrPerfBase2 failed\n",__func__);
        exit(-1);
    }
    gAppDdrPerfBase3 = (uintptr_t) mmap_device_memory(0, PERF_DDR_INST3_SIZE, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, PERF_DDR_INST3_BASE);
    if(gAppDdrPerfBase3 == 0)
    {
        APP_PRINT(APP_LOG,"%s: Memory map of gAppDdrPerfBase3 failed\n",__func__);
        exit(-1);
    }
#endif
    return;
}

/*
 * Unmap ddr stat memory regions
 */
void appPerfStatsUnMapDDRMemory()
{
#if defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
    munmap_device_memory((void *)gAppDdrPerfBase0, PERF_DDR_INST0_SIZE);
#endif
#if defined(SOC_J721S2) || defined(SOC_J784S4)
    munmap_device_memory((void *)gAppDdrPerfBase1, PERF_DDR_INST1_SIZE);
#endif
#if defined(SOC_J784S4)
    munmap_device_memory((void *)gAppDdrPerfBase2, PERF_DDR_INST2_SIZE);
    munmap_device_memory((void *)gAppDdrPerfBase3, PERF_DDR_INST3_SIZE);
#endif
    return;
}

/*
 * Read ddr stat information
 */
/* read EMIF counter and calculate read and write bytes since last read */
void appPerfStatsDdrStatsReadCounters(uint32_t *val0, uint32_t *val1, uint32_t *val2, uint32_t *val3, bool raw)
{
    static uint32_t is_first_time = 1;
    static volatile uint32_t *cnt_sel[APP_PERF_NUM_DDR_INSTANCES];
    static volatile uint32_t *cnt0[APP_PERF_NUM_DDR_INSTANCES];
    static volatile uint32_t *cnt1[APP_PERF_NUM_DDR_INSTANCES];
    static volatile uint32_t *cnt2[APP_PERF_NUM_DDR_INSTANCES];
    static volatile uint32_t *cnt3[APP_PERF_NUM_DDR_INSTANCES];

    static volatile uint32_t last_cnt0 = 0, last_cnt1 = 0, last_cnt2 = 0, last_cnt3 = 0;
    volatile uint32_t cur_cnt0 = 0, cur_cnt1 = 0, cur_cnt2 = 0, cur_cnt3 = 0;
    uint32_t diff_cnt0, diff_cnt1, diff_cnt2, diff_cnt3, ddr_inst;


#if defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4) || defined(SOC_J722S)
    cnt_sel[0] = (volatile uint32_t *)(gAppDdrPerfBase0 + 0x0);
    cnt0[0]    = (volatile uint32_t *)(gAppDdrPerfBase0 + 0x4);
    cnt1[0]    = (volatile uint32_t *)(gAppDdrPerfBase0 + 0x8);
    cnt2[0]    = (volatile uint32_t *)(gAppDdrPerfBase0 + 0xC);
    cnt3[0]    = (volatile uint32_t *)(gAppDdrPerfBase0 + 0x10);
#endif

#if defined(SOC_J721S2) || defined(SOC_J784S4)
    cnt_sel[1] = (volatile uint32_t *)(gAppDdrPerfBase1 + 0x0);
    cnt0[1]    = (volatile uint32_t *)(gAppDdrPerfBase1 + 0x4);
    cnt1[1]    = (volatile uint32_t *)(gAppDdrPerfBase1 + 0x8);
    cnt2[1]    = (volatile uint32_t *)(gAppDdrPerfBase1 + 0xC);
    cnt3[1]    = (volatile uint32_t *)(gAppDdrPerfBase1 + 0x10);
#endif

#if defined(SOC_J784S4)
    cnt_sel[2] = (volatile uint32_t *)(gAppDdrPerfBase2 + 0x0);
    cnt0[2]    = (volatile uint32_t *)(gAppDdrPerfBase2 + 0x4);
    cnt1[2]    = (volatile uint32_t *)(gAppDdrPerfBase2 + 0x8);
    cnt2[2]    = (volatile uint32_t *)(gAppDdrPerfBase2 + 0xC);
    cnt3[2]    = (volatile uint32_t *)(gAppDdrPerfBase2 + 0x10);

    cnt_sel[3] = (volatile uint32_t *)(gAppDdrPerfBase3 + 0x0);
    cnt0[3]    = (volatile uint32_t *)(gAppDdrPerfBase3 + 0x4);
    cnt1[3]    = (volatile uint32_t *)(gAppDdrPerfBase3 + 0x8);
    cnt2[3]    = (volatile uint32_t *)(gAppDdrPerfBase3 + 0xC);
    cnt3[3]    = (volatile uint32_t *)(gAppDdrPerfBase3 + 0x10);
#endif

    if(is_first_time)
    {
        for (ddr_inst = 0; ddr_inst < APP_PERF_NUM_DDR_INSTANCES; ddr_inst++)
        {
            /* cnt0 is counting reads, cnt1 is counting writes, cnt2, cnt3 not used */
            *cnt_sel[ddr_inst] = (APP_PERF_DDR_STATS_CTR0 <<  0u) |
                       (APP_PERF_DDR_STATS_CTR1 <<  8u) |
                       (APP_PERF_DDR_STATS_CTR2 << 16u) |
                       (APP_PERF_DDR_STATS_CTR3 << 24u);

            last_cnt0 += *cnt0[ddr_inst];
            last_cnt1 += *cnt1[ddr_inst];
            last_cnt2 += *cnt2[ddr_inst];
            last_cnt3 += *cnt3[ddr_inst];
        }

        is_first_time = 0;
    }

    for (ddr_inst = 0; ddr_inst < APP_PERF_NUM_DDR_INSTANCES; ddr_inst++)
    {
        cur_cnt0 += *cnt0[ddr_inst];
        cur_cnt1 += *cnt1[ddr_inst];
        cur_cnt2 += *cnt2[ddr_inst];
        cur_cnt3 += *cnt3[ddr_inst];
    }

    if(raw)
    {
        *val0 = (uint32_t)cur_cnt0;
        *val1 = (uint32_t)cur_cnt1;
        *val2 = (uint32_t)cur_cnt2;
        *val3 = (uint32_t)cur_cnt3;
        return;
    }

    if(cur_cnt0 < last_cnt0)
    {
        /* wrap around case */
        diff_cnt0 = (0xFFFFFFFFu - last_cnt0) + cur_cnt0;
    }
    else
    {
        diff_cnt0 = cur_cnt0 - last_cnt0;
    }

    if(cur_cnt1 < last_cnt1)
    {
        /* wrap around case */
        diff_cnt1 = (0xFFFFFFFFu - last_cnt1) + cur_cnt1;
    }
    else
    {
        diff_cnt1 = cur_cnt1 - last_cnt1;
    }

    if(cur_cnt2 < last_cnt2)
    {
        /* wrap around case */
        diff_cnt2 = (0xFFFFFFFFu - last_cnt2) + cur_cnt2;
    }
    else
    {
        diff_cnt2 = cur_cnt2 - last_cnt2;
    }

    if(cur_cnt3 < last_cnt3)
    {
        /* wrap around case */
        diff_cnt3 = (0xFFFFFFFFu - last_cnt3) + cur_cnt3;
    }
    else
    {
        diff_cnt3 = cur_cnt3 - last_cnt3;
    }


    last_cnt0 = cur_cnt0;
    last_cnt1 = cur_cnt1;
    last_cnt2 = cur_cnt2;
    last_cnt3 = cur_cnt3;

    *val0 = (uint32_t)diff_cnt0;
    *val1 = (uint32_t)diff_cnt1;
    *val2 = (uint32_t)diff_cnt2;
    *val3 = (uint32_t)diff_cnt3;
}

/*
 * Update ddr stat information
 */
void appPerfStatsDddrStatsUpdate()
{
    app_perf_stats_ddrLoad_t *ddrLoad = &g_app_perf_stats_ddrLoad_obj;
    uint32_t val0 = 0, val1 = 0, val2 = 0, val3 = 0;
    uint64_t cur_time;
    uint32_t elapsed_time;
    struct timespec ts;

    pthread_mutex_lock(&lock);

    clock_gettime(CLOCK_MONOTONIC, &ts);
    cur_time = timespec2nsec(&ts) / 1000;

    if(cur_time > ddrLoad->last_timestamp)
    {
        elapsed_time = cur_time - ddrLoad->last_timestamp;
        if(elapsed_time==0)
            elapsed_time = 1; /* to avoid divide by 0 */
        ddrLoad->total_time += elapsed_time;

        appPerfStatsDdrStatsReadCounters(&val0, &val1, &val2, &val3, false);

        uint64_t write_bytes = val0 * APP_PERF_DDR_BURST_SIZE_BYTES;
        uint64_t read_bytes  = val1 * APP_PERF_DDR_BURST_SIZE_BYTES;

        ddrLoad->total_read += read_bytes;
        ddrLoad->total_write += write_bytes;

        ddrLoad->ddr_stats.read_bw_avg = (ddrLoad->total_read/ddrLoad->total_time); /* in MB/s */
        ddrLoad->ddr_stats.write_bw_avg = (ddrLoad->total_write/ddrLoad->total_time); /* in MB/s */

        uint32_t read_bw_peak = read_bytes/elapsed_time; /* in MB/s */
        uint32_t write_bw_peak = write_bytes/elapsed_time; /* in MB/s */
        if(read_bw_peak > ddrLoad->ddr_stats.read_bw_peak)
            ddrLoad->ddr_stats.read_bw_peak = read_bw_peak;
        if(write_bw_peak > ddrLoad->ddr_stats.write_bw_peak)
            ddrLoad->ddr_stats.write_bw_peak = write_bw_peak;

#ifdef APP_PERF_SHOW_DDR_STATS
        ddrLoad->ddr_stats.counter0_total += val2;
        ddrLoad->ddr_stats.counter1_total += val3;

        ddrLoad->snapshot_count -= elapsed_time;

        if(ddrLoad->snapshot_count <= 0)
        {
            APP_PRINT(APP_DBG, "ACTIVE_CMD = %d, QUEUE_FULL = %d  \n", ddrLoad->ddr_stats.counter0_total,
                                                                 ddrLoad->ddr_stats.counter1_total);

            ddrLoad->ddr_stats.counter0_total = 0;
            ddrLoad->ddr_stats.counter1_total = 0;

            ddrLoad->snapshot_count = APP_PERF_SNAPSHOT_WINDOW_WIDTH;
        }

#endif
    }

    ddrLoad->last_timestamp = cur_time;

    pthread_mutex_unlock(&lock);
}

/*
 * Reset ddr stat information
 */
void appPerfStatsResetDdrLoadCalcAll()
{
    app_perf_stats_ddrLoad_t *ddrLoad = &g_app_perf_stats_ddrLoad_obj;
    struct timespec ts;

    pthread_mutex_lock(&lock);

    ddrLoad->ddr_stats.read_bw_avg = 0;
    ddrLoad->ddr_stats.write_bw_avg = 0;
    ddrLoad->ddr_stats.read_bw_peak = 0;
    ddrLoad->ddr_stats.write_bw_peak = 0;
    ddrLoad->ddr_stats.total_available_bw = APP_PERF_DDR_MHZ*APP_PERF_DDR_BUS_WIDTH*APP_PERF_NUM_DDR_INSTANCES*2/8;
    ddrLoad->total_time = 0;
    ddrLoad->total_read = 0;
    ddrLoad->total_write = 0;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ddrLoad->last_timestamp = timespec2nsec(&ts) / 1000;
    ddrLoad->snapshot_count = APP_PERF_SNAPSHOT_WINDOW_WIDTH;

    ddrLoad->ddr_stats.counter0_total = 0;
    ddrLoad->ddr_stats.counter1_total = 0;
    ddrLoad->ddr_stats.counter2_total = 0;
    ddrLoad->ddr_stats.counter3_total = 0;

    pthread_mutex_unlock(&lock);
}

/*
 * Print ddr stat information
 */
int32_t appPerfStatsDdrStatsPrint()
{
    app_perf_stats_ddrLoad_t *ddrLoad = &g_app_perf_stats_ddrLoad_obj;
    app_perf_stats_ddr_stats_t *ddr_load_stat = &ddrLoad->ddr_stats;
    int32_t status = 0;

    APP_PRINT(APP_LOG, "\n");
    APP_PRINT(APP_LOG, "DDR performance statistics,\n");
    APP_PRINT(APP_LOG, "===========================\n");
    APP_PRINT(APP_LOG, "\n");
    APP_PRINT(APP_LOG, "DDR: READ  BW: AVG = %6d MB/s, PEAK = %6d MB/s\n",
        ddr_load_stat->read_bw_avg,
        ddr_load_stat->read_bw_peak);
    APP_PRINT(APP_LOG, "DDR: WRITE BW: AVG = %6d MB/s, PEAK = %6d MB/s\n",
        ddr_load_stat->write_bw_avg,
        ddr_load_stat->write_bw_peak);
    APP_PRINT(APP_LOG, "DDR: TOTAL BW: AVG = %6d MB/s, PEAK = %6d MB/s\n",
        ddr_load_stat->read_bw_avg + ddr_load_stat->write_bw_avg,
        ddr_load_stat->write_bw_peak + ddr_load_stat->read_bw_peak);

    return status;
}

/*
 * Print usage information
 */
void printUsage(void)
{
    APP_PRINT(APP_LOG, "ddr_bw <options>                                               \n");
    APP_PRINT(APP_LOG, "                                                               \n");
    APP_PRINT(APP_LOG, "Options:                                                       \n");
    APP_PRINT(APP_LOG, "    v          - Be verbose                                    \n");
    exit(0);
}

/*
 * Parse command line options
 */
void parseCmdLine (int argc, char *argv[])
{
  int c;

  while ((c = getopt (argc, argv, "v")) != -1)
  {
    switch (c)
    {
      case 'v':
        gVerbose = 1;
        break;
      default:
        printUsage();
    }
  }
}

/*
 * Main
 */
int main (int argc, char **argv)
{
    bool user_request_exit = false;

    parseCmdLine(argc, argv);

    if (procmgr_ability(0,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_KEYDATA,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_IO,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_MEM_PHYS,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_PRIORITY,
                        PROCMGR_AOP_DENY  | PROCMGR_ADN_NONROOT | PROCMGR_AOP_LOCK      | PROCMGR_AID_EOL) != EOK) {
        APP_PRINT(APP_LOG, "Unable to gain procmgr abilities for nonroot operation\n");
        return 0;
    }
    /* Get IO priveleges */
    if (ThreadCtl(_NTO_TCTL_IO, NULL) == -1) {
        perror("ThreadCtl(_NTO_TCTL_IO");
        return 0;
    }

    appPerfStatsMapDDRMemory();

    appPerfStatsResetDdrLoadCalcAll();
    appPerfStatsDddrStatsUpdate();

    /* Modifying terminal to allow key-press interaction */
    struct termios new_termios;
    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO | ECHOCTL | ECHONL);
    new_termios.c_cflag |= HUPCL;
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 10;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    APP_PRINT(APP_LOG, "-----------------------------------------\n"
                       "DDR DW \n"
                       "Press any key to print ddr bw stat\n"
                       "Press'q' to quit.\n"
                       "-----------------------------------------\n");

    for (;;) {
        if (user_request_exit) {
            APP_PRINT(APP_DBG,"bailed");
            break;
        }
        char ch[8];
        int chnum = 0;

        fd_set rdfds;
        FD_ZERO(&rdfds);
        FD_SET(0, &rdfds);

        struct timeval tv = {.tv_sec = 1, .tv_usec = 0 }; // 1sec
        int ret = select(1, &rdfds, NULL, NULL, &tv);
        if (-1 == ret) {
            perror("select returns error");
            break;
        }
        if (0 == ret) {
            appPerfStatsDddrStatsUpdate();
            continue;
        }

        chnum = read(STDIN_FILENO, ch, 8);
        if (chnum == 1) {
            switch (ch[0]) {
                case 'q':
                case 'Q':
                    user_request_exit = true;
                     APP_PRINT(APP_DBG,"bailed at user request\n");
                    break;
                default:
                    appPerfStatsDdrStatsPrint();
                    APP_PRINT(APP_LOG, "-----------------------------------------\n"
                                       " q : stop & quit program.\n"
                                       "-----------------------------------------\n");
            }
        }
        else if (chnum == 0) {
            continue;
        }
    }

    /* Restore the terminal to its original state */
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

    appPerfStatsUnMapDDRMemory();

    APP_PRINT(APP_LOG, "ddr_bw, exiting.\n");
    return(0);
}

