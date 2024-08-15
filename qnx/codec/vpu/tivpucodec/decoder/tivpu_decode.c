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
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "tivpu_dec.h"
#include "misc/debug.h"

#define EXTRA_FRAME_BUFFER_NUM 1
#define DEC_DESTROY_TIME_OUT   (60000*2) //2 min


static BOOL RegisterFrameBuffers(DecoderContext_t* ctx)
{
    FrameBuffer*            pFrame            = NULL;
    Uint32                  framebufStride    = 0;
    ParamDecFrameBuffer     paramFb;
    RetCode                 result;
    DecInitialInfo*         codecInfo         = &ctx->initialInfo;
    BOOL                    success;
    ComponentParamRet       ret;

    ctx->stateDoing = TRUE;
    ret = tivpu_get_parameter_renderer(ctx, GET_PARAM_RENDERER_FRAME_BUF, (void*)&paramFb);
    if (ParamReturnTest(ret, &success) == FALSE) {
        return FALSE;
    }

    pFrame               = paramFb.fb;
    framebufStride       = paramFb.stride;
    VLOG(TRACE, "<%s> COMPRESSED: %d, LINEAR: %d\n", __FUNCTION__, paramFb.nonLinearNum, paramFb.linearNum);

    result = VPU_DecRegisterFrameBufferEx(ctx->handle, pFrame, paramFb.nonLinearNum, paramFb.linearNum,
                                              framebufStride, codecInfo->picHeight, COMPRESSED_FRAME_MAP);

    if (result != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d Failed to VPU_DecRegisterFrameBufferEx(%d)\n", __FUNCTION__, __LINE__, result);
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, result);
        return FALSE;
    }

    ctx->stateDoing = FALSE;

    return TRUE;
}

static Int32 CheckChromaFormatFlag(DecoderContext_t *ctx, DecOutputInfo* const outputInfo)
{
    Int32 chromaIDCFlag     = 0;
    Uint32 sequenceChangeFlag = outputInfo->sequenceChanged;

    if (0 != sequenceChangeFlag) {
        DecInfo decInfo;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, &decInfo);
        chromaIDCFlag = decInfo.initialInfo.chromaFormatIDC;
    } else {
        chromaIDCFlag = ctx->chromaIDCFlag;
    }
    return chromaIDCFlag;
}

/* TODO:: This can be moved to a single interrupt handler */
static DEC_INT_STATUS HandlingInterruptFlag(DecoderContext_t* ctx)
{
    DecHandle            handle            = ctx->handle;
    Int32                interruptFlag     = 0;
    Uint32               interruptWaitTime = VPU_WAIT_TIME_OUT_CQ;
    Uint32               interruptTimeout  = VPU_DEC_TIMEOUT;
    DEC_INT_STATUS       status            = DEC_INT_STATUS_NONE;

    if (1 < vdi_get_instance_num(VPU_HANDLE_CORE_INDEX(ctx->handle))) {
        interruptWaitTime = VPU_WAIT_TIME_OUT_LONG;//TODO : need to check in customer side
    }

    if (ctx->startTimeout == 0ULL) {
        ctx->startTimeout = osal_gettime();
    }
    do {
        interruptFlag = VPU_WaitInterruptEx(handle, interruptWaitTime);
        if (INTERRUPT_TIMEOUT_VALUE == interruptFlag) {
            Uint64   currentTimeout = osal_gettime();
            if (0 < interruptTimeout && (currentTimeout - ctx->startTimeout) > interruptTimeout) {
                VLOG(ERR, "\n INSNTANCE #%d INTERRUPT TIMEOUT.\n", handle->instIndex);
                status = DEC_INT_STATUS_TIMEOUT;
                break;
            }
            interruptFlag = 0;
        }

        if (interruptFlag < 0) {
            VLOG(ERR, "<%s:%d> interruptFlag is negative value! %08x\n", __FUNCTION__, __LINE__, interruptFlag);
        }

        if (interruptFlag > 0) {
            VPU_ClearInterruptEx(handle, interruptFlag);
            ctx->startTimeout = 0ULL;
            status = DEC_INT_STATUS_DONE;
            if (interruptFlag & (1<<INT_WAVE5_INIT_SEQ)) {
                break;
            }

            if (interruptFlag & (1<<INT_WAVE5_DEC_PIC)) {
                break;
            }

            if (interruptFlag & (1<<INT_WAVE5_BSBUF_EMPTY)) {
                status = DEC_INT_STATUS_EMPTY;
                break;
            }
        }
    } while (FALSE);

    return status;
}


