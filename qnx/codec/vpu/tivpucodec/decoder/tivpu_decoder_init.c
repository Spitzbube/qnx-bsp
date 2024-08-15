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

#include "tivpu_dec.h"

int32_t tivpu_dec_open_params(void** hdl, tivpu_dec_config_t* dec_config)
{
    uint32_t     coreIdx = dec_config->coreIdx;
    DecOpenParam *open_params = NULL;
    DecoderContext_t* ctx = NULL;

    ctx = (DecoderContext_t*)osal_malloc(sizeof(DecoderContext_t));
    if(!ctx) {
        codec_slogerr("vpu_dec:%s Failed to allocate mem for DecoderContext struct, ctx", __func__);
        return RETCODE_FAILURE;
    }
    osal_memset((void*)ctx, 0, sizeof(DecoderContext_t));

    open_params = &ctx->decOpenParam;

    if (coreIdx < MAX_NUM_VPU_CORE) {
        codec_sloginfo("%s:%d Opening params for VPU index: %d \n", __FUNCTION__, __LINE__, coreIdx);
    }
    else {
        codec_slogerr("%s:%d VPU index of %d is not supported\n", __FUNCTION__, __LINE__, coreIdx);
        osal_free(ctx);
        return RETCODE_FAILURE;
    }

    if (SetUpDecoderOpenParam(open_params, dec_config) != RETCODE_SUCCESS) {
        codec_slogerr("SetUpDecoderOpenParam error\n");
        osal_free(ctx);
        return RETCODE_FAILURE;
    }

    ctx->handle                       = NULL;
    ctx->wtlFormat                    = dec_config->format;
    ctx->numOutBuffers                = dec_config->numOutBufs;
    ctx->numDecBuffers                = dec_config->numDecBufs;
    ctx->enableUserData               = 0;
    ctx->numDecoded                   = 0;
    ctx->numOutput                    = 0;
    ctx->rdPtr                        = 0;
    ctx->triggerSwap                  = 0;
    ctx->state                        = DEC_STATE_OPEN_DECODER;
    ctx->stateDoing                   = FALSE;
    ctx->last                         = FALSE;
    ctx->terminate                    = FALSE;
    ctx->reuse                        = FALSE;
    ctx->consumed                     = FALSE;
    ctx->streamEndFlag                = FALSE;
    ctx->fbAllocated                  = FALSE;
    ctx->enablePPU                    = FALSE;
    ctx->delayDisplay                 = 0;
    ctx->dispFrameToClearNext         = -1;
    VLOG(INFO, "PRODUCT ID: %d\n", ctx->attr.productId);

    *hdl = ctx;
    return RETCODE_SUCCESS;
}



/***************** Added/modified for resource manager implementation ******/
int32_t tivpu_dec_register_buffers(tivpu_context_t *vpu_ctx, vpu_buffer_t *bufs, int32_t nbuf, uint8_t bufDir)
{
    int i = 0;
    BOOL ret = FALSE;
    vpu_buffer_t *ctx_bufs = NULL;
    DecoderContext_t *ctx = NULL;

    if((vpu_ctx == NULL) || (bufs == NULL) || (!nbuf)) {
        return -1;
    }

    ctx = vpu_ctx->codec_ctx;

    // allocate and make a copy of all the vpu_buffer_t structs.
    ctx_bufs = (vpu_buffer_t *)osal_malloc(sizeof(vpu_buffer_t) * nbuf);
    if(!ctx_bufs) {
        codec_slogerr("vpu_dec:%s Failed to allocate mem for ctx_bufs", __func__);
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
            codec_slogerr("vpu_dec:%s Failed to get the virt mem: errno %d", __func__, errno);
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
        ctx_bufs[i].usr_info.state = BUF_DRV_UNTRACKED;

        print_buf_info(NULL, &ctx_bufs[i]);
    }

    if(bufDir == 0) { /* TODO: include enum definition header here. Handle output buffers: TIVPU_BUFDIR_OUTPUT*/
        vpu_ctx->output_bufs = ctx_bufs;
        vpu_ctx->output_buf_num = nbuf;
    } else { /* Handle input buffers: TIVPU_BUFDIR_INPUT*/
        vpu_ctx->input_bufs = ctx_bufs;
        vpu_ctx->input_buf_num = nbuf;

        ctx->bsBuffer = ctx_bufs;
        ctx->numBuffers = nbuf;

        ret = tivpu_prepare_feeder(ctx);
        if(ret == FALSE) {
            return -1 /* OMX_ErrorInsufficientResources*/;
        }

        ctx->state = DEC_STATE_OPEN_DECODER;
        while(ctx->state == DEC_STATE_OPEN_DECODER) {
            ret = tivpu_execute_decoder(vpu_ctx, NULL, NULL, NULL);
            if (ret == FALSE) {
                codec_slogerr("OMXIL omxil_dec_register_buffers error: OpenDecoder failure");
                return -1; /*OMX_ErrorInsufficientResources*/
            }
        }
    }

    return RETCODE_SUCCESS;
}

#if 0 // This will be part of the codec init common code.
int32_t tivpu_dec_get_product_info(uint32_t coreIdx, DecoderContext_t* ctx)
{
    int32_t retval;

    retval = VPU_GetProductInfo(coreIdx, &ctx->attr);
    if (retval != RETCODE_SUCCESS) {
        codec_slogerr("%s:%d Failure in VPU_GetProductInfo, ret(%08x)\n", __FUNCTION__, __LINE__, retval);
        return retval;
    }

    /* Set cyclePerTick value based on attributes obtained from the VPU */
    ctx->cyclePerTick = 32768;
    if (TRUE == ctx->attr.supportNewTimer)
        ctx->cyclePerTick = 256;

    /* Print VPU Product Info */
    codec_sloginfo("VPU coreNum : [%d]\n", coreIdx);
    codec_sloginfo("Firmware : CustomerCode: %04x | version : rev.%d\n", ctx->attr.customerId, ctx->attr.fwVersion);
    codec_sloginfo("Hardware : %04x\n", ctx->attr.productId);
    codec_sloginfo("API      : %d.%d.%d\n\n", API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    codec_sloginfo("fwVersion       : %08x(r%d)\n", ctx->attr.fwVersion, ctx->attr.fwVersion);
    codec_sloginfo("productName     : %s%4x\n", ctx->attr.productName, ctx->attr.productVersion);

    return retval;
}
#endif
