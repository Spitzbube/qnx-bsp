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

#ifndef OMXIL_CORELOG_H
#define OMXIL_CORELOG_H

#include <sys/slog2.h>
#include <sys/slogcodes.h>

#define LOG_SHUTDOWN SLOG2_SHUTDOWN
#define LOG_CRITICAL SLOG2_CRITICAL
#define LOG_ERROR SLOG2_ERROR
#define LOG_WARNING SLOG2_WARNING
#define LOG_NOTICE SLOG2_NOTICE
#define LOG_INFO SLOG2_INFO
#define LOG_DEBUG1 SLOG2_DEBUG1
#define LOG_DEBUG2 SLOG2_DEBUG2

int omxil_init_slog2();

void omxil_log(int level, const char *fmt, ...);
void omxil_vlog(int level, const char *fmt, va_list ap);

#define LOG(level, ...) (omxil_log(level, __VA_ARGS__))

#endif // !OMXIL_CORELOG_H