static BOOL Decode(DecoderContext_t *ctx, vpu_buffer_t* inbuf, vpu_buffer_t* outbuf)
{
    DecOutputInfo                   decOutputInfo;
    DEC_INT_STATUS                  intStatus;
    RetCode                         result;
    BOOL                            doDecode = TRUE;
    QueueStatusInfo                 qStatus;

    ctx->stateDoing = TRUE;

    VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &qStatus);
    if (COMMAND_QUEUE_DEPTH == qStatus.instanceQueueCount) {
        doDecode = FALSE;
    }

    if (TRUE == doDecode) {

        result = VPU_DecStartOneFrame(ctx->handle, &ctx->decParam);

        if (result == RETCODE_SUCCESS) {
            ; // Success
        }
        else if (result == RETCODE_QUEUEING_FAILURE) {
            // Just retry
            VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &qStatus);
            if (qStatus.instanceQueueCount == 0) {
                VLOG(ERR, "<%s:%d> The queue is empty but it can't add a command\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
        }
        else if (result == RETCODE_VPU_RESPONSE_TIMEOUT) {
            VLOG(ERR, "<%s:%d> Failed to VPU_DecStartOneFrame() ret(%d)\n", __FUNCTION__, __LINE__, result);
            HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
            return FALSE;
        }
        else {
            VLOG(ERR, "VPU_DecStartOneFrame failed. Error code is 0x%x \n", result);
            ChekcAndPrintDebugInfo(ctx->handle, FALSE, result);
            return FALSE;
        }
    }
    else {
        if (inbuf) ctx->reuse = FALSE;
    }

    intStatus=HandlingInterruptFlag(ctx);

    switch (intStatus) {
    case DEC_INT_STATUS_TIMEOUT:
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, RETCODE_VPU_RESPONSE_TIMEOUT);
        VPU_SWReset(ctx->decOpenParam.coreIdx, SW_RESET_SAFETY, ctx->handle);
        return FALSE;
    case DEC_INT_STATUS_EMPTY:
    case DEC_INT_STATUS_NONE:
        return TRUE; // Try again
    default:
        break; // Success
    }

    // Get data from the sink component.
    osal_memset(&decOutputInfo, 0x00, sizeof(DecOutputInfo));
    result = VPU_DecGetOutputInfo(ctx->handle, &decOutputInfo);

    if (result == RETCODE_REPORT_NOT_READY) {
        VLOG(ERR, "Decode output report not yet ready, result = %d \n", result);
        /* Try one more time... */
        result = VPU_DecGetOutputInfo(ctx->handle, &decOutputInfo);
        if (result == RETCODE_REPORT_NOT_READY) {
            VLOG(ERR, "Decoder output report STILL not ready, result = %d \n", result);
            return FALSE; /* Can't get output report */
        }
        else if (result == RETCODE_SUCCESS) {
            /* SUCCESS */
            DisplayDecodedInformation(ctx->handle, ctx->decOpenParam.bitstreamFormat, ctx->numDecoded, &decOutputInfo, FALSE, ctx->cyclePerTick);
        }
        else {
            VLOG(ERR, "Failed to decode error\n");
            VPU_SWReset(ctx->decOpenParam.coreIdx, SW_RESET_SAFETY, ctx->handle);
            return FALSE;
        }
    }
    else if (result != RETCODE_SUCCESS) {
        /* ERROR */
        VLOG(ERR, "Failed to decode error\n");
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, result);
        VPU_SWReset(ctx->decOpenParam.coreIdx, SW_RESET_SAFETY, ctx->handle);
        return FALSE;
    }
    else {
        /* SUCCESS */
        DisplayDecodedInformation(ctx->handle, ctx->decOpenParam.bitstreamFormat, ctx->numDecoded, &decOutputInfo, FALSE, ctx->cyclePerTick);
    }

    if ((decOutputInfo.decodingSuccess & 0x01) == 0) {
        VLOG(ERR, "VPU_DecGetOutputInfo decode fail framdIdx %d error(0x%08x) reason(0x%08x), reasonExt(0x%08x)\n",
            ctx->numDecoded, decOutputInfo.decodingSuccess, decOutputInfo.errorReason, decOutputInfo.errorReasonExt);
        if (WAVE5_SYSERR_WATCHDOG_TIMEOUT == decOutputInfo.errorReason) {
            VLOG(ERR, "WAVE5_SYSERR_WATCHDOG_TIMEOUT\n");
        }
        else if (WAVE5_SYSERR_VLC_BUF_FULL == decOutputInfo.errorReason) {
            VLOG(ERR, "VLC_BUFFER FULL\n");
        }
        else if (HEVC_SPECERR_OVER_PICTURE_WIDTH_SIZE == decOutputInfo.errorReason || HEVC_SPECERR_OVER_PICTURE_HEIGHT_SIZE == decOutputInfo.errorReason) {
            VLOG(ERR, "Not supported Width or Height(%dx%d)\n", decOutputInfo.decPicWidth, decOutputInfo.decPicHeight);
        }
    }
    else {
        if (TRUE == ctx->autoErrorRecovery.enable) {
            ctx->decParam.skipframeMode     = ctx->autoErrorRecovery.skipCmd;
            ctx->autoErrorRecovery.enable   = FALSE;
            ctx->autoErrorRecovery.skipCmd  = WAVE_SKIPMODE_WAVE_NONE;
        }
    }

    codec_slogdbg("indexFrameDisplay = %d", decOutputInfo.indexFrameDisplay);
    codec_slogdbg("indexFrameDecoded = %d", decOutputInfo.indexFrameDecoded);

    ctx->idxDisplayFrame = decOutputInfo.indexFrameDisplay;
    ctx->idxDecodedFrame = decOutputInfo.indexFrameDecoded;

    if (decOutputInfo.indexFrameDecoded >= 0 || decOutputInfo.indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
        ctx->numDecoded++;
        ctx->decodedAddr = decOutputInfo.bytePosFrameStart;
        // Return a used data to a source port.
    }

    if ((decOutputInfo.indexFrameDisplay >= 0) || (decOutputInfo.indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END)) {
        ctx->numOutput++;
        ctx->chromaIDCFlag = CheckChromaFormatFlag(ctx, &decOutputInfo);
        if(ctx->delayDisplay > 0) {
            /* Lag of one display frame before clearing display flags for older frames */
            /* If this is a display frame, then release the prior one for re-use. */
            VPU_DecClrDispFlag(ctx->handle, ctx->dispFrameToClearNext);
        }
        else {
           ctx->delayDisplay = 1;
        }
        /* Update dispFrameToClearNext with next frame number */
        ctx->dispFrameToClearNext = decOutputInfo.indexFrameDisplay;
    }

    if (decOutputInfo.indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END) {
        ctx->stateDoing = FALSE;
        ctx->terminate  = TRUE;
    }

    osal_memcpy((void*)&ctx->decOutInfo, (void*)&decOutputInfo, sizeof(DecOutputInfo));

    return TRUE;
}

