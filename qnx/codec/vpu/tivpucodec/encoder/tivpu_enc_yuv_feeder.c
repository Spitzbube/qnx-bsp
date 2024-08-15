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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

#include "tivpu_enc.h"

static void InitYuvFeederContext(EncoderContext_t* ctx)
{

    ctx->fbAllocated        = FALSE;
    ctx->yuvMode            = 0;
    ctx->fbCount.reconFbNum = 0;
    ctx->fbCount.srcFbNum   = 0;
    ctx->srcFbIndex         = 0;

    osal_memset(&ctx->srcFbAllocInfo,          0x00, sizeof(FrameBufferAllocInfo));
    osal_memset(&ctx->reconFbAllocInfo,        0x00, sizeof(FrameBufferAllocInfo));
    osal_memset((void*)ctx->pFbRecon,          0x00, sizeof(ctx->pFbRecon));
    osal_memset((void*)ctx->pFbReconMem,       0x00, sizeof(ctx->pFbReconMem));
    osal_memset((void*)ctx->pFbSrc,            0x00, sizeof(ctx->pFbSrc));
    osal_memset((void*)ctx->pFbOffsetTbl,      0x00, sizeof(ctx->pFbOffsetTbl));
    osal_memset((void*)ctx->pFbOffsetTblMem,   0x00, sizeof(ctx->pFbOffsetTblMem));
}

ComponentParamRet tivpu_get_parameter_yuv_feeder(EncoderContext_t* ctx, GetParameterCMD commandType, void* data)
{
    BOOL                 result  = TRUE;
    ParamEncFrameBuffer* allocFb = NULL;

    switch(commandType) {
    case GET_PARAM_YUVFEEDER_FRAME_BUF:
        if (ctx->fbAllocated == FALSE) return COMPONENT_PARAM_NOT_READY;
        allocFb = (ParamEncFrameBuffer*)data;
        allocFb->reconFb          = ctx->pFbRecon;
        allocFb->srcFb            = ctx->pFbSrc;
        allocFb->reconFbAllocInfo = ctx->reconFbAllocInfo;
        allocFb->srcFbAllocInfo   = ctx->srcFbAllocInfo;
        break;
    default:
        return COMPONENT_PARAM_NOT_FOUND;
    }

    return (result == TRUE) ? COMPONENT_PARAM_SUCCESS : COMPONENT_PARAM_FAILURE;
}

static BOOL AllocateFrameBuffer(EncoderContext_t* ctx, vpu_buffer_t* inbuf)
{
    EncOpenParam            encOpenParam = ctx->encOpenParam;
    Uint32                  fbWidth = 0;
    Uint32                  fbHeight = 0;
    Uint32                  sourceFbHeight = 0;
    Uint32                  fbStride = 0;
    Uint32                  fbSize = 0;
    DRAMConfig              dramConfig;
    DRAMConfig*             pDramConfig = NULL;
    TiledMapType            mapType;
    FrameBufferAllocInfo    fbAllocInfo;

    osal_memset(&fbAllocInfo, 0x00, sizeof(FrameBufferAllocInfo));
    osal_memset(&dramConfig, 0x00, sizeof(DRAMConfig));

    /* Buffers for source frames */

    fbWidth = VPU_ALIGN8(encOpenParam.picWidth);
    fbHeight = VPU_ALIGN8(encOpenParam.picHeight);
    fbAllocInfo.endian  = encOpenParam.sourceEndian;

    mapType = LINEAR_FRAME_MAP;
    sourceFbHeight = fbHeight;
    fbStride = CalcStride(fbWidth, sourceFbHeight, (FrameBufferFormat)encOpenParam.srcFormat, encOpenParam.cbcrInterleave, mapType, FALSE);
    fbSize = VPU_GetFrameBufSize(ctx->handle, encOpenParam.coreIdx, fbStride, sourceFbHeight, mapType, (FrameBufferFormat)encOpenParam.srcFormat, encOpenParam.cbcrInterleave, pDramConfig);

    if (fbStride > ctx->srcStride) {
        VLOG(ERR, "Calculated VPU required FB stride is greater than the Input buffer stride\n");
        return FALSE;
    }

    fbAllocInfo.format  = (FrameBufferFormat)encOpenParam.srcFormat;
    fbAllocInfo.cbcrInterleave = encOpenParam.cbcrInterleave;
    fbAllocInfo.mapType = mapType;
    fbAllocInfo.stride  = fbStride;
    fbAllocInfo.height  = sourceFbHeight;
    fbAllocInfo.size    = fbSize;
    fbAllocInfo.type    = FB_TYPE_PPU;
    fbAllocInfo.num     = ctx->fbCount.srcFbNum;
    fbAllocInfo.nv21    = encOpenParam.nv21;

    /* Use the Input buffers and re-cast them into the "framebuffer" format, to be used as the input to the Encoder */
    if (FALSE == SetFbDetails(encOpenParam.coreIdx, inbuf, ctx->pFbSrc, fbSize, ctx->fbCount.srcFbNum)) {
        VLOG(ERR, "failed to attach source buffers\n");
        return FALSE;
    }
    ctx->srcFbAllocInfo = fbAllocInfo;

    /* Buffers for reconstructed frames */
    osal_memset(&fbAllocInfo, 0x00, sizeof(FrameBufferAllocInfo));

    pDramConfig = NULL;
    if (ctx->encOpenParam.bitstreamFormat == STD_AVC) {
        fbWidth  = VPU_ALIGN16(encOpenParam.picWidth);
        fbHeight = VPU_ALIGN16(encOpenParam.picHeight);

        if ((ctx->rotAngle != 0 || ctx->mirDir != 0) && !(ctx->rotAngle == 180 && ctx->mirDir == MIRDIR_HOR_VER)) {
            fbWidth  = VPU_ALIGN16(encOpenParam.picWidth);
            fbHeight = VPU_ALIGN16(encOpenParam.picHeight);
        }
        if (ctx->rotAngle == 90 || ctx->rotAngle == 270) {
            fbWidth  = VPU_ALIGN16(encOpenParam.picHeight);
            fbHeight = VPU_ALIGN16(encOpenParam.picWidth);
        }
    }
    else {
        fbWidth  = VPU_ALIGN8(encOpenParam.picWidth);
        fbHeight = VPU_ALIGN8(encOpenParam.picHeight);

        if ((ctx->rotAngle != 0 || ctx->mirDir != 0) && !(ctx->rotAngle == 180 && ctx->mirDir == MIRDIR_HOR_VER)) {
            fbWidth  = VPU_ALIGN32(encOpenParam.picWidth);
            fbHeight = VPU_ALIGN32(encOpenParam.picHeight);
        }
        if (ctx->rotAngle == 90 || ctx->rotAngle == 270) {
            fbWidth  = VPU_ALIGN32(encOpenParam.picHeight);
            fbHeight = VPU_ALIGN32(encOpenParam.picWidth);
        }
    }

    if (WAVE521C_DUAL_CODE == (VPU_HANDLE_TO_ENCINFO(ctx->handle)->productCode)) {
        mapType = encOpenParam.EncStdParam.waveParam.internalBitDepth==8?COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT:COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT;
    } else {
        mapType = COMPRESSED_FRAME_MAP;
    }


    fbStride = CalcStride(fbWidth, fbHeight, (FrameBufferFormat)encOpenParam.outputFormat, encOpenParam.cbcrInterleave, mapType, FALSE);
    fbSize   = VPU_GetFrameBufSize(ctx->handle, encOpenParam.coreIdx, fbStride, fbHeight, mapType, (FrameBufferFormat)encOpenParam.outputFormat, encOpenParam.cbcrInterleave, pDramConfig);

    if (FALSE == AllocFBMemory(encOpenParam.coreIdx, ctx->pFbReconMem, ctx->pFbRecon, fbSize, ctx->fbCount.reconFbNum, ENC_FBC, ctx->handle->instIndex)) {
        VLOG(ERR, "failed to allocate recon buffers\n");
        return FALSE;
    }

    fbAllocInfo.stride  = fbStride;
    fbAllocInfo.height  = fbHeight;
    fbAllocInfo.size    = fbSize;
    fbAllocInfo.type    = FB_TYPE_CODEC;
    fbAllocInfo.num     = ctx->fbCount.reconFbNum;

    ctx->reconFbAllocInfo = fbAllocInfo;

    return TRUE;
}

