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

#include "tivpu_enc.h"
#include "misc/debug.h"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#define ENC_DESTROY_TIME_OUT (60000*2) //2 min


static void SetEncPicParam(EncoderContext_t* ctx, vpu_buffer_t* inBuf, vpu_buffer_t* outBuf, EncParam* encParam)
{
    encParam->picStreamBufferAddr                = outBuf->phys_addr;
    encParam->picStreamBufferSize                = outBuf->size;
    encParam->srcIdx                             = 0;
    encParam->srcEndFlag                         = ctx->last;
    encParam->sourceFrame                        = &ctx->fb;
    /*
    if (encOpenParam.useAsLongtermPeriod > 0 && testEncConfig.refLongtermPeriod > 0) {
        encParam->useCurSrcAsLongtermPic         = (frameIdx % testEncConfig.useAsLongtermPeriod) == 0 ? 1 : 0;
        encParam->useLongtermRef                 = (frameIdx % testEncConfig.refLongtermPeriod)   == 0 ? 1 : 0;
    }
    */
    encParam->skipPicture                        = 0;
    encParam->forceAllCtuCoefDropEnable          = 0;

    encParam->forcePicQpEnable                   = 0;
    encParam->forcePicQpI                        = 0;
    encParam->forcePicQpP                        = 0;
    encParam->forcePicQpB                        = 0;
    encParam->forcePicTypeEnable                 = 0;
    encParam->forcePicType                       = 0;
    /*
    if (testEncConfig.forceIdrPicIdx == frameIdx) {
        encParam->forcePicTypeEnable = 1;
        encParam->forcePicType = 3;    // IDR
    }
    */

    // FW will encode header data implicitly when changing the header syntaxes
    encParam->codeOption.implicitHeaderEncode    = 1;
    encParam->codeOption.encodeAUD               = 0;

    encParam->codeOption.encodeEOS               = 0;
    encParam->codeOption.encodeEOB               = 0;
}

static BOOL RegisterFrameBuffers(EncoderContext_t* ctx)
{
    FrameBuffer*            pReconFb      = NULL;
    FrameBuffer*            pSrcFb        = NULL;
    FrameBufferAllocInfo    srcFbAllocInfo;
    Uint32                  reconFbStride = 0;
    Uint32                  reconFbHeight = 0;
    ParamEncFrameBuffer     paramFb;
    RetCode                 result;
    ComponentParamRet       ret;
    BOOL                    success;
    TiledMapType            mapType;
    ctx->stateDoing = TRUE;
    ret = tivpu_get_parameter_yuv_feeder(ctx, GET_PARAM_YUVFEEDER_FRAME_BUF, (void*)&paramFb);
    if (ParamReturnTest(ret, &success) == FALSE) return FALSE;

    pReconFb      = paramFb.reconFb;
    reconFbStride = paramFb.reconFbAllocInfo.stride;
    reconFbHeight = paramFb.reconFbAllocInfo.height;

    if ((ctx->attr.productId == PRODUCT_ID_521) && ctx->attr.supportDualCore == TRUE) {
        mapType = ctx->encOpenParam.EncStdParam.waveParam.internalBitDepth == 8 ? COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT : COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT;
    }
    else {
        mapType = COMPRESSED_FRAME_MAP;
    }
    result = VPU_EncRegisterFrameBuffer(ctx->handle, pReconFb, ctx->fbCount.reconFbNum, reconFbStride, reconFbHeight, mapType);
    if (result != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d Failed to VPU_EncRegisterFrameBuffer(%d)\n", __FUNCTION__, __LINE__, result);
        ChekcAndPrintDebugInfo(ctx->handle, TRUE, result);
        return FALSE;
    }

    pSrcFb = paramFb.srcFb;
    srcFbAllocInfo = paramFb.srcFbAllocInfo;
    result = VPU_EncAllocateFrameBuffer(ctx->handle, srcFbAllocInfo, pSrcFb);
    if (result != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_EncAllocateFrameBuffer fail to allocate source frame buffer\n");
        ChekcAndPrintDebugInfo(ctx->handle, TRUE, result);
        return FALSE;
    }

    ctx->stateDoing = FALSE;

    return TRUE;
}