ComponentParamRet tivpu_get_parameter_decoder(DecoderContext_t* ctx, GetParameterCMD commandType, void* data)
{
    BOOL                        result  = TRUE;
    ParamDecNeedFrameBufferNum* fbNum;

    if (ctx->handle == NULL)  return COMPONENT_PARAM_NOT_READY;

    switch(commandType) {
    case GET_PARAM_DEC_HANDLE:
        *(DecHandle*)data = ctx->handle;
        break;
    case GET_PARAM_DEC_FRAME_BUF_NUM:
        if (ctx->state <= DEC_STATE_INIT_SEQ) return COMPONENT_PARAM_NOT_READY;
        fbNum = (ParamDecNeedFrameBufferNum*)data;

        if (ctx->numDecBuffers < ctx->initialInfo.minFrameBufferCount){
            fbNum->nonLinearNum = ctx->initialInfo.minFrameBufferCount; // max_dec_pic_buffering
            if (ctx->numDecBuffers != 0) // do not warn for using default config
                codec_slogwarn("%s:%d increasing decode buffer count to min\n", __FUNCTION__, __LINE__);
        }
        else {
            fbNum->nonLinearNum = ctx->numDecBuffers;
        }

        if (ctx->decOpenParam.wtlEnable == TRUE) {
            fbNum->linearNum = ctx->numOutBuffers;
            if (fbNum->linearNum < (ctx->initialInfo.frameBufDelay+1)) {
                VLOG(ERR, "%s:%d NUM_OUT_BUFFERS is too low (%d)\n", __FUNCTION__, __LINE__, fbNum->linearNum);
                VLOG(ERR, "Need to have at least this many output buffers: (%d)\n", ctx->initialInfo.frameBufDelay+1);
                result = FALSE;
            }
        }
        else {
            fbNum->linearNum = 0;
        }
        break;
    case GET_PARAM_DEC_CODEC_INFO:
        if (ctx->state <= DEC_STATE_INIT_SEQ) return COMPONENT_PARAM_NOT_READY;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, data);
        break;
    default:
        result = FALSE;
        break;
    }

    return (result == TRUE) ? COMPONENT_PARAM_SUCCESS : COMPONENT_PARAM_FAILURE;
}

