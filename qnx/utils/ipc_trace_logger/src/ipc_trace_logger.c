/*
 *  Copyright (c) Texas Instruments Incorporated 2023
 *  All rights reserved.
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
 *  \file ipc_trae_logger.c
 *
 *  \brief File containing the IPC trace logger tool.
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/slog.h>

#define IPC_TRACE_ARG_VAL( _o, _v ) if( (_v) == NULL || *(_v) == '\0' ) { ret = EINVAL; fprintf(stderr, "%s: missing argument for '%s'\n", __func__, _o); break;}
#define _SLOG_IPC_REMOTE_LOG    50
#define IPC_TRACE_MAX_LINE_LENGTH     (512)      // taken from ipc_trace.h
#define IPC_TRACE_BUFFER_MAX_SIZE     (0x80000)  // taken from ipc_trace.h

static char *supported_opts[] = {
    "tracebuffer_base",      // Trace buffer base physical address
    "tracebuffer_size",      // Trace buffer size
    "print_to_slog",         // Output to slog or DebugOut
    "wait_mode",             // Wait mode
    NULL
};

typedef struct trace_logger_t
{
    uintptr_t traceBufPhyAddr;
    uint32_t  traceBufSize;
    void*     traceBufVirtAddr;
    char buf[IPC_TRACE_MAX_LINE_LENGTH];
    uint32_t  rd_idx;
    uint32_t  printToSlog;
    uint32_t  waitMode;
} trace_logger_t;

trace_logger_t g_logger;


void print_help()
{
    printf("For usage of ipc_trace_logger, call \"use ipc_trace_logger\"\n");
}

void print_string(uint32_t line, char *string)
{
    if (g_logger.printToSlog)
        slogf(_SLOG_IPC_REMOTE_LOG,_SLOG_INFO, "TRACE:%d: %s", line, string);
    else
        printf("TRACE:%d: %s\n", line, string);
}

uint32_t GetStringFromTraceBuffer()
{
    uint32_t idx = 0, copy_bytes = 0;
    uint8_t cur_char;
    volatile uint8_t *src = (uint8_t *)(g_logger.traceBufVirtAddr + g_logger.rd_idx);

    for (copy_bytes = 0U; copy_bytes < IPC_TRACE_MAX_LINE_LENGTH; copy_bytes ++)
    {
        if (g_logger.rd_idx >= g_logger.traceBufSize)
        {
            g_logger.rd_idx = 0;
        }

        cur_char = src[idx];

        if ((cur_char==(uint8_t)0xA0) ||
            (cur_char==(uint8_t)'\r') ||
            (cur_char==(uint8_t)'\n'))
        {
            g_logger.rd_idx++;
            break;
        }
        else if (cur_char==(uint8_t)0)
        {
            // looks like this is the end
            break;
        }
        else
        {
            // ASCII string character
            g_logger.rd_idx++;
            g_logger.buf[idx] = (char)cur_char;
            idx ++;
        }
    }
    return idx;
}

int main(int argc, char *argv[])
{
    char                   *input_args;
    char                   *value;
    char                   *freeptr;
    char                   *options;
    int                    opt;
    uint32_t               done = 0;
    int                    ret = EOK;
    uint32_t               line = 0;
    int                    tries = 0;

    memset(&g_logger, 0, sizeof(trace_logger_t));
    g_logger.traceBufSize = IPC_TRACE_BUFFER_MAX_SIZE;

    // option has to be the last, no "-"
    input_args = NULL;
    if (strstr(argv[argc - 1], "-") == NULL) {
        input_args = argv[--argc];
    }

    if (input_args != NULL) {

        freeptr = strdup(input_args);

        options = freeptr;

        while (options && *options != '\0') 
        {
            opt = getsubopt(&options, supported_opts, &value);
            switch (opt)
            {
                case 0:
                    IPC_TRACE_ARG_VAL(supported_opts[opt], value);
                    g_logger.traceBufPhyAddr     = (uintptr_t)strtoul(value, 0, 0);
                    break;
                case 1:
                    IPC_TRACE_ARG_VAL(supported_opts[opt], value);
                    g_logger.traceBufSize     = strtoul(value, 0, 0);
                    break;
                case 2:
                    IPC_TRACE_ARG_VAL(supported_opts[opt], value);
                    g_logger.printToSlog     = strtoul(value, 0, 0);
                    break;
                case 3:
                    IPC_TRACE_ARG_VAL(supported_opts[opt], value);
                    g_logger.waitMode     = strtoul(value, 0, 0);
                    break;
                default:
                    fprintf(stderr, "ipc_trace_logger: unknown option: %s\n", value);
                    fprintf(stderr, "type: use ipc_trace_logger\n");
                    ret = EPERM;
                    goto fail1;
                    break;
            }
        }
fail1:
        free(freeptr);

        if (ret != EOK)
        {
            goto fail0;
        }
    }

    if ((!g_logger.traceBufPhyAddr) || (!g_logger.traceBufSize))
    {
        perror("ipc_trace_logger: No buffer addres specified or size is bad!");
        ret = EINVAL;
        goto fail0;
    }

    g_logger.traceBufVirtAddr  = (void *)mmap_device_memory(0, g_logger.traceBufSize, PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, g_logger.traceBufPhyAddr);
    if(g_logger.traceBufVirtAddr == MAP_FAILED)
    {
        perror("ipc_trace_logger: mmap_device_memory failed!");
        ret = ENOMEM;
        goto fail0;
    }
    g_logger.rd_idx = 0;

    while (!done)
    {
        uint32_t str_len;
        str_len = GetStringFromTraceBuffer();
        if(str_len > 0)
        {
            g_logger.buf[str_len] = (uint8_t)'\0';
            print_string(line++, g_logger.buf);
            tries = 0;
        }
        else 
        {
            tries++;
            if (tries > 2) {
                if (g_logger.waitMode) {
                    delay(500);
                }
                else {
                    done = 1;
                }
            }
        }
    }

    munmap_device_memory((void *)g_logger.traceBufPhyAddr, g_logger.traceBufSize);

fail0:
    return ret;
}