ComponentParamRet tivpu_get_parameter_encoder(EncoderContext_t* ctx, GetParameterCMD commandType, void* data)
{
    BOOL result  = TRUE;

    switch(commandType) {
    case GET_PARAM_COM_IS_CONTAINER_CONUSUMED:
        break;
    case GET_PARAM_SRC_FRAME_INFO:
        break;
    case GET_PARAM_ENC_HANDLE:
        if (ctx->handle == NULL) return COMPONENT_PARAM_NOT_READY;
        break;
    case GET_PARAM_ENC_FRAME_BUF_NUM:
        if (ctx->fbCountValid == FALSE) return COMPONENT_PARAM_NOT_READY;
        break;
    case GET_PARAM_ENC_FRAME_BUF_REGISTERED:
        if (ctx->state <= ENCODER_STATE_REGISTER_FB) return COMPONENT_PARAM_NOT_READY;
        *(BOOL*)data = TRUE;
        break;
    default:
        result = FALSE;
        break;
    }

    return (result == TRUE) ? COMPONENT_PARAM_SUCCESS : COMPONENT_PARAM_FAILURE;
}

static ENC_INT_STATUS HandlingInterruptFlag(EncoderContext_t* ctx)
{
    EncHandle       handle                = ctx->handle;
    Int32           interruptFlag         = 0;
    Uint32          interruptWaitTime     = VPU_WAIT_TIME_OUT_CQ;
    Uint32          interruptTimeout      = VPU_ENC_TIMEOUT;
    ENC_INT_STATUS  status                = ENC_INT_STATUS_NONE;

    if (1 < vdi_get_instance_num(VPU_HANDLE_CORE_INDEX(ctx->handle))) {
        interruptWaitTime = VPU_WAIT_TIME_OUT_LONG;
    }

    if (ctx->startTimeout == 0ULL) {
        ctx->startTimeout = osal_gettime();
    }
    do {
        interruptFlag = VPU_WaitInterruptEx(handle, interruptWaitTime);
        if (INTERRUPT_TIMEOUT_VALUE == interruptFlag) {
            Uint64   currentTimeout = osal_gettime();

            if ((currentTimeout - ctx->startTimeout) > interruptTimeout) {
                VLOG(ERR, "<%s:%d> startTimeout(%lld) currentTime(%lld) diff(%d)\n",
                    __FUNCTION__, __LINE__, ctx->startTimeout, currentTimeout, (Uint32)(currentTimeout - ctx->startTimeout));
                status = ENC_INT_STATUS_TIMEOUT;
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

            if (interruptFlag & (1<<INT_WAVE5_ENC_SET_PARAM)) {
                status = ENC_INT_STATUS_DONE;
                break;
            }

            if (interruptFlag & (1<<INT_WAVE5_ENC_PIC)) {
                status = ENC_INT_STATUS_DONE;
                break;
            }

            if (interruptFlag & (1<<INT_WAVE5_BSBUF_FULL)) {
                status = ENC_INT_STATUS_FULL;
                break;
            }

#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
            if (interruptFlag & (1 << INT_WAVE5_ENC_SRC_RELEASE)) {
                status = ENC_INT_STATUS_SRC_RELEASED;
            }
#endif
        }
    } while (FALSE);

    return status;
}

static BOOL SetSequenceInfo(EncoderContext_t* ctx)
{
    EncHandle       handle  = ctx->handle;
    RetCode         ret     = RETCODE_SUCCESS;
    ENC_INT_STATUS  status;
    EncInitialInfo* initialInfo = &ctx->initialInfo;
    Uint32 src_buf_num = ctx->fbCount.srcFbNum;

    if (ctx->stateDoing == FALSE) {
        do {
            ret = VPU_EncIssueSeqInit(handle);
        } while (ret == RETCODE_QUEUEING_FAILURE && ctx->terminate == FALSE);

        if (ret != RETCODE_SUCCESS) {
            VLOG(ERR, "%s:%d Failed to VPU_EncIssueSeqInit() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            ChekcAndPrintDebugInfo(ctx->handle, TRUE, ret);
            return FALSE;
        }
    }
    ctx->stateDoing = TRUE;

    while (ctx->terminate == FALSE) {
        if ((status=HandlingInterruptFlag(ctx)) == ENC_INT_STATUS_DONE) {
            break;
        }
        else if (status == ENC_INT_STATUS_NONE) {
            return TRUE;
        }
        else if (status == ENC_INT_STATUS_TIMEOUT) {
            VLOG(INFO, "%s:%d INSTANCE #%d INTERRUPT TIMEOUT\n", __FUNCTION__, __LINE__, handle->instIndex);
            HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
            return FALSE;
        }
        else {
            VLOG(INFO, "%s:%d Unknown interrupt status: %d\n", __FUNCTION__, __LINE__, status);
            return FALSE;
        }
    }

    if ((ret=VPU_EncCompleteSeqInit(handle, initialInfo)) != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d FAILED TO ENC_PIC_HDR: ret(%d), SEQERR(%08x)\n",
            __FUNCTION__, __LINE__, ret, initialInfo->seqInitErrReason);
        ChekcAndPrintDebugInfo(ctx->handle, TRUE, ret);
        return FALSE;
    }

    ctx->fbCount.reconFbNum = initialInfo->minFrameBufferCount;

    ctx->fbCount.srcFbNum   = initialInfo->minSrcFrameCount + COMMAND_QUEUE_DEPTH + EXTRA_SRC_BUFFER_NUM;

    if ( ctx->encOpenParam.sourceBufCount > ctx->fbCount.srcFbNum)
        ctx->fbCount.srcFbNum = ctx->encOpenParam.sourceBufCount;


    if ( ctx->subFrameSyncCfg.subFrameSyncOn == TRUE) {
        if ( ctx->fbCount.srcFbNum > 5 ) {
            ctx->fbCount.srcFbNum = 5;
            VLOG(TRACE, "Set src frame buffer number to 5 if subFrameSync is enabled(constraint)\n");
        }
    }
    ctx->fbCountValid = TRUE;

    /* srcFbNum is used to index into the input buffer array inside SetFbDetails().
     * Limit the size of the srcFbNum to be at most the size of the
     * input buffer array
     */
    if (ctx->fbCount.srcFbNum > src_buf_num)
        ctx->fbCount.srcFbNum = src_buf_num;

    VLOG(INFO, "[ENCODER] Required  reconFbCount=%d, srcFbCount=%d, %dx%d\n",
        ctx->fbCount.reconFbNum, ctx->fbCount.srcFbNum, ctx->encOpenParam.picWidth, ctx->encOpenParam.picHeight);
    ctx->stateDoing = FALSE;

    return TRUE;
}

static BOOL Encode(EncoderContext_t* ctx, vpu_buffer_t* inbuf, vpu_buffer_t* outbuf, vpu_buffer_t* outbuf_first)
{
    BOOL                    doEncode        = FALSE;
    //BOOL                    doChangeParam   = FALSE;
    EncHeaderParam          encHeaderParam;
    EncParam*               encParam        = &ctx->encParam;
    EncOutputInfo           encOutputInfo;
    ENC_INT_STATUS          intStatus;
    RetCode                 result;
    ENC_QUERY_WRPTR_SEL     encWrPtrSel     = GET_ENC_PIC_DONE_WRPTR;
    QueueStatusInfo         qStatus;
    int i=0;

    ctx->stateDoing = TRUE;

    if (ctx->first == TRUE) {
        /* First Frame Only: generate bitstream HEADER and put into outbuf_first buffer */

        osal_memset(&encHeaderParam, 0x00, sizeof(EncHeaderParam));
        encHeaderParam.encodeAUD = 0;
        if (outbuf_first) {
            encHeaderParam.buf       = outbuf_first->phys_addr;
            encHeaderParam.size      = outbuf_first->size;
        }
        else {
            VLOG(ERR, "First frame header gen: First output buffer pointer is NULL - nothing to fill.\n");
            return FALSE;
        }

        if (ctx->encOpenParam.bitstreamFormat == STD_HEVC) {
            encHeaderParam.headerType = CODEOPT_ENC_VPS | CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;
        }
        else {
            /* H.264 */
            encHeaderParam.headerType = CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;
        }
        while(1) {
            result = VPU_EncGiveCommand(ctx->handle, ENC_PUT_VIDEO_HEADER, &encHeaderParam);
            if ( result != RETCODE_QUEUEING_FAILURE ) {
                break;
            }
            else {
                VLOG(ERR, "VPU_EncGiveCommand for ENC_PUT_VIDEO_HEADER failed. Error code is 0x%x \n", result);
                ChekcAndPrintDebugInfo(ctx->handle, TRUE, result);
                return FALSE;
            }
        }

        DisplayEncodedInformation(ctx->handle, ctx->encOpenParam.bitstreamFormat, 0, NULL, 0, 0);

        /* Check VPU interrupt for completed operation */
        if ((intStatus=HandlingInterruptFlag(ctx)) == ENC_INT_STATUS_TIMEOUT) {
            ChekcAndPrintDebugInfo(ctx->handle, TRUE, RETCODE_VPU_RESPONSE_TIMEOUT);
            HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
            VPU_SWReset(ctx->encOpenParam.coreIdx, SW_RESET_SAFETY, ctx->handle);
            return FALSE;
        }
        else if (intStatus == ENC_INT_STATUS_DONE) {
            ; /* VPU successfully wrote the Header to the output buffer */
        }
        else {
            VLOG(ERR, "Unexpected VPU interrupt status after PUT HEADER cmd., intStatus = 0x%x \n", intStatus);
            ctx->headerSize = 0;
            return FALSE;
        }

        /* Get VPU output info after it completed the ENC_PUT_VIDEO_HEADER */
        osal_memset(&encOutputInfo, 0x00, sizeof(EncOutputInfo));
        result = VPU_EncGetOutputInfo(ctx->handle, &encOutputInfo);
        if (result == RETCODE_REPORT_NOT_READY) {
            VLOG(ERR, "Header fill report not yet ready, result = %d \n", result);
            /* Try one more time... */
            result = VPU_EncGetOutputInfo(ctx->handle, &encOutputInfo);
            if (result == RETCODE_REPORT_NOT_READY) {
                VLOG(ERR, "Header fill report STILL not ready, result = %d \n", result);
                return FALSE; /* Can't get output report */
            }
        }
        else if (result == RETCODE_VLC_BUF_FULL) {
            VLOG(ERR, "VLC BUFFER FULL!!! ALLOCATE MORE TASK BUFFER(%d)!!!\n", ONE_TASKBUF_SIZE_FOR_CQ);
            return FALSE;
        }
        else if (result != RETCODE_SUCCESS) {
            /* ERROR */
            VLOG(ERR, "Failed to fill header error = %d, %x\n", result, encOutputInfo.errorReason);
            ChekcAndPrintDebugInfo(ctx->handle, TRUE, result);
            HandleEncoderError(ctx->handle, encOutputInfo.encPicCnt, &encOutputInfo);
            VPU_SWReset(ctx->encOpenParam.coreIdx, SW_RESET_SAFETY, ctx->handle);
            return FALSE;
        }
        else {
            ;/* SUCCESS */
        }

        ctx->headerSize = encOutputInfo.bitstreamSize;
        ctx->first = FALSE;
    }

    if (inbuf && outbuf) {
        SetEncPicParam(ctx, inbuf, outbuf, encParam);
        doEncode = TRUE;
    }
    else {
        return FALSE;
    }

    VPU_EncGiveCommand(ctx->handle, ENC_GET_QUEUE_STATUS, &qStatus);
    if (COMMAND_QUEUE_DEPTH == qStatus.instanceQueueCount) {
        doEncode = FALSE;
    }

    /*
    if (doChangeParam == TRUE) {
        result = SetChangeParam(ctx->handle, ctx->testEncConfig, ctx->encOpenParam, ctx->changedCount);
        if (result == RETCODE_SUCCESS) {
            VLOG(TRACE, "ENC_SET_PARA_CHANGE queue success\n");
            ctx->changedCount++;
        }
        else if (result == RETCODE_QUEUEING_FAILURE) { // Just retry
            VLOG(INFO, "ENC_SET_PARA_CHANGE Queue Full\n");
            doEncode  = FALSE;
        }
        else { // Error
            VLOG(ERR, "VPU_EncGiveCommand[ENC_SET_PARA_CHANGE] failed Error code is 0x%x \n", result);
            ChekcAndPrintDebugInfo(ctx->handle, TRUE, result);
            return FALSE;
        }
    }
    */

    if (doEncode == TRUE) {
        result = VPU_EncStartOneFrame(ctx->handle, encParam);
        if (result == RETCODE_SUCCESS) {
            ctx->frameIdx++;
        }
        else if (result == RETCODE_QUEUEING_FAILURE) {
            // Just retry
            VPU_EncGiveCommand(ctx->handle, ENC_GET_QUEUE_STATUS, (void*)&qStatus);
            if (qStatus.instanceQueueCount == 0) {
                VLOG(ERR, "<%s:%d> The queue is empty but it can't add a command\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
        }
        else { // Error
            VLOG(ERR, "VPU_EncStartOneFrame failed Error code is 0x%x \n", result);
            ChekcAndPrintDebugInfo(ctx->handle, TRUE, result);
            //CNMErrorSet(CNM_ERROR_HANGUP);
            HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
            return FALSE;
        }
    }

    if ((intStatus=HandlingInterruptFlag(ctx)) == ENC_INT_STATUS_TIMEOUT) {
        ChekcAndPrintDebugInfo(ctx->handle, TRUE, RETCODE_VPU_RESPONSE_TIMEOUT);
        HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
        VPU_SWReset(ctx->encOpenParam.coreIdx, SW_RESET_SAFETY, ctx->handle);
        return FALSE;
    }
    else if (intStatus == ENC_INT_STATUS_FULL || intStatus == ENC_INT_STATUS_LOW_LATENCY) {
        PhysicalAddress         paRdPtr;
        PhysicalAddress         paWrPtr;
        int                     size;

        encWrPtrSel = (intStatus==ENC_INT_STATUS_FULL) ? GET_ENC_BSBUF_FULL_WRPTR : GET_ENC_LOW_LATENCY_WRPTR;
        VPU_EncGiveCommand(ctx->handle, ENC_WRPTR_SEL, &encWrPtrSel);
        VPU_EncGetBitstreamBuffer(ctx->handle, &paRdPtr, &paWrPtr, &size);
        VLOG(TRACE, "<%s:%d> INT_BSBUF_FULL inst=%d, %p, %p\n", __FUNCTION__, __LINE__, ctx->handle->instIndex, paRdPtr, paWrPtr);

        if (outbuf) {
            ctx->outSize  = size;
            ctx->streamBufFull = TRUE;
        }
        ctx->fullInterrupt = TRUE;
        return TRUE;
    }
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
    else if (intStatus == ENC_INT_STATUS_SRC_RELEASED) {
        Uint32 srcBufFlag = 0;
        VPU_EncGiveCommand(ctx->handle, ENC_GET_SRC_BUF_FLAG, &srcBufFlag);
        for (i = 0; i < ctx->fbCount.srcFbNum; i++) {
            if ( (srcBufFlag >> i) & 0x01) {
                ctx->encodedSrcFrmIdxArr[i] = 1;
            }
        }
        return TRUE;
    }
#endif
    else if (intStatus == ENC_INT_STATUS_NONE) {
        if (outbuf) {
            ctx->outSize  = 0;
        }
        return TRUE; /* Try again */
    }

    VPU_EncGiveCommand(ctx->handle, ENC_WRPTR_SEL, &encWrPtrSel);
    osal_memset(&encOutputInfo, 0x00, sizeof(EncOutputInfo));
    result = VPU_EncGetOutputInfo(ctx->handle, &encOutputInfo);
    if (result == RETCODE_REPORT_NOT_READY) {
        VLOG(ERR, "Encode output report not yet ready, result = %d \n", result);
        /* Try one more time... */
        result = VPU_EncGetOutputInfo(ctx->handle, &encOutputInfo);
        if (result == RETCODE_REPORT_NOT_READY) {
            VLOG(ERR, "Encoder output report STILL not ready, result = %d \n", result);
            return FALSE; /* Can't get output report */
        }
    }
    else if (result == RETCODE_VLC_BUF_FULL) {
        VLOG(ERR, "VLC BUFFER FULL!!! ALLOCATE MORE TASK BUFFER(%d)!!!\n", ONE_TASKBUF_SIZE_FOR_CQ);
    }
    else if (result != RETCODE_SUCCESS) {
        /* ERROR */
        VLOG(ERR, "Failed to encode error = %d, %x\n", result, encOutputInfo.errorReason);
        ChekcAndPrintDebugInfo(ctx->handle, TRUE, result);
        HandleEncoderError(ctx->handle, encOutputInfo.encPicCnt, &encOutputInfo);
        VPU_SWReset(ctx->encOpenParam.coreIdx, SW_RESET_SAFETY, ctx->handle);
        return FALSE;
    }
    else {
        ;/* SUCCESS */
    }

    if (encOutputInfo.reconFrameIndex == RECON_IDX_FLAG_CHANGE_PARAM) {
        VLOG(TRACE, "CHANGE PARAMETER!\n");
        return TRUE; /* Try again */
    }
    else {
        DisplayEncodedInformation(ctx->handle, ctx->encOpenParam.bitstreamFormat, ctx->frameIdx, &encOutputInfo, encParam->srcEndFlag, encParam->srcIdx);
    }

    if ( encOutputInfo.warnInfo & WAVE5_ETCWARN_FORCED_SPLIT_BY_CU8X8 )
        VLOG(TRACE, "WAVE5_ETCWARN_FORCED_SPLIT_BY_CU8X8\n");

    if (result != RETCODE_SUCCESS )
        return FALSE;

    for (i = 0; i < ctx->fbCount.srcFbNum; i++) {
        if ( (encOutputInfo.releaseSrcFlag >> i) & 0x01) {
            ctx->encodedSrcFrmIdxArr[i] = 1;
        }
    }

    ctx->fullInterrupt      = FALSE;

    if ( outbuf ) {
        ctx->outSize  = encOutputInfo.bitstreamSize;
    }

    /* Finished encoding ... */
    if (encOutputInfo.reconFrameIndex == RECON_IDX_FLAG_ENC_END) {
        ctx->last          = TRUE;  /* Send finish signal */
        ctx->stateDoing    = FALSE;
        ctx->terminate     = TRUE;
    }

    return TRUE;
}

static BOOL OpenEncoder(EncoderContext_t *ctx)
{
    SecAxiUse               secAxiUse;
    MirrorDirection         mirrorDirection;
    RetCode                 result;

    ctx->stateDoing = TRUE;

    ctx->encOpenParam.bitstreamBuffer     = ctx->bsBuf.bs[0].phys_addr;
    ctx->encOpenParam.bitstreamBufferSize = ctx->bsBuf.bs[0].size;

    if ((result = VPU_EncOpen(&ctx->handle, &ctx->encOpenParam)) != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_EncOpen failed Error code is 0x%x \n", result);
        return FALSE;
    }
    /* VPU_EncGiveCommand(ctx->handle, ENABLE_LOGGING, 0); */

    if (ctx->rotAngle != 0 || ctx->mirDir != 0) {
        VPU_EncGiveCommand(ctx->handle, ENABLE_ROTATION, 0);
        VPU_EncGiveCommand(ctx->handle, ENABLE_MIRRORING, 0);
        VPU_EncGiveCommand(ctx->handle, SET_ROTATION_ANGLE, &ctx->rotAngle);
        mirrorDirection = (MirrorDirection)ctx->mirDir;
        VPU_EncGiveCommand(ctx->handle, SET_MIRROR_DIRECTION, &mirrorDirection);
    }

    osal_memset(&secAxiUse,   0x00, sizeof(SecAxiUse));
    //secAxiUse.useEncRdoEnable = (ctx->testEncConfig.secondaryAXI & 0x1) ? TRUE : FALSE;  //USE_RDO_INTERNAL_BUF
    //secAxiUse.useEncLfEnable  = (ctx->testEncConfig.secondaryAXI & 0x2) ? TRUE : FALSE;  //USE_LF_INTERNAL_BUF
    secAxiUse.useEncRdoEnable = FALSE;  //USE_RDO_INTERNAL_BUF
    secAxiUse.useEncLfEnable  = FALSE;  //USE_LF_INTERNAL_BUF

    VPU_EncGiveCommand(ctx->handle, SET_SEC_AXI, &secAxiUse);
    VPU_EncGiveCommand(ctx->handle, SET_CYCLE_PER_TICK,   (void*)&ctx->cyclePerTick);

    ctx->subFrameSyncCfg.subFrameSyncOn = ctx->encOpenParam.subFrameSyncEnable;
    ctx->stateDoing = FALSE;

    return TRUE;
}



/******************************** Modified for resmgr ***************************************/

static inline vpu_buffer_t *tivpu_get_vpu_buf(vpu_buffer_t *vbufs, int nbuf, void *buf_hdr)
{
    for(int i = 0; i < nbuf; i++) {
        if(vbufs[i].usr_info.priv == buf_hdr)
            return &(vbufs[i]);
    }
    return NULL;
}

int32_t tivpu_enc_start(tivpu_context_t *vpu_ctx)
{
    EncoderContext_t *ctx = NULL;
    if((!vpu_ctx) || (!vpu_ctx->codec_ctx))
        return -1;

    ctx = vpu_ctx->codec_ctx;

    ctx->first = TRUE;

    return 0;
}

int32_t tivpu_enc_stop(tivpu_context_t *vpu_ctx)
{
    EncoderContext_t *ctx = NULL;

    if((!vpu_ctx) || (!vpu_ctx->codec_ctx))
        return -1;

    ctx = vpu_ctx->codec_ctx;

    ctx->last = TRUE;

    return 0;
}

int32_t tivpu_enc_process(tivpu_context_t *vpu_ctx, void *in_buf, void *out_buf, void *op_first_buf, uint8_t is_eos,
                            codec_dbg_info_t *info)
{
    EncoderContext_t *ctx = NULL;
    vpu_buffer_t *i_buf, *o_buf, *o_first_buf = NULL;
    int32_t err = EOK;

    if(!vpu_ctx) {
        return -1;
    }

    ctx = vpu_ctx->codec_ctx;

    i_buf = tivpu_get_vpu_buf(vpu_ctx->input_bufs, vpu_ctx->input_buf_num, in_buf);
    //dump_buf_to_file("in", i_buf->virt_addr, i_buf->size, info);

    if(ctx->first == TRUE) {
        o_first_buf = tivpu_get_vpu_buf(vpu_ctx->output_bufs, vpu_ctx->output_buf_num, (void *)op_first_buf);
    }

    o_buf = tivpu_get_vpu_buf(vpu_ctx->output_bufs, vpu_ctx->output_buf_num, (void *)out_buf);

    if(TRUE != tivpu_execute_yuv_feeder(ctx, i_buf)) {
        VLOG(ERR, "%s:%d execute tivpu_execute_yuv_feeder FAILED:\n", __FUNCTION__, __LINE__);
        err = -1;
    }

    if(ctx->first == TRUE) {
        if(TRUE != tivpu_execute_encoder(ctx, i_buf, o_buf, o_first_buf)) {
            VLOG(ERR, "%s:%d tivpu_execute_encoder FAILED:\n", __FUNCTION__, __LINE__);
            err = -1;
        }
    } else {
        if(TRUE != tivpu_execute_encoder(ctx, i_buf, o_buf, NULL)) {
            VLOG(ERR, "%s:%d tivpu_execute_encoder FAILED:\n", __FUNCTION__, __LINE__);
            err = -1;
        }
    }

    //print_buf_info(NULL, o_buf);
    //dump_buf_to_file("out", o_buf->virt_addr, ctx->outSize, info);
    return err;
}

BOOL tivpu_execute_encoder(EncoderContext_t* ctx, vpu_buffer_t* inbuf, vpu_buffer_t* outbuf, vpu_buffer_t* outbuf_first)
{
    BOOL ret;
    switch (ctx->state) {
    case ENCODER_STATE_OPEN:
        ret = OpenEncoder(ctx);
        if (ctx->stateDoing == FALSE) ctx->state = ENCODER_STATE_INIT_SEQ;
        break;
    case ENCODER_STATE_INIT_SEQ:
        ret = SetSequenceInfo(ctx);
        if (ctx->stateDoing == FALSE) ctx->state = ENCODER_STATE_REGISTER_FB;
        break;
    case ENCODER_STATE_REGISTER_FB:
        ret = RegisterFrameBuffers(ctx);
        if (ctx->stateDoing == FALSE) ctx->state = ENCODER_STATE_ENCODING;
        break;
    case ENCODER_STATE_ENCODING:
        ret = Encode(ctx, inbuf, outbuf, outbuf_first);
        break;
    default:
        ret = FALSE;
        break;
    }

    if (ret == FALSE || ctx->terminate == TRUE) {
        ctx->last  = TRUE;
    }
    return ret;
}

BOOL tivpu_prepare_encoder(EncoderContext_t* ctx)
{
    Uint32 i;
    Uint32 num = ctx->encOpenParam.streamBufCount;

    for (i=0; i < num; i++) {
        if ( i < ctx->bsBuf.num) {
            ctx->bsBuffer[i] = &ctx->bsBuf.bs[i];
        }
        else {
            VLOG(ERR, "%s:%d number of bsBuffer's requested is more than bsBuf.num !!\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
    }

    return TRUE;
}

BOOL tivpu_destroy_encoder(EncoderContext_t* ctx)
{
    BOOL            success = TRUE;
    ENC_INT_STATUS  intStatus;
    Uint64          currentTime = 0;
    Uint32          timeout = ENC_DESTROY_TIME_OUT;
    Uint32          max_iteration = 100;

    if ( NULL == ctx )
        return FALSE;
    if (FALSE == ctx->stateDoing) {
        if (ctx && ctx->handle) {
            ctx->desStTimeout = osal_gettime();
            ctx->stateDoing = TRUE;
        }
    }

    while (VPU_EncClose(ctx->handle) == RETCODE_VPU_STILL_RUNNING) {
        if ((intStatus = HandlingInterruptFlag(ctx)) == ENC_INT_STATUS_TIMEOUT) {
            HandleEncoderError(ctx->handle, ctx->frameIdx, NULL);
            VLOG(ERR, "NO RESPONSE FROM VPU_EncClose2()\n");
            ctx->stateDoing = FALSE;
            success = FALSE;
            break;
        }
        else if (intStatus == ENC_INT_STATUS_DONE) {
            EncOutputInfo   outputInfo;
            VLOG(INFO, "VPU_EncClose() : CLEAR REMAIN INTERRUPT\n");
            VPU_EncGetOutputInfo(ctx->handle, &outputInfo);
            continue;
        }

        currentTime = osal_gettime();
        if ( (currentTime - ctx->desStTimeout) > timeout) {
            VLOG(ERR, "\n INSNTANCE #%d VPU Close TIMEOUT.\n", ctx->handle->instIndex);
            ctx->stateDoing = FALSE;
            success = FALSE;
            break;
        } else {
            if (max_iteration > ctx->iterationCnt) {
                VLOG(INFO, "Iteration Count in DestroyEncoder(): %d \n", ctx->iterationCnt);
                ctx->iterationCnt++;
                return TRUE;
            }
        }
        osal_msleep(10);
    }

    ctx->stateDoing = FALSE;

    osal_free(ctx);

    return success;
}



/****** DEBUG***********/
/**** MOVE THIS TO main_helper later ************/
//#define DEBUG

#if defined DEBUG
#define codec_dbg(...)    slogf(_SLOG_SETCODE(_SLOGC_PRIVATE_START, 55), _SLOG_INFO, __VA_ARGS__)
#else
#define codec_dbg(...)
#endif

static BOOL log_more = TRUE;

int32_t dump_buf_to_file(const char * dir, void *buf, size_t buf_len, codec_dbg_info_t *info)
{

    char filename[48] = {0};
    pid_t pid   = info->pid;
    int32_t cnt = info->cnt;

    snprintf(filename, 48, "/tmp/%d-%d.%s.tmp", cnt, pid, dir);

    codec_dbg("File name is %s", filename);

    if (log_more && (!access(filename, R_OK))) {
        log_more = FALSE;
        codec_dbg("file %s exists", filename);
        codec_dbg("stop logging");
        return EOK;
    }

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        codec_dbg("Failed to open file: %s", filename);
        return -1;
    }
    fwrite(buf, 1, buf_len, fp);
    //fflush(fp);
    fclose(fp);

    return  EOK;
}


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
