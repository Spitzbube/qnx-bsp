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
#include <sys/mman.h>

#include "tivpu_enc.h"

int32_t tivpu_enc_open_params(void** hdl, tivpu_enc_config_t* enc_config)
{
    uint32_t     i;
    uint32_t     coreIdx = enc_config->coreIdx;
    EncOpenParam *open_params = NULL;
    EncoderContext_t* ctx = NULL;

    ctx = (EncoderContext_t*)osal_malloc(sizeof(EncoderContext_t));
    if (ctx == NULL) {
        codec_slogerr("Failed to allocate EncoderContext_t struct, size: %d\n", (int32_t)sizeof(EncoderContext_t));
        return RETCODE_FAILURE;
    }
    osal_memset((void*)ctx, 0, sizeof(EncoderContext_t));

    open_params = &ctx->encOpenParam;

    if (coreIdx < MAX_NUM_VPU_CORE) {
        codec_sloginfo("%s:%d Opening params for VPU index: %d \n", __FUNCTION__, __LINE__, coreIdx);
    }
    else {
        codec_slogerr("%s:%d VPU index of %d is not supported\n", __FUNCTION__, __LINE__, coreIdx);
        osal_free(ctx);
        return RETCODE_FAILURE;
    }

    if (SetupEncoderOpenParam(open_params, enc_config) == FALSE) {
        codec_slogerr("SetupEncoderOpenParam error\n");
        osal_free(ctx);
        return RETCODE_FAILURE;
    }

    ctx->handle                      = NULL;
    ctx->frameIdx                    = 0;
    ctx->fbCount.reconFbNum          = ctx->encOpenParam.sourceBufCount;
    ctx->fbCount.srcFbNum            = ctx->encOpenParam.sourceBufCount;
    ctx->srcStride                   = enc_config->stride;
    for (i=0; i<ENC_SRC_BUF_NUM ; i++ ) {
        ctx->encodedSrcFrmIdxArr[i] = 0;
        if ( ctx->encOpenParam.subFrameSyncEnable)
            ctx->encodedSrcFrmIdxArr[i] = 1;
    }
    osal_memset(&ctx->vbCustomLambda,  0x00, sizeof(vpu_buffer_t));
    osal_memset(&ctx->vbScalingList,   0x00, sizeof(vpu_buffer_t));
    osal_memset(&ctx->scalingList,     0x00, sizeof(UserScalingList));
    osal_memset(&ctx->customLambda[0], 0x00, sizeof(ctx->customLambda));
    osal_memset(&ctx->vbCustomMap[0],  0x00, sizeof(ctx->vbCustomMap));

#if 0
    if (ctx->encOpenParam.ringBufferEnable)
        ctx->numBsPortQueue = 10;
    else
        ctx->numBsPortQueue = ctx->encOpenParam.streamBufCount;
#endif

    *hdl = ctx;
    return RETCODE_SUCCESS;
}

int32_t tivpu_enc_src_buf_config(EncoderContext_t* ctx, tivpu_enc_config_t* enc_config)
{
    BOOL ret;

    ret = tivpu_create_yuv_feeder(ctx);

    if (ret == TRUE) {
        return RETCODE_SUCCESS;
    }
    else {
        return RETCODE_FAILURE;
    }
}

#define codec_dbg(...)    slogf(_SLOG_SETCODE(_SLOGC_PRIVATE_START, 55), _SLOG_INFO, __VA_ARGS__)
#if 0
void print_buf_info(const char *message, vpu_buffer_t *buf)
{
    if(message != NULL)
        codec_dbg("BI::%s",message);

    codec_dbg("BI::phys_addr =%lx", buf->phys_addr);
    codec_dbg("BI::size =%lx", buf->size);
    codec_dbg("BI::base =%lx", buf->base);
    codec_dbg("BI::virt_addr =%lx", buf->virt_addr);
    codec_dbg("BI::usr_info.usr_addr =%lx", buf->usr_info.usr_addr);
    codec_dbg("BI::usr_info.priv =%lx", buf->usr_info.priv);

}
#endif