static BOOL UpdateBitstream(DecoderContext_t* ctx, vpu_buffer_t* inbuf)
{
    RetCode         ret = RETCODE_SUCCESS;
    PhysicalAddress rdPtr, wrPtr;
    Uint32          room;
    Uint32          updateSize;
    BOOL            update = TRUE;

    VPU_DecGetBitstreamBuffer(ctx->handle, &rdPtr, &wrPtr, &room);

    codec_slogdbg("%s: rdPtr = 0x%lx wrPtr = 0x%lx room = 0x%x", __func__, rdPtr, wrPtr, room);

    ctx->rdPtr = rdPtr;

    if (inbuf == NULL) return TRUE;

    if (inbuf->size > 0) {
        if (room < inbuf->size) {
            ctx->reuse = TRUE;
            return TRUE;
        }
    }

    if (ctx->last == TRUE) {
        codec_slogdbg("%s: ctx->last = TRUE", __func__);
        updateSize = (inbuf->size == 0) ? STREAM_END_SET_FLAG : inbuf->size;
    }
    else {
        updateSize = inbuf->size;
        update     = (inbuf->size > 0);
    }


    if (update == TRUE) {
        if ((ret=VPU_DecUpdateBitstreamBuffer(ctx->handle, updateSize)) != RETCODE_SUCCESS) {
            VLOG(ERR, "<%s:%d> Failed to VPU_DecUpdateBitstreamBuffer() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            ChekcAndPrintDebugInfo(ctx->handle, FALSE, ret);
            return FALSE;
        }
        if (ctx->last == TRUE && updateSize != STREAM_END_SET_FLAG) {
            codec_slogdbg("%s:%d setting STREAM_END_SET_FLAG", __func__, __LINE__);
            VPU_DecUpdateBitstreamBuffer(ctx->handle, STREAM_END_SET_FLAG);
        }
    }

    ctx->reuse = FALSE;

    return TRUE;
}

static BOOL OpenDecoder(DecoderContext_t* ctx)
{
    ParamDecBitstreamBuffer bsBuf;
    ComponentParamRet       ret;
    BOOL                    success = FALSE;
    vpu_buffer_t            vbUserData;
    RetCode                 retCode;

    ctx->stateDoing = TRUE;
    ret = tivpu_get_parameter_feeder(ctx, GET_PARAM_FEEDER_BITSTREAM_BUF, &bsBuf);
    if (ParamReturnTest(ret, &success) == FALSE) {
        return FALSE;
    }

    ctx->decOpenParam.bitstreamBuffer     = bsBuf.bs->phys_addr;
    ctx->decOpenParam.bitstreamBufferSize = STREAM_BUF_SIZE_HEVC; //bsBuf.bs->size;
    retCode = VPU_DecOpen(&ctx->handle, &ctx->decOpenParam);

    if (retCode != RETCODE_SUCCESS) {
        VLOG(ERR, "<%s:%d> Failed to VPU_DecOpen(ret:%d)\n", __FUNCTION__, __LINE__, retCode);
        return FALSE;
    }
    // VPU_DecGiveCommand(ctx->handle, ENABLE_LOGGING, 0);

    vbUserData.size = (1320*1024);  /* 40KB * (queue_depth + report_queue_depth+1) = 40KB * (16 + 16 +1) */
    vdi_allocate_dma_memory(ctx->decOpenParam.coreIdx, &vbUserData, DEC_ETC, ctx->handle->instIndex);

    VPU_DecGiveCommand(ctx->handle, SET_ADDR_REP_USERDATA, (void*)&vbUserData.phys_addr);
    VPU_DecGiveCommand(ctx->handle, SET_SIZE_REP_USERDATA, (void*)&vbUserData.size);
    VPU_DecGiveCommand(ctx->handle, ENABLE_REP_USERDATA,   (void*)&ctx->enableUserData);

    VPU_DecGiveCommand(ctx->handle, SET_CYCLE_PER_TICK,   (void*)&ctx->cyclePerTick);

    ctx->vbUserData = vbUserData;
    ctx->stateDoing = FALSE;

    return TRUE;
}

static BOOL DecodeHeader(DecoderContext_t* ctx)
{
    DecHandle                      handle  = ctx->handle;
    Uint32                         coreIdx = ctx->decOpenParam.coreIdx;
    RetCode                        ret     = RETCODE_SUCCESS;
    DEC_INT_STATUS                 status;
    DecInitialInfo*                initialInfo = &ctx->initialInfo;
    SecAxiUse                      secAxiUse;

    if (ctx->stateDoing == FALSE) {
        /* previous state done */
        ret = VPU_DecIssueSeqInit(handle);
        if (RETCODE_QUEUEING_FAILURE == ret) {
            return TRUE; // Try again
        }

        if (ret != RETCODE_SUCCESS) {
            ChekcAndPrintDebugInfo(ctx->handle, FALSE, ret);
            VLOG(ERR, "%s:%d Failed to VPU_DecIssueSeqInit() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            return FALSE;
        }
    }

    ctx->stateDoing = TRUE;

    while (ctx->terminate == FALSE) {
        if ((status=HandlingInterruptFlag(ctx)) == DEC_INT_STATUS_DONE) {
            break;
        }
        else if (status == DEC_INT_STATUS_TIMEOUT) {
            HandleDecoderError(ctx->handle, 0, NULL);
            VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_SIZE);    /* To finish bitstream empty status */
            VPU_SWReset(coreIdx, SW_RESET_SAFETY, handle);
            VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_CLEAR_FLAG);    /* To finish bitstream empty status */
            return FALSE;
        }
        else if (status == DEC_INT_STATUS_EMPTY) {
            return TRUE;
        }
        else if (status == DEC_INT_STATUS_NONE) {
            return TRUE;
        }
        else {
            VLOG(INFO, "%s:%d Unknown interrupt status: %d\n", __FUNCTION__, __LINE__, status);
            return FALSE;
        }
    }

    ret = VPU_DecCompleteSeqInit(handle, initialInfo);

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d FAILED TO DEC_PIC_HDR: ret(%d), SEQERR(%08x)\n", __FUNCTION__, __LINE__, ret, initialInfo->seqInitErrReason);
        ChekcAndPrintDebugInfo(ctx->handle, FALSE, ret);
        return FALSE;
    }

    if (ctx->decOpenParam.wtlEnable) {
        VPU_DecGiveCommand(ctx->handle, DEC_SET_WTL_FRAME_FORMAT, &ctx->wtlFormat);
    }

   /* Setting up secondary AXI is dependent on the H/W configuration.
    * Note that we will disable the secondary AXI configuration
    * if there is no separate SRAM reserved for IP, LF and BIT.
    */
    secAxiUse.useIpEnable    = FALSE;
    secAxiUse.useLfRowEnable = FALSE;
    secAxiUse.useBitEnable   = FALSE;
    VPU_DecGiveCommand(ctx->handle, SET_SEC_AXI, &secAxiUse);

    ctx->first      = FALSE;
    ctx->stateDoing = FALSE;

    return TRUE;
}

