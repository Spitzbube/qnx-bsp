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
#include <inttypes.h>
#include <stdlib.h> 
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#include "input.h"
#include "h264_input.h"
#include "hevc_input.h"
#include "log.h"

std::unique_ptr<OmxilVideoDecInput> createInput(const char* path)
{
    const char *ext = strrchr(path, '.');
    if(ext != NULL) {
        if(strcasecmp(ext , ".h264") == 0 || strcasecmp(ext , ".264") == 0) {
            return std::unique_ptr<OmxilVideoDecInput>(new OmxilVideoDecInputH264(path));
        }
        else if(strcasecmp(ext , ".hevc") == 0 || strcasecmp(ext , ".h265") == 0 || strcasecmp(ext , ".265") == 0) {
            return std::unique_ptr<OmxilVideoDecInput>(new OmxilVideoDecInputHEVC(path));
        }
        else
            throw Error("Couldn't create input for %s\n", path);
    }
    else
        throw Error("Couldn't create input for %s\n", path);
}

OmxilVideoDecInput::OmxilVideoDecInput(const char* path)
                    : m_EOF(false)
                    , mWidth(0)
                    , mHeight(0)
                    , mAlignedWidth(0)
                    , mAlignedHeight(0)
                    , mLumaDepth(8)
                    , mChromaDepth(8)
                    , mChromaFmt(0)
                    , mFrameRate(0)
                    , mConfigSize(0)
{
    m_fd = open(path, O_RDONLY);
    if (m_fd == -1) {
        throw Error("Failed to open input file! %s\n", strerror(errno));
    }
    
    m_inputBuffer = (uint8_t *)malloc(DEFAULT_BUFFER_SIZE);
    if(m_inputBuffer == NULL) {
        throw Error("Failed to allocate input buffer! %s\n", strerror(errno));
    }

    m_inputDataSize = read(m_fd, m_inputBuffer, DEFAULT_BUFFER_SIZE);
    if(m_inputDataSize < 0) {
        throw Error("Failed to read input data! %s\n", strerror(errno));
    }
    else if(m_inputDataSize < DEFAULT_BUFFER_SIZE) {
        LOG(LOG_DEBUG2,"Reach the end of file");
        m_EOF = true;
    }

    m_currPtr = m_inputBuffer;
    LOG(LOG_DEBUG2,"Read input data %d bytes", m_inputDataSize);

}

OmxilVideoDecInput::~OmxilVideoDecInput()
{
    free(m_inputBuffer);
    if (m_fd != -1)
        close(m_fd);
}

