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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Video.h>
#include <OMX_Extension_video_TI.h>

/* #include <vpu_enc-cli.h> */

#include "log.h"
#include "omxil_enc_interface.h"
#include "tivpu_enc.h"
#include "vpu_usr.h"
#include "tivpu_codec.h"
#include "ti/shmemallocator/SharedMemoryAllocatorUsr.h"

#define ALIGN8(X)  (((X)+7) &~7)


static const char *const ENC_COMPONENT_ROLES[] = {
    "video_encoder.avc",
    "video_encoder.hevc",
};

static uint32_t SIZE_OF_ROLES = sizeof(ENC_COMPONENT_ROLES) / sizeof(ENC_COMPONENT_ROLES[0]);

typedef struct codec_handle
{
    void                  *tivpu_hdl;
    vpu_buffer_t          *input_bufs;
    vpu_buffer_t          *output_bufs;
    int                   input_buf_num;
    int                   output_buf_num;
    omxil_encode_callback callback;
    void*                 cb_ctx;
} codec_t;


static BOOL omxil_vpu_enc_config_update(encoder_config* pEncConfig, tivpu_enc_config_t *vpu_enc_cfg)
{
    if (pEncConfig->oFormat == OMX_VIDEO_CodingAVC) {
        vpu_enc_cfg->bitFormat = (int) STD_AVC;
    }
    else if (pEncConfig->oFormat == (OMX_VIDEO_CODINGTYPE)OMXQ_VIDEO_CodingHEVC) {
        vpu_enc_cfg->bitFormat = (int) STD_HEVC;
    }
    else {
        LOG(LOG_ERROR, "Invalid codec standard mode: 0x%x \n", pEncConfig->oFormat);
        return FALSE;
    }

    if (pEncConfig->iFormat == OMX_COLOR_FormatYUV420Planar) {
        vpu_enc_cfg->cbcrInterleave = 0;
    }
    else {
        vpu_enc_cfg->cbcrInterleave = 1; /* default here is to use NV12 */
    }

    if (pEncConfig->iFormat == OMX_COLOR_FormatYCbYCr) {
        vpu_enc_cfg->packedFormat = (int) PACKED_YUYV;
    }
    else if (pEncConfig->iFormat == OMX_COLOR_FormatYCrYCb) {
        vpu_enc_cfg->packedFormat = (int) PACKED_YVYU;
    }
    else if (pEncConfig->iFormat == OMX_COLOR_FormatCbYCrY) {
        vpu_enc_cfg->packedFormat = (int) PACKED_UYVY;
    }
    else if (pEncConfig->iFormat == OMX_COLOR_FormatCrYCbY) {
        vpu_enc_cfg->packedFormat = (int) PACKED_VYUY;
    }
    else {
        vpu_enc_cfg->packedFormat = 0;
    }

    vpu_enc_cfg->width              = pEncConfig->width;
    vpu_enc_cfg->height             = pEncConfig->height;
    vpu_enc_cfg->stride             = pEncConfig->stride;
    vpu_enc_cfg->profile            = pEncConfig->profile;
    vpu_enc_cfg->level              = pEncConfig->level;
    vpu_enc_cfg->framerate          = pEncConfig->framerate;
    vpu_enc_cfg->keyFrameInterval   = pEncConfig->keyFrameInterval;
    vpu_enc_cfg->qpI                = pEncConfig->qpI;
    vpu_enc_cfg->qpP                = pEncConfig->qpP;
    vpu_enc_cfg->bitrate            = pEncConfig->bitrate;
    vpu_enc_cfg->rateControl        = pEncConfig->rateControl;
    vpu_enc_cfg->arithmeticEncoding = pEncConfig->arithmeticEncoding;
    vpu_enc_cfg->sliceType          = pEncConfig->sliceType;
    vpu_enc_cfg->sliceSize          = pEncConfig->sliceSize;
    vpu_enc_cfg->coreIdx            = pEncConfig->coreIdx;
    vpu_enc_cfg->sourceBufCount     = pEncConfig->sourceBufCount;
    vpu_enc_cfg->streamBufCount     = pEncConfig->streamBufCount;
    vpu_enc_cfg->streamBufSize      = pEncConfig->streamBufSize;
    vpu_enc_cfg->setLossless        = pEncConfig->setLossless;
    vpu_enc_cfg->setGOP             = pEncConfig->setGOP;
    vpu_enc_cfg->coreIdx = pEncConfig->coreIdx;

    return TRUE;
}

