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

#include <stdlib.h>

#include "log.h"

static slog2_buffer_set_config_t buffer_config;
static slog2_buffer_t slog2_buffer_handle;

void omxil_vlog(int level, const char *fmt, va_list ap)
{
    vslog2f(slog2_buffer_handle, _SLOGC_MEDIA, level, fmt, ap);
}

int omxil_init_slog2()
{
    const char *log_lvl_env;
    int log_lvl;
    memset(&buffer_config, 0, sizeof(buffer_config));
    buffer_config.buffer_set_name = "OpenMAXIL";

    // These two buffers are configured below.
    buffer_config.num_buffers = 1;

    if((log_lvl_env = getenv( "OMXIL_DEBUG" )) == NULL )
        buffer_config.verbosity_level = SLOG2_INFO;
    else {
        log_lvl = atoi(log_lvl_env);
        if(log_lvl)
            buffer_config.verbosity_level = SLOG2_DEBUG2;
        else
            buffer_config.verbosity_level = SLOG2_INFO;
    }

    // Configure the first buffer, using 8 x 4KB pages.
    buffer_config.buffer_config[0].buffer_name = "omxil_buffer";
    buffer_config.buffer_config[0].num_pages = 8;

    // Register the Buffer Set
    if (slog2_register(&buffer_config, &slog2_buffer_handle, SLOG2_TRY_REUSE_BUFFER_SET) == -1) {
        fprintf(stderr, "Error registering slogger2 buffer!\n");
        return -1;
    }

    return 0;
}

void omxil_log(int severity, const char *fmt, ...)
{
    va_list arglist;
    va_start(arglist, fmt);
    omxil_vlog(severity, fmt, arglist);
    va_end(arglist);
}