static int tivpu_is_vpu_buf_inuse(tivpu_context_t *vpu_ctx, vpu_buffer_t *vbuf)
{
    DecoderContext_t *ctx = vpu_ctx->codec_ctx;
    uintptr_t vbuf_start = vbuf->phys_addr;
    uintptr_t read_ptr = ctx->rdPtr;

    /* is the readptr reading the current in buf location? */
    return ((vbuf_start <= read_ptr) && ((vbuf_start + vbuf->size) > read_ptr));
}

static void tivpu_mark_inbuf_inuse(tivpu_context_t *vpu_ctx)
{
    vpu_buffer_t *vbufs = vpu_ctx->input_bufs;

    for(int i = 0; i < vpu_ctx->input_buf_num; i++) {

        if(vbufs[i].usr_info.state == BUF_DRV_INQUEUE &&
                tivpu_is_vpu_buf_inuse(vpu_ctx, &vbufs[i])) {
            codec_slogdbg("phys_addr = 0x%lx set to BUF_DRV_INUSE", vbufs[i].phys_addr);
            vbufs[i].usr_info.state = BUF_DRV_INUSE;
            break;
        }
    }
}

static inline vpu_buffer_t *tivpu_get_vpu_buf(vpu_buffer_t *vbufs, int nbuf, void *buf_hdr)
{
    for(int i = 0; i < nbuf; i++) {
        if(vbufs[i].usr_info.priv == buf_hdr)
            return &(vbufs[i]);
    }
    return NULL;
}

static uintptr_t tivpu_get_return_buf(tivpu_context_t *vpu_ctx)
{
    vpu_buffer_t *vbufs = vpu_ctx->input_bufs;
    int num_inuse_bufs = 0;
    /* return the buffer in case the buffer was previously marked as used
     * but the readptr is out of the input buffers range 
     * NOTE: It might be worthwhile to make sure that there is only
     * one and  __ONLY__ one BUF_DRV_INUSE after this function call
     */
    for(int i = 0; i < vpu_ctx->input_buf_num; i++) {

        if(vbufs[i].usr_info.state == BUF_DRV_INUSE &&
                !tivpu_is_vpu_buf_inuse(vpu_ctx, &vbufs[i])) {
            codec_slogdbg("phys_addr = 0x%lx set to BUF_DRV_UNTRACKED", vbufs[i].phys_addr);
            vbufs[i].usr_info.state = BUF_DRV_UNTRACKED;
            return vbufs[i].phys_addr;
        }
    }

    for(int i = 0; i < vpu_ctx->input_buf_num; i++) {
        if(vbufs[i].usr_info.state == BUF_DRV_INUSE) {
            num_inuse_bufs++;
        }
    }

    if(num_inuse_bufs != 1)
        codec_slogwarn("INCONSISTENT BUFFER MANAGEMENT num_inuse_bufs = %d", num_inuse_bufs);
    return 0;
}

