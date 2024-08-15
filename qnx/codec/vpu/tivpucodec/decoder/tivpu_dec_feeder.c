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
#include <time.h>

#include "tivpu_dec.h"

ComponentParamRet tivpu_get_parameter_feeder(DecoderContext_t* ctx, GetParameterCMD commandType, void* data)
{
    ParamDecBitstreamBuffer*    bsBuf   = NULL;

    switch(commandType) {
    case GET_PARAM_FEEDER_BITSTREAM_BUF:
        if (ctx->bsBuffer == NULL) return COMPONENT_PARAM_NOT_READY;
        bsBuf = (ParamDecBitstreamBuffer*)data;
        bsBuf->num = ctx->numBuffers;
        bsBuf->bs  = ctx->bsBuffer;
        break;
    default:
        return COMPONENT_PARAM_NOT_FOUND;
    }

    return COMPONENT_PARAM_SUCCESS;
}

BOOL tivpu_prepare_feeder(DecoderContext_t* ctx)
{
    Uint32          i;
    vpu_buffer_t*   bsBuffer;
    Uint32          num;

    ctx->loopCount  = 0;
    if (ctx->decOpenParam.bitstreamMode == BS_MODE_INTERRUPT) {
        ctx->numBuffers = 1;
        ctx->bsSize = STREAM_BUF_SIZE_HEVC;
    }

    num = ctx->numBuffers;

    bsBuffer = ctx->bsBuffer;
    for (i = 0; i < num; i++) {
        bsBuffer[i].size = ctx->bsSize;
        if (vdi_attach_dma_memory(ctx->decOpenParam.coreIdx, &bsBuffer[i]) < 0) { 
            VLOG(ERR, "%s:%d failed to vdi_attach to bitstream buffer\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
    }

    if (ctx->decOpenParam.bitstreamMode == BS_MODE_INTERRUPT) {
        ctx->nextWrPtr = bsBuffer[0].phys_addr;
    }

    return TRUE;
}

void tivpu_release_feeder(DecoderContext_t* ctx)
{
    Uint32 i = 0;

    if (NULL != ctx->bsBuffer) {
        for (i = 0; i < ctx->numBuffers; i++) {
            vdi_dettach_dma_memory(ctx->decOpenParam.coreIdx, &ctx->bsBuffer[i]);
        }
    }
}