/**************** Added for resmgr *********************************/
int32_t tivpu_enc_register_buffers(tivpu_context_t *vpu_ctx, vpu_buffer_t *bufs, int32_t nbuf, uint8_t bufDir)
{
    int i = 0;
    BOOL ret = FALSE;
    vpu_buffer_t *ctx_bufs = NULL;
    EncoderContext_t *ctx = NULL;

    if((vpu_ctx == NULL) || (bufs == NULL) || (!nbuf)) {
        return -1;
    }

    ctx = vpu_ctx->codec_ctx;

    // allocate and make a copy of all the vpu_buffer_t structs.
    ctx_bufs = (vpu_buffer_t *)osal_malloc(sizeof(vpu_buffer_t) * nbuf);
    if(!ctx_bufs) {
        codec_slogerr("vpu_enc:%s Failed to allocate mem for ctx_bufs", __func__);
        return -1;
    }


    // map the buffers to the resmgr address space.
    for (i = 0; i < nbuf; i++) {
        osal_memcpy((void *)&(ctx_bufs[i]), (void *)&(bufs[i]),sizeof(vpu_buffer_t));
        void *va = mmap64(NULL, ctx_bufs[i].size,
                    PROT_READ|PROT_WRITE|PROT_NOCACHE,
                    MAP_SHARED|MAP_PHYS,
                    NOFD, ctx_bufs[i].phys_addr);
        if (va == MAP_FAILED) {
            codec_slogerr("vxe_enc:%s Failed to get the virt mem: errno %d", __func__, errno);
            while(i > 0) {
                i--;
                munmap((void *)(ctx_bufs[i].virt_addr), ctx_bufs[i].size);
            }
            return (EINVAL);
        }
        // Now we the buffer mapped to the resmgr space.
        // Also cache the allocate size. The size attr is used internally within VPU.
        ctx_bufs[i].virt_addr = (unsigned long)va;
        ctx_bufs[i].usr_info.alloc_size = ctx_bufs[i].size;

        print_buf_info(NULL, &ctx_bufs[i]);
    }

    if(bufDir == 0) { /* TODO: include enum definition header here. Handle output buffers: TIVPU_BUFDIR_OUTPUT*/
            vpu_ctx->output_bufs = ctx_bufs;
            vpu_ctx->output_buf_num = nbuf;
            ctx->bsBuf.bs = ctx_bufs;
            ctx->bsBuf.num = nbuf;

        ret = tivpu_enc_bitstream_prepare(ctx);
        if(ret == FALSE) {
            // Maybe RETCODE_FAILURE can be used. Need to check.
            return -2; //TODO: add custom enum User should see OMX_ErrorInsufficientResources
        }

        ret = tivpu_prepare_encoder(ctx);
        if(ret == FALSE) {
            return -2; //TODO: add custom enum User should see OMX_ErrorInsufficientResources
        }


    } else { /* Handle input buffers: TIVPU_BUFDIR_INPUT*/
        vpu_ctx->input_bufs = ctx_bufs;
        ctx->state = ENCODER_STATE_OPEN;

        vpu_ctx->input_buf_num = nbuf;
        ctx->fbCount.srcFbNum = nbuf;

        while (ctx->state == ENCODER_STATE_OPEN)
        {
            ret = tivpu_execute_encoder(ctx, NULL, NULL, NULL);
            if (ret == FALSE)
            {
                codec_slogerr("OMXIL omxil_enc_register_buffers error: OpenEncoder failure");
                return -2; /* OMX_ErrorInsufficientResources*/
            }
        }
        while (ctx->state == ENCODER_STATE_INIT_SEQ)
        {
            ret = tivpu_execute_encoder(ctx, NULL, NULL, NULL);
            if (ret == FALSE)
            {
                codec_slogerr("OMXIL omxil_enc_register_buffers error: SetSequenceInfo failure");
                return -2; /* OMX_ErrorInsufficientResources*/
            }
        }

        ret = tivpu_prepare_yuv_feeder(ctx, ctx_bufs);
        if (ret == FALSE)
        {
            codec_slogerr("OMXIL omxil_enc_register_buffers error: tivpu_prepare_yuv_feeder failure");
                return -2; /* OMX_ErrorInsufficientResources*/
        }

        while (ctx->state == ENCODER_STATE_REGISTER_FB)
        {
            ret = tivpu_execute_encoder(ctx, NULL, NULL, NULL);
            if (ret == FALSE)
            {
                codec_slogerr("OMXIL omxil_enc_register_buffers error: RegisterFrameBuffers failure");
                return -2; /* OMX_ErrorInsufficientResources*/
            }
        }
    }

    return RETCODE_SUCCESS;
}