BOOL tivpu_execute_decoder(tivpu_context_t *vpu_ctx, vpu_buffer_t* inbuf, vpu_buffer_t* outbuf, void *ret_buf_info)
{
    BOOL ret    = FALSE;
    DecoderContext_t *ctx = vpu_ctx->codec_ctx;

    if (inbuf)  ctx->reuse = TRUE;
    if (ctx->state == DEC_STATE_INIT_SEQ || ctx->state == DEC_STATE_DECODING) {
        if (UpdateBitstream(ctx, inbuf) == FALSE) {
            return FALSE;
        }

        if (inbuf) {
            /* In ring-buffer mode, mark if the inbuf is actually consumed this time */
            ctx->consumed = (ctx->reuse == FALSE);
            codec_slogdbg("phys_addr = 0x%lx set to BUF_DRV_INQUEUE", inbuf->phys_addr);
            inbuf->usr_info.state = BUF_DRV_INQUEUE;
        }
        /* Move the appropriate buffer to inuse. */
        tivpu_mark_inbuf_inuse(vpu_ctx);
        if (ret_buf_info)
            *((uintptr_t *)(ret_buf_info)) = tivpu_get_return_buf(vpu_ctx);
    }

    switch (ctx->state) {
    case DEC_STATE_OPEN_DECODER:
        ret = OpenDecoder(ctx);
        if (ctx->stateDoing == FALSE) ctx->state = DEC_STATE_INIT_SEQ;
        break;
    case DEC_STATE_INIT_SEQ:
        ret = DecodeHeader(ctx);
        if (ctx->stateDoing == FALSE) ctx->state = DEC_STATE_REGISTER_FB;
        break;
    case DEC_STATE_REGISTER_FB:
        ret = RegisterFrameBuffers(ctx);
        if (ctx->stateDoing == FALSE) {
            ctx->state = DEC_STATE_DECODING;
            DisplayDecodedInformation(ctx->handle, ctx->decOpenParam.bitstreamFormat, 0, NULL, FALSE, 0);
        }
        break;
    case DEC_STATE_DECODING:
        ret = Decode(ctx, inbuf, outbuf);
        break;
    default:
        ret = FALSE;
        break;
    }

    if (ret == FALSE)
        ctx->terminate = TRUE;

    return ret;
}

void tivpu_release_decoder(DecoderContext_t* ctx)
{
    ctx->stateDoing = FALSE;
    ctx->iterationCnt = 0;
}

BOOL tivpu_destroy_decoder(DecoderContext_t* ctx)
{
    DEC_INT_STATUS  intStatus;
    BOOL            success     = TRUE;
    Uint32          i           = 0;
    Uint64          currentTime = 0;
    Uint32          timeout = DEC_DESTROY_TIME_OUT;
    RetCode         ret;

    if (NULL != ctx->handle) {
        if (FALSE == ctx->stateDoing) {
            ctx->desStTimeout = osal_gettime();
            if (RETCODE_SUCCESS != (ret=VPU_DecUpdateBitstreamBuffer(ctx->handle, STREAM_END_SET_FLAG))) {
                VLOG(WARN, "%s:%d Failed to VPU_DecUpdateBitstreamBuffer, ret(%08x) \n",__FUNCTION__, __LINE__, ret);
            }

            ctx->stateDoing = TRUE;
        }

        while (VPU_DecClose(ctx->handle) == RETCODE_VPU_STILL_RUNNING) {
            if ((intStatus=HandlingInterruptFlag(ctx)) == DEC_INT_STATUS_TIMEOUT) {
                HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
                VLOG(ERR, "<%s:%d> NO RESPONSE FROM VPU_DecClose()\n", __FUNCTION__, __LINE__);
                ctx->stateDoing = FALSE;
                success = FALSE;
                break;
            }
            else if (intStatus == DEC_INT_STATUS_DONE) {
                DecOutputInfo outputInfo;
                VLOG(INFO, "VPU_DecClose() : CLEAR REMAIN INTERRUPT\n");
                VPU_DecGetOutputInfo(ctx->handle, &outputInfo);
                continue;
            }

            for (i=0; i<MAX_REG_FRAME; i++) {
                VPU_DecClrDispFlag(ctx->handle, i);
            }

            currentTime = osal_gettime();
            if ( (currentTime - ctx->desStTimeout) > timeout) {
                VLOG(ERR, "\n INSNTANCE #%d VPU Close TIMEOUT.\n", ctx->handle->instIndex);
                ctx->stateDoing = FALSE;
                success = FALSE;
                break;
            }
            osal_msleep(10);
        }
    }

    tivpu_set_parameter_renderer(ctx, SET_PARAM_RENDERER_RELEASE_FRAME_BUFFRES, NULL);

    if (ctx->vbUserData.size && ctx->handle) {
        vdi_free_dma_memory(ctx->decOpenParam.coreIdx, &ctx->vbUserData, DEC_ETC, ctx->handle->instIndex);
    }


    ctx->stateDoing = FALSE;

    osal_free(ctx);

    return success;
}