void omxil_get_roles(const char** roles, OMX_U32 nSize)
{
    uint32_t i, rolesize;

    rolesize = SIZE_OF_ROLES > nSize ? nSize : SIZE_OF_ROLES;
    for(i = 0; i < rolesize; i++)
        roles[i] = ENC_COMPONENT_ROLES[i];

    return;
}

OMX_ERRORTYPE omxil_comp_role_enum(OMX_U8 *cRole, OMX_U32 nIndex)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if ((NULL != cRole) && (nIndex < SIZE_OF_ROLES))
    {
        strcpy((char*)cRole, ENC_COMPONENT_ROLES[nIndex]);
    }
    else
    {
        eError = OMX_ErrorBadParameter;
    }

    return eError;
}

#if VPU_FEATURE_OOB
/* TODO: Need to implement this  */
static void buf_done(vpu_buffer_t *buf, mm_enc_process_cb type, void *ctx)
{
    codec_t *hdl = ctx;
    if(type == SRC_FRAME_RELEASE) {
        LOG(LOG_DEBUG2, "Returned input buffer->size(%p->%d)", buf, buf->size[0]);
        hdl->callback(hdl->cb_ctx, buf->priv, QOMX_EMPTY_BUFFER_DONE);
    }
    else if(type == CODED_BUFF_READY) {
        LOG(LOG_DEBUG2, "Encoded frame with size(%p->%d)", buf, buf->size[0]);
        OMX_BUFFERHEADERTYPE *pBufHdr = buf->priv;
        pBufHdr->nFilledLen = buf->size[0];
        hdl->callback(hdl->cb_ctx, pBufHdr, QOMX_FILL_BUFFER_DONE);
    }
    else if(type == ENC_STR_END) {
        LOG(LOG_INFO, "%s:%d EOS received", __func__, __LINE__);
        hdl->callback(hdl->cb_ctx, NULL, QOMX_EOS);
    }
    else if(type == ENC_ERROR_FATAL) {
        LOG(LOG_ERROR, "%s:%d Error received", __func__, __LINE__);
        hdl->callback(hdl->cb_ctx, NULL, QOMX_ERROR);
    }
    else {
        LOG(LOG_ERROR, "%s:%d unknown cb type %d", __func__, __LINE__, type);
    }
}
#endif

static int cleanup_buffers(codec_t *hdl)
{
    if(!hdl) {
        return -1; // TODO:: fix error type here
    }
    /* free all vpu_buffer_t memory for input and output */
    free(hdl->input_bufs);
    free(hdl->output_bufs);
    hdl->input_bufs = hdl->output_bufs = NULL;
    hdl->output_buf_num = 0;

    return EOK;
}

void *omxil_create_encoder(omxil_encode_callback cb, void *cb_ctx)
{
    codec_t *hdl = NULL;

    if( (hdl = calloc(1,sizeof(*hdl))) == NULL ) {
        LOG(LOG_ERROR,"Couldn't allocate %zubytes",sizeof(*hdl));
        return NULL;
    }

    /*TODO::  Here it calls codec_open . Need to have a enum to pass to it for encoder and encoder  */
    if((hdl->tivpu_hdl = vpu_codec_open()) == NULL) {
        LOG(LOG_ERROR, "vpu_enc_open() failed");
        free(hdl);
        return NULL;
    }

    hdl->callback = cb;
    hdl->cb_ctx = cb_ctx;

    return hdl;
}

/*TODO: Make sure the nBuffer and uBufferSize are filled in properly. The current
 *      implementation seems to do the reverse though */
