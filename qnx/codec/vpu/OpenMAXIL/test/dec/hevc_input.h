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

#ifndef VXDDEC_TEST_INPUT_HEVC
#define VXDDEC_TEST_INPUT_HEVC
#include <OMX_Extension_video_TI.h>
#include "input.h"
#include "log.h"

class OmxilVideoDecInputHEVC : public OmxilVideoDecInput {
public:
    OmxilVideoDecInputHEVC(const char* path) : OmxilVideoDecInput(path)
    {
        if(loadConfig() != EOK) {
            throw Error("Failed to read config data!\n");
        }
    }
    virtual ~OmxilVideoDecInputHEVC() = default;

    virtual int32_t readFrame(uint8_t *oBuf, uint32_t *oSize);
    virtual const char  *getRole() {return HEVC_DECODER_ROLE;}
    virtual OMX_VIDEO_CODINGTYPE getFormat() {return (OMX_VIDEO_CODINGTYPE)OMXQ_VIDEO_CodingHEVC;}

private:
    const char *HEVC_DECODER_ROLE = "video_decoder.hevc";
    int32_t loadConfig(void);

 };

#endif //VXDDEC_TEST_INPUT_HEVC