/************************* Functions added for resmgr ******************************/
int32_t tivpu_dec_start(DecoderContext_t *ctx)
{
    if(ctx == NULL)
        return -1;

    ctx->first = TRUE;

    return 0;
}

int32_t tivpu_dec_stop(DecoderContext_t *ctx)
{

    if(ctx == NULL)
        return -1;

    ctx->last = TRUE;

    return 0;
}

void dump_dec_status( vpu_dec_status_t *ds)
{
    codec_slogdbg("RM:: is_first = %d", ds->is_first);
    codec_slogdbg("RM:: vpu_return_buf = %lx", ds->vpu_return_buf);
    codec_slogdbg("RM:: o_filled_len = %d", ds->o_filled_len);
    codec_slogdbg("RM:: displayed_frames = %d", ds->displayed_frames);
    codec_slogdbg("RM:: decoded_frames = %d", ds->decoded_frames);
    codec_slogdbg("RM:: terminate = %d", ds->terminate);
}

// Do the actual decode and get populate the decode status
static int32_t decode_helper(tivpu_context_t *vpu_ctx, vpu_buffer_t *i_buf, vpu_buffer_t *o_buf, vpu_dec_status_t *dec_status)
{
    int32_t err = EOK;
    DecoderContext_t *ctx = NULL;

    if(!vpu_ctx) {
        return -1;
    }

    ctx = vpu_ctx->codec_ctx;

    if(ctx->state != DEC_STATE_DECODING) {
        VLOG(ERR,   "%s:%d Error: Decoder not in decoding state", __func__, __LINE__);
        err = -2; /*OMX_ErrorHardware*/
    }

    if(o_buf != NULL && !(ctx->terminate)) {
        /* Decode a frame if possible */
        if(TRUE != tivpu_execute_decoder(vpu_ctx, i_buf, o_buf, &dec_status->vpu_return_buf)) {
            VLOG(ERR,   "%s:%d decode error", __func__, __LINE__);
            err = -2; /*OMX_ErrorHardware*/
        } else {

            dec_status->o_filled_len = o_buf->size;
            dec_status->displayed_frames = ctx->idxDisplayFrame;
            dec_status->decoded_frames = ctx->idxDecodedFrame;
            dec_status->terminate = ctx->terminate;

            dump_dec_status(dec_status);
#if defined (DEBUG_MODE)
            vpu_buffer_t *vbufs = vpu_ctx->input_bufs;
            for(int i = 0; i < vpu_ctx->input_buf_num; i++) {
                codec_slogdbg("%s: vbufs[%d] = %lx state = %d", __func__, i, vbufs[i].phys_addr, vbufs[i].usr_info.state);
            }
#endif

        }
    }

    /* Return the last remaining input buffer in queue */
    if (ctx->terminate) {
        vpu_buffer_t *vbufs = vpu_ctx->input_bufs;
        for(int i = 0; i < vpu_ctx->input_buf_num; i++) {
            if (vbufs[i].usr_info.state == BUF_DRV_INQUEUE) {
                dec_status->vpu_return_buf = vbufs[i].phys_addr;
                vbufs[i].usr_info.state = BUF_DRV_UNTRACKED;
                break;
            }
        }
    }

    return err;
}

