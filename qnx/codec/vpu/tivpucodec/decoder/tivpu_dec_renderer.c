//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
// Copyright 2022, Texas Instruments Incorporated - http://www.ti.com/
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------

#include <string.h>

#include "tivpu_dec.h"
#include "misc/debug.h"

static void ReleaseFrameBuffers(DecoderContext_t* ctx)
{
    Uint32           coreIdx = ctx->decOpenParam.coreIdx;
    Uint32           i;

    for (i=0; i<MAX_REG_FRAME; i++) {
        if (i >= ctx->fbCount.nonLinearNum)
            break;
        if (ctx->pFbMem[i].size) {
                vdi_free_dma_memory(coreIdx, &ctx->pFbMem[i], DEC_FBC, ctx->handle->instIndex);
        }
    }
    for (i=0; i<MAX_REG_FRAME; i++) {
        if (i < ctx->fbCount.linearNum)
            break;
        if (ctx->pFbWtlMem && ctx->pFbWtlMem[i].size) {
                vdi_dettach_dma_memory(coreIdx, &ctx->pFbWtlMem[i]);
        }
    }

    for (i=0; i<MAX_REG_FRAME; i++) {
        if (ctx->pPPUFrame[i].size)
            vdi_free_dma_memory(coreIdx, &ctx->pPPUFbMem[i], DEC_ETC, ctx->handle->instIndex);
    }
}

static BOOL AllocateFrameBuffer(DecoderContext_t* ctx, vpu_buffer_t* outbuf)
{
    BOOL                 success;
    Uint32               compressedNum;
    Uint32               linearNum;
    ComponentParamRet    ret;

    ret = tivpu_get_parameter_decoder(ctx, GET_PARAM_DEC_FRAME_BUF_NUM, &ctx->fbCount);
    if (ParamReturnTest(ret, &success) == FALSE) {
        return FALSE;
    }

    osal_memset((void*)ctx->pFrame, 0x00, sizeof(ctx->pFrame));
    osal_memset((void*)ctx->pFbMem, 0x00, sizeof(ctx->pFbMem));

    compressedNum  = ctx->fbCount.nonLinearNum;
    linearNum      = ctx->fbCount.linearNum;

    /* Point Linear WTL FB addrs array to the outbuf addrs already allocated by the application */
    ctx->pFbWtlMem = outbuf;

    if (compressedNum == 0 && linearNum == 0) {
        VLOG(ERR, "%s:%d The number of framebuffers are zero. compressed %d, linear: %d\n",
            __FUNCTION__, __LINE__, compressedNum, linearNum);
        return FALSE;
    }

    if (AllocateDecFrameBuffer(ctx, compressedNum, linearNum, ctx->pFrame, ctx->pFbMem, ctx->pFbWtlMem, &ctx->framebufStride) == FALSE) {
        VLOG(INFO, "%s:%d Failed to AllocateDecFrameBuffer()\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    ctx->fbAllocated = TRUE;

    return TRUE;
}

ComponentParamRet tivpu_get_parameter_renderer(DecoderContext_t* ctx, GetParameterCMD commandType, void* data)
{
    ParamDecFrameBuffer*    allocFb = NULL;

    if (ctx->fbAllocated == FALSE) return COMPONENT_PARAM_NOT_READY;

    switch(commandType) {
    case GET_PARAM_RENDERER_FRAME_BUF:
        allocFb = (ParamDecFrameBuffer*)data;
        allocFb->stride        = ctx->framebufStride;
        allocFb->linearNum     = ctx->fbCount.linearNum;
        allocFb->nonLinearNum  = ctx->fbCount.nonLinearNum;
        allocFb->fb            = ctx->pFrame;
        break;
    default:
        return COMPONENT_PARAM_NOT_FOUND;
    }

    return COMPONENT_PARAM_SUCCESS;
}

ComponentParamRet tivpu_set_parameter_renderer(DecoderContext_t* ctx, SetParameterCMD commandType, void* data)
{
    BOOL                result = TRUE;

    UNREFERENCED_PARAMETER(ctx);
    switch(commandType) {
    case SET_PARAM_RENDERER_ALLOC_FRAMEBUFFERS:
        result = AllocateFrameBuffer(ctx, (vpu_buffer_t*)data);
        break;
    case SET_PARAM_RENDERER_RELEASE_FRAME_BUFFRES:
        ReleaseFrameBuffers(ctx);
        break;
    default:
        return COMPONENT_PARAM_NOT_FOUND;
    }

    if (result == TRUE) return COMPONENT_PARAM_SUCCESS;
    else                return COMPONENT_PARAM_FAILURE;
}

BOOL tivpu_prepare_renderer(DecoderContext_t* ctx, vpu_buffer_t* outbuf)
{
    BOOL ret;

    if (ctx->handle == NULL) {
        return FALSE;
    }

    ret = AllocateFrameBuffer(ctx, outbuf);
    if (ret == FALSE || ctx->fbAllocated == FALSE) {
        return ret;
    }

    return TRUE;
}