OMX_ERRORTYPE omxil_query_buf_info(void *hdl, OMX_U32 *nBuffer, OMX_U32 *uBufferSize)
{
    codec_t *ehdl = (codec_t*)hdl;

    if (*nBuffer == 0)
        return OMX_ErrorHardware;

    ehdl->output_buf_num = *nBuffer;

    if(vpu_enc_get_buf_info(ehdl->tivpu_hdl, nBuffer, uBufferSize) != 0) {
        LOG(LOG_ERROR, "%s: vpu_enc_get_buf_info returns error", __func__);
        return OMX_ErrorHardware;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omxil_validate_inport_parameters(void *hdl, OMX_PARAM_PORTDEFINITIONTYPE *pPortDef)
{
    if((int)pPortDef->format.video.eColorFormat != OMX_COLOR_FormatYUV420SemiPlanar) {
        LOG(LOG_ERROR, "%s: unsupported color format(%x)", __func__, pPortDef->format.video.eColorFormat);
        return OMX_ErrorBadParameter;
    }

    if(pPortDef->format.video.nFrameWidth == 0 ||
            pPortDef->format.video.nFrameHeight == 0 ||
            pPortDef->format.video.nStride == 0) {
        LOG(LOG_ERROR, "%s: Invalid stride(%d) or resolution(%d, %d)", __func__, pPortDef->format.video.nStride,
                pPortDef->format.video.nFrameWidth, pPortDef->format.video.nFrameHeight);
        return OMX_ErrorBadParameter;
    }
    if((pPortDef->format.video.nStride % 32) != 0) {
        LOG(LOG_ERROR, "%s: Invalid stride(%d), stride needs to be 32 byte aligned",
                __func__, pPortDef->format.video.nStride);
        return OMX_ErrorBadParameter;
    }
    if((pPortDef->format.video.nFrameHeight % 8) != 0) {
        LOG(LOG_INFO, "%s: Please make sure buffer height is 8 byte aligned", __func__);
    }

    pPortDef->nBufferSize = pPortDef->format.video.nStride * ALIGN8(pPortDef->format.video.nFrameHeight) * 3 / 2;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE omxil_validate_outport_parameters(void *hdl, OMX_PARAM_PORTDEFINITIONTYPE *pPortDef)
{
    if(pPortDef->format.video.eCompressionFormat != OMX_VIDEO_CodingAVC &&
        pPortDef->format.video.eCompressionFormat != (OMX_VIDEO_CODINGTYPE)OMXQ_VIDEO_CodingHEVC) {
        LOG(LOG_ERROR, "%s: unsupported compression format(%x)", __func__, pPortDef->format.video.eCompressionFormat);
        return OMX_ErrorBadParameter;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omxil_enc_config(void *hdl, OMX_PARAM_PORTDEFINITIONTYPE *pPortDefIn, OMX_PARAM_PORTDEFINITIONTYPE *pPortDefOut, encoder_config *config)
{
    codec_t      *ehdl = (codec_t *)hdl;
    tivpu_enc_config_t vpu_cfg = {0};

    if (config->coreIdx < 0 || config->coreIdx >= MAX_NUM_VPU_CORE) {
        LOG(LOG_ERROR, "%s:%d VPU index of %d is not supported\n", __FUNCTION__, __LINE__, config->coreIdx);
        return OMX_ErrorBadParameter;
    }

    config->sourceBufCount = pPortDefIn->nBufferCountActual;
    config->streamBufCount = pPortDefOut->nBufferCountActual;
    config->streamBufSize = pPortDefOut->nBufferSize;

    if (omxil_vpu_enc_config_update(config, &vpu_cfg) == FALSE) {
        LOG(LOG_ERROR, "omxil_vpu_enc_config_update error\n");
        free(ehdl);
        return OMX_ErrorBadParameter;
    }

    vpu_enc_init(ehdl->tivpu_hdl, &vpu_cfg);

    config->streamBufSize = vpu_cfg.streamBufSize;


    //For the VPU_FEATURE_OOB, Need to register buf_done here.

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omxil_close_encoder(void *hdl)
{
    codec_t *ehdl = (codec_t*)hdl;

    cleanup_buffers(ehdl);
    printf("%s: %d\n",__func__, __LINE__);

    vpu_enc_deinit(ehdl->tivpu_hdl);

    free(ehdl);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE omxil_enc_start(void *hdl)
{
    codec_t *ehdl = (codec_t*)hdl;

    vpu_enc_start_streaming(ehdl->tivpu_hdl);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omxil_enc_stop(void *hdl)
{
    codec_t *ehdl = (codec_t*)hdl;

    vpu_enc_stop_streaming(ehdl->tivpu_hdl);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omxil_enc_register_buffers(void *hdl, BufferList *bufList)
{
    codec_t *ehdl = (codec_t *)hdl;
    vpu_buffer_t *bufs = NULL;
    int nbuf = 0, i = 0;
    vpu_buf_dir buf_dir = TIVPU_BUFDIR_OUTPUT;

    if (!ehdl || !bufList)
        return OMX_ErrorUndefined;

    /* store the number of bufs to register */
    nbuf = bufList->nAllocSize;

    /* Allocate the vup_buffer containers */
    bufs = (vpu_buffer_t *)malloc(sizeof(vpu_buffer_t) * nbuf);

    if(bufs == NULL) {
        LOG(LOG_ERROR, "OMXIL omxil_enc_register_buffer error no memory to allocate i/o buffers");
        return OMX_ErrorInsufficientResources;
    }

    /* The o/p buffers should be registered first. See component notes */
    if(bufList->eDir == OMX_DirOutput) {

        /* Update the handle with buffer count */
        ehdl->output_bufs = bufs;
        ehdl->output_buf_num = nbuf;
        buf_dir = TIVPU_BUFDIR_OUTPUT;
    } else { /* (bufList->eDir == OMX_DirInput) */

        ehdl->input_buf_num = nbuf;
        ehdl->input_bufs = bufs;
        buf_dir = TIVPU_BUFDIR_INPUT;
    }

    /* Update the buffers info with the phys_addr */
    for (i = 0; i < nbuf; i++) {
        if (bufList->bAllocated == OMX_FALSE) { /* Handle application allocated buffers here */
            off64_t offset;
            if (mem_offset64(bufList->pAllocHdr[i]->pBuffer, NOFD, 1, &offset, NULL) == -1)
            {
                LOG(LOG_ERROR, "%s:%d Failed to get physical address ", __func__, __LINE__);
                cleanup_buffers(ehdl);
                return OMX_ErrorInsufficientResources;
            }
            bufs[i].phys_addr = (PhysicalAddress)offset;
        } else { /* Handle the component buffers here */
            shm_buf **p_buf = (shm_buf **)bufList->pShmBufs;
            bufs[i].phys_addr = (PhysicalAddress)(p_buf[i]->phy_addr);
        }
        
        bufs[i].usr_info.usr_addr = (long unsigned int)bufList->pAllocHdr[i]->pBuffer;
        bufs[i].size = bufList->pAllocHdr[i]->nAllocLen;
        bufs[i].usr_info.priv = bufList->pAllocHdr[i];
    }

    /* Now that the buffers are updated, lets hand it over to the resmgr */
    if (0 != vpu_enc_buf_prepare(ehdl->tivpu_hdl, bufs, nbuf, buf_dir)) {
        LOG(LOG_ERROR, "%s: Error returned from vpu_enc_buf_prepare", __func__);
        return OMX_ErrorHardware;
    }

    return OMX_ErrorNone;
}


OMX_ERRORTYPE omxil_enc_encodeFrame(void *hdl, OMX_BUFFERHEADERTYPE *input, OMX_BUFFERHEADERTYPE *output, OMX_BUFFERHEADERTYPE *output_first)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    codec_t *ehdl = (codec_t *)hdl;
    BOOL is_eos = FALSE;
    vpu_enc_status_t enc_status = {0};
    int32_t status = EOK;

    if(input->nFilledLen > 0) {
        if(input->nFlags & OMX_BUFFERFLAG_EOS) {
            is_eos = TRUE;
        }
        /* if this is the first frame, bot output and output_first will have its nFilledLen updated
         * else only the output nFilledLen is updated. is_first will indicate if it is the first frame.
        */
        status = vpu_encode_frame(ehdl->tivpu_hdl, (unsigned long)input, (unsigned long)output, (unsigned long)output_first, is_eos, &enc_status);

        if(status != EOK) {
            err = OMX_ErrorHardware;
        }
        if(output_first && enc_status.is_first) {
            output_first->nFilledLen = enc_status.hdr_size;
            output->nFilledLen = enc_status.out_size;
            ehdl->callback(ehdl->cb_ctx, output_first, QOMX_FILL_BUFFER_DONE);
        } else {
            output->nFilledLen = enc_status.out_size;
        }
    }

    /* return the buffer header */
    ehdl->callback(ehdl->cb_ctx, input, QOMX_EMPTY_BUFFER_DONE);
    ehdl->callback(ehdl->cb_ctx, output, QOMX_FILL_BUFFER_DONE);

    if(input->nFlags & OMX_BUFFERFLAG_EOS) {
        LOG(LOG_DEBUG1, "%s:%d EOS received", __func__, __LINE__);
        ehdl->callback(ehdl->cb_ctx, NULL, QOMX_EOS);
    }

    return err;
}

OMX_ERRORTYPE omxil_enc_forceKeyFrame(void *hdl)
{
    //TODO
    LOG(LOG_INFO, "OMXIL J7 enc comp:%s:%d Not support yet",__func__, __LINE__);
    return OMX_ErrorNone;
}