// Moved the first frame decode to its own function
int32_t tivpu_decode_first(tivpu_context_t *vpu_ctx, vpu_buffer_t *in_buf, vpu_buffer_t *out_buf, uint32_t in_len, uint8_t is_eos, vpu_dec_status_t *dec_status)
{
    DecoderContext_t *ctx = NULL;
    int32_t err = EOK;

    if(!vpu_ctx) {
        return -1;
    }

    ctx = vpu_ctx->codec_ctx;

    if(ctx->state != DEC_STATE_INIT_SEQ) {
        err = -2; /*OMX_ErrorHardware*/
    }
    if(TRUE != tivpu_execute_decoder(vpu_ctx, in_buf, NULL, NULL)) {
        VLOG(ERR,   "%s:%d Decoder state init error and/or Decode Header error", __func__, __LINE__);
        err = -2; /*OMX_ErrorHardware*/
    }
    if(ctx->state != DEC_STATE_REGISTER_FB) {
        VLOG(ERR,   "%s:%d Unexpected device state %d ", __func__, __LINE__, ctx->state);
        err = -2; /*OMX_ErrorHardware*/
    }
    if(TRUE != tivpu_prepare_renderer(ctx, out_buf)) {
        VLOG(ERR,   "%s:%d AllocateFramebuffer error", __func__, __LINE__);
        err = -2; /*OMX_ErrorHardware*/
    }

    /* The i/p is already passed to the init seq and o/p to the prepare_renderer.
     * Pass null for this execute */
    if(TRUE != tivpu_execute_decoder(vpu_ctx, NULL, NULL, NULL)) {
        VLOG(ERR,   "%s:%d Register Framebuffers error", __func__, __LINE__);
        err = -2; /*OMX_ErrorHardware*/
    }

    if(ctx->state != DEC_STATE_DECODING) {
        VLOG(ERR, "%s:%d Error: Decoder not in decoding state", __func__, __LINE__);
        err = -2; /* OMX_ErrorHardware*/
    }


    return err;
}


int32_t tivpu_dec_process(tivpu_context_t *vpu_ctx, void *in_buf, void *out_buf, uint32_t in_len, uint8_t is_eos,
                                 vpu_dec_status_t *dec_status)
{
    DecoderContext_t *ctx = NULL;
    vpu_buffer_t *i_buf = NULL;
    vpu_buffer_t *o_buf = NULL;
    int32_t err = EOK;

    if(!vpu_ctx) {
        return -1;
    }

    ctx = vpu_ctx->codec_ctx;

    if(!ctx) {
        return -1;
    }

#if defined (DEBUG_MODE)
    vpu_buffer_t *vbufs = vpu_ctx->input_bufs;
    for(int i = 0; i < vpu_ctx->input_buf_num; i++) {
        codec_slogdbg("%s: vbufs[%d] = %lx state = %d", __func__, i, vbufs[i].phys_addr, vbufs[i].usr_info.state);
    }
#endif

    /* vpulib is done decoding the last frame. Don't decode anymore. */
    if (ctx->terminate) {
        /* Fill the decode status info */
        dec_status->o_filled_len = 0;
        dec_status->displayed_frames = ctx->idxDisplayFrame;
        dec_status->decoded_frames = ctx->idxDecodedFrame;
        dec_status->terminate = ctx->terminate;
        dump_dec_status(dec_status);

        return err;
    }

    /* get the input and output vpu_buffer_t */
    if(out_buf) {
        o_buf = tivpu_get_vpu_buf(vpu_ctx->output_bufs, vpu_ctx->output_buf_num, (void *)out_buf);
    }
    /* Always need a valid o_buf to continue decoding */
    if (o_buf == NULL) {
        VLOG(ERR, "%s:%d Error: No valid vpu_buffer_t output buffer to use for decoding", __func__, __LINE__);
        return -1;
    }

    if (in_buf) {
        i_buf = tivpu_get_vpu_buf(vpu_ctx->input_bufs, vpu_ctx->input_buf_num, in_buf);
        if (i_buf == NULL) {
            VLOG(ERR, "%s:%d Error: in_buf not successfully translated to vpu_buffer_t input buffer", __func__, __LINE__);
            return -1;
        }
        i_buf->size = in_len;

    } else {
        codec_slogdbg("%s: omx component passed a null input buffer", __func__);
    }

    if (is_eos) {
        codec_slogdbg("%s:%d EOS received", __func__, __LINE__);
        VLOG(TRACE, "%s:%d EOS received", __func__, __LINE__);
        ctx->last = TRUE;
    }

    /*
     * Output buf should be valid to call a decode. The other reason is for the initial setup.
     * - Handle the first call to process separately. This needs setting up the fb mem and
     *   walking through the sequence.
     */
    if((in_len > 0) && (ctx->first == TRUE)) {

        err = tivpu_decode_first(vpu_ctx, i_buf, o_buf, in_len, is_eos, dec_status);
        err = decode_helper(vpu_ctx, NULL, o_buf, dec_status);
    } else {
        err = decode_helper(vpu_ctx, i_buf, o_buf, dec_status);
    }

    return err;
}
