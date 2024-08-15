/*
 ----------------------------------------------------------------------------
 * COPYRIGHT (C) 2020 CHIPS&MEDIA INC. ALL RIGHTS RESERVED
 * COPYRIGHT (C) 2022 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
 * SPDX License Identifier: BSD-3-Clause
 * SPDX License Identifier: LGPL-2.1-only
 *
 * The entire notice above must be reproduced on all authorized copies.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ----------------------------------------------------------------------------
*/

#include <string.h>
#include <stdarg.h>
#include "main_helper.h"

BOOL yuvFeeder_Feed(
    EncoderContext_t*   ctx,
    Int32               coreIdx,
    vpu_buffer_t*       inbuf,
    FrameBuffer*        fb,
    size_t              picWidth,
    size_t              picHeight,
    Uint32              srcFbIndex,
    ENC_subFrameSyncCfg *subFrameSyncConfig
    )
{
    EncOpenParam encOpenParam = ctx->encOpenParam;
    size_t       frameSize;
    size_t       frameSizeY;
    size_t       frameSizeC;
    Int32        nY;
    Int32        bitdepth=0;
    Int32        yuv3p4b=0;
    Int32        packedFormat=0;
    Uint32       outWidth=0;
    Uint32       outHeight=0;
    Uint32       subFrameSyncSrcWriteMode = subFrameSyncConfig->subFrameSyncSrcWriteMode;
    BOOL         subFrameSyncEn = subFrameSyncConfig->subFrameSyncOn;
    Uint32       writeSrcLine = subFrameSyncSrcWriteMode & ~REMAIN_SRC_DATA_WRITE;
    /* size_t               lumaSize, chromaSize; */
    size_t               chromaStride;
    size_t               stride      = fb->stride;
    FrameBufferFormat    format      = fb->format;
    int                  interLeave  = fb->cbcrInterleave;

    if ( subFrameSyncEn == TRUE && subFrameSyncSrcWriteMode == SRC_0LINE_WRITE ) {
        return TRUE;
    }
    CalcYuvSize(fb->format, picWidth, picHeight, encOpenParam.cbcrInterleave, &frameSizeY, &frameSizeC, &frameSize, &bitdepth, &packedFormat, &yuv3p4b);

    if ( subFrameSyncEn == TRUE && subFrameSyncSrcWriteMode & REMAIN_SRC_DATA_WRITE && writeSrcLine == picHeight) {
        return TRUE;
    }
    if (fb->mapType == LINEAR_FRAME_MAP ) {
        outWidth  = (yuv3p4b&&packedFormat==0) ? ((picWidth+31)/32)*32  : picWidth;
        outHeight = (yuv3p4b) ? ((picHeight+7)/8)*8 : picHeight;

        if ( yuv3p4b  && packedFormat) {
            outWidth = ((picWidth*2)+2)/3*4;
        }
        else if(packedFormat) {
            outWidth *= 2;           // 8bit packed mode(YUYV) only. (need to add 10bit cases later)
            if (bitdepth != 0)      // 10bit packed
                outWidth *= 2;
        }

        picWidth  = outWidth;
        picHeight = outHeight;

        nY = picHeight;
        switch (format) {
        case FORMAT_420:
            chromaStride = stride / 2;
            /*
            chromaWidth = picWidth / 2;
            chromaSize = picWidth * picHeight / 4; */
            break;
        default:
            chromaStride = stride / 2;
            /*
            chromaWidth = picWidth / 2;
            chromaSize = picWidth * picHeight / 4; */
            break;
        }

        /* lumaSize = picWidth * picHeight; */

        fb->bufY  = inbuf->phys_addr;
        fb->bufCb = inbuf->phys_addr + (stride * nY);

        if (interLeave == 0) {
            fb->bufCr = fb->bufCb + (chromaStride * nY/2);
        }
    }
    else {
        return FALSE;
    }

    return TRUE;
}