BOOL tivpu_prepare_yuv_feeder(EncoderContext_t* ctx, vpu_buffer_t* inbuf)
{
    ComponentParamRet    ret;
    BOOL                 success;

    ret = tivpu_get_parameter_encoder(ctx, GET_PARAM_ENC_FRAME_BUF_NUM, &ctx->fbCount);
    if (ParamReturnTest(ret, &success) == FALSE) {
        return FALSE;
    }

    ret = tivpu_get_parameter_encoder(ctx, GET_PARAM_ENC_HANDLE, &ctx->handle);
    if (ParamReturnTest(ret, &success) == FALSE) {
        return FALSE;
    }

    if (FALSE == AllocateFrameBuffer(ctx, inbuf)) {
        VLOG(ERR, "AllocateFramBuffer() error\n");
        return FALSE;
    }

    ctx->fbAllocated = TRUE;
    return TRUE;
}

BOOL tivpu_execute_yuv_feeder(EncoderContext_t* ctx, vpu_buffer_t* inbuf)
{
    EncOpenParam*       encOpenParam = &ctx->encOpenParam;
    int                 ret          = 0;

    if ( ctx->last ) {
        return TRUE;
    }
    if (inbuf == NULL) {
        return FALSE;
    }

    ctx->fb = ctx->pFbSrc[ctx->srcFbIndex];
    ret = yuvFeeder_Feed(ctx, encOpenParam->coreIdx, inbuf, &ctx->fb, encOpenParam->picWidth, encOpenParam->picHeight, ctx->srcFbIndex, &ctx->subFrameSyncCfg);

    ctx->srcFbIndex++;
    if (ctx->srcFbIndex >= ctx->fbCount.srcFbNum){
        ctx->srcFbIndex = 0;
    }

    if (ret == FALSE) {
        ctx->last = TRUE;
    }


    return TRUE;
}

void tivpu_release_fb_mem(EncoderContext_t* ctx)
{
    Uint32 i = 0;

    for (i = 0; i < ctx->fbCount.reconFbNum*2; i++) {
        if (ctx->pFbReconMem[i].size)
            vdi_free_dma_memory(ctx->encOpenParam.coreIdx, &ctx->pFbReconMem[i], ENC_FBC, 0);
    }

    for (i = 0; i < ctx->fbCount.srcFbNum; i++) {
        if (ctx->pFbSrcMem[i].size)
            vdi_free_dma_memory(ctx->encOpenParam.coreIdx, &ctx->pFbSrcMem[i], ENC_SRC, ctx->handle->instIndex);
        if (ctx->pFbOffsetTblMem[i].size)
            vdi_free_dma_memory(ctx->encOpenParam.coreIdx, &ctx->pFbOffsetTblMem[i], ENC_FBCY_TBL, ctx->handle->instIndex);
    }
}

BOOL tivpu_create_yuv_feeder(EncoderContext_t* ctx)
{
    InitYuvFeederContext(ctx);

    return TRUE;
}
