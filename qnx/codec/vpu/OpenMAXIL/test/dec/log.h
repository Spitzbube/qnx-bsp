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

#ifndef OMXIL_VDEC_LOG_H_
#define OMXIL_VDEC_LOG_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdexcept>
struct Error : std::exception
{
    char text[1000];

    Error(char const* fmt, ...) __attribute__((format(printf,2,3))) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(text, sizeof text, fmt, ap);
        va_end(ap);
    }

    char const* what() const throw() { return text; }
};

typedef enum ErrorLevel {
	LOG_SHUTDOWN = 0,  /* Shut down the system NOW. eg: for OEM use */
	LOG_CRITICAL = 1,  /* Unexpected unrecoverable error. eg: hard disk error */
	LOG_ERROR    = 2,  /* Unexpected recoverable error. eg: needed to reset a hw controller */
	LOG_WARNING  = 3,  /* Expected error. eg: parity error on a serial port */
	LOG_NOTICE   = 4,  /* Warnings. eg: Out of paper */
	LOG_INFO     = 5,  /* Information. eg: Printing page 3 */
	LOG_DEBUG1   = 6,  /* Debug messages eg: Normal detail */
	LOG_DEBUG2   = 7,  /* Debug messages eg: Fine detail */
} ErrorLevel_t;

void LOG(int lvl, const char *fmt, ...);

#if defined (__cplusplus)
extern "C" {
#endif

extern void omxil_log(int level, const char *fmt, ...);

#if defined (__cplusplus)
}
#endif
#endif //OMXIL_VDEC_LOG_H_

