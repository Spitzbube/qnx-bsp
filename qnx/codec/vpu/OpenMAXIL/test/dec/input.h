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

#ifndef VXDDEC_TEST_INPUT
#define VXDDEC_TEST_INPUT
#include <memory>
#include <OMX_Video.h>

#define CONFIG_DATA_BUFFER_SIZE 8096
#define DEFAULT_BUFFER_SIZE (2*1024*1024)
#define INPUT_BUFFER_SIZE  (5*1024*1024) // Input Buffer size - set to: ((1 / NUM_IN_BUFFERS) * 10MB)


class OmxilVideoDecInput {
public:
    OmxilVideoDecInput(const char* path);
    ~OmxilVideoDecInput();

    virtual int32_t readFrame(uint8_t *oBuf, uint32_t *oSize) {return 0;}
    virtual const char  *getRole() {return NULL;}
    virtual OMX_VIDEO_CODINGTYPE getFormat() {return OMX_VIDEO_CodingUnused;}
    int32_t getAlignedWidth() {return mAlignedWidth;};
    int32_t getAlignedHeight() {return mAlignedHeight;};
    int32_t getWidth() {return mWidth;};
    int32_t getHeight() {return mHeight;};
    int32_t getLumaDepth() {return mLumaDepth;};
    int32_t getChromaDepth() {return mChromaDepth;};
    int32_t getFrameRate() {return mFrameRate;};
    uint8_t *getConfigPtr() {return mpConfig;};
    uint32_t getConfigSize() {return mConfigSize;};
    int32_t getChromaFormat() {return mChromaFmt;};

protected:
    int m_fd;
    uint8_t *m_inputBuffer;
    uint8_t *m_currPtr;
    int32_t m_inputDataSize;
    bool    m_EOF;

    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mAlignedWidth;
    uint32_t mAlignedHeight;
    uint32_t mLumaDepth;
    uint32_t mChromaDepth;
    int32_t  mChromaFmt;
    uint32_t mFrameRate;
    uint8_t  mpConfig[CONFIG_DATA_BUFFER_SIZE];
    uint32_t mConfigSize;
};

std::unique_ptr<OmxilVideoDecInput> createInput(const char* path);
#endif //VXDDEC_TEST_INPUT
