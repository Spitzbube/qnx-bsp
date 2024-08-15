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
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Video.h>
#include <OMX_Extension_video_TI.h>


#include "log.h"
#include "omxil_dec_interface.h"
#include "vpu_usr.h"
#include "ti/shmemallocator/SharedMemoryAllocatorUsr.h"

static const char *const DEC_COMPONENT_ROLES[] = {
    "video_decoder.avc",
    "video_decoder.hevc",
};
static uint32_t SIZE_OF_ROLES = sizeof(DEC_COMPONENT_ROLES) / sizeof(DEC_COMPONENT_ROLES[0]);

static uint32_t outputBufferCount;

void omxil_get_roles(const char** roles, OMX_U32 nSize)
{
    uint32_t i, rolesize;

    rolesize = SIZE_OF_ROLES > nSize ? nSize : SIZE_OF_ROLES;
    for(i = 0; i < rolesize; i++)
        roles[i] = DEC_COMPONENT_ROLES[i];

    return;
}

OMX_ERRORTYPE omxil_comp_role_enum(OMX_U8 *cRole, OMX_U32 nIndex)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if ((NULL != cRole) && (nIndex < SIZE_OF_ROLES))
    {
        strcpy((char*)cRole, DEC_COMPONENT_ROLES[nIndex]);
    }
    else
    {
        eError = OMX_ErrorBadParameter;
    }

    return eError;
}


#if defined (DEBUG_MODE)
void dump_dec_status( vpu_dec_status_t *ds)
{
    slogf(_SLOGC_MEDIA, _SLOG_INFO, "DS:: is_first = %d", ds->is_first);
    slogf(_SLOGC_MEDIA, _SLOG_INFO, "DS:: vpu_return_buf = %lx", ds->vpu_return_buf);
    slogf(_SLOGC_MEDIA, _SLOG_INFO, "DS:: o_filled_len = %d", ds->o_filled_len);
    slogf(_SLOGC_MEDIA, _SLOG_INFO, "DS:: displayed_frames = %d", ds->displayed_frames);
    slogf(_SLOGC_MEDIA, _SLOG_INFO, "DS:: decoded_frames = %d", ds->decoded_frames);
    slogf(_SLOGC_MEDIA, _SLOG_INFO, "DS:: terminate = %d", ds->terminate);
}
#else
#define dump_dec_status(x)
#endif


/* These buffer were sent post QOMX_EOS. Release those */
static void tivpu_dec_release_buffer(codec_t *hdl, void *buf_hdr)
{
#if defined (DEBUG_MODE)
    slogf(_SLOGC_MEDIA, _SLOG_INFO, "%s:%d Releasing output (%p)", __func__, __LINE__, buf_hdr);
#endif
    LOG(LOG_INFO, "Releasing the output frames");
    hdl->callback(hdl->cb_ctx, buf_hdr, QOMX_RELEASE_OUTPUT_BUFFER);
}

static void buf_done(codec_t *hdl, vpu_buffer_t *obufs, vpu_dec_status_t *dec_status)
{
    OMX_BUFFERHEADERTYPE *pBufHdr   = NULL;
    uint32_t i, index;

    if(!hdl) {
        LOG(LOG_ERROR, "Invalid Handle");
        return;
    }

    if(dec_status->terminate) {
#if defined (DEBUG_MODE)
        slogf(_SLOGC_MEDIA, _SLOG_INFO, "Decoder (buf_done) => OMX callback for QOMX_EOS done here");
#endif
        hdl->callback(hdl->cb_ctx, NULL, QOMX_EOS);
    }
    else {
        hdl->output_buf_cnt++;
#if defined (DEBUG_MODE)
        slogf(_SLOGC_MEDIA, _SLOG_INFO, "Decoder (buf_done) => output_buf_cnt (%d)", hdl->output_buf_cnt);
#endif
    }

#if defined (DEBUG_MODE)
        slogf(_SLOGC_MEDIA, _SLOG_INFO, "Decoder (buf_done) => DecodedFrame idx (%d)", dec_status->decoded_frames);
        slogf(_SLOGC_MEDIA, _SLOG_INFO, "Decoder (buf_done) => DisplayFrame idx (%d)", dec_status->displayed_frames);
#endif

    if(dec_status->displayed_frames >= 0) {
        hdl->priorDispIdx = hdl->currDispIdx; /* update prior Display index with OLD value of currDispIdx */
        hdl->currDispIdx  = dec_status->displayed_frames; /* refresh currDispIdx with latest value */

        /* Only give back display buffers after a lag of 1 */
        if(hdl->frameDisplayed > 0) {
            LOG(LOG_DEBUG2, "%s:%d Display frame (%p->%d)", __func__, __LINE__,
                 obufs[hdl->priorDispIdx].usr_info.priv, obufs[hdl->priorDispIdx].size);
            hdl->output_frame_cnt++;
#if defined (DEBUG_MODE)
            slogf(_SLOGC_MEDIA, _SLOG_INFO, "Decoder (buf_done) => Total Displayed Frames (%d)", hdl->output_frame_cnt);
            slogf(_SLOGC_MEDIA, _SLOG_INFO, "Decoder (buf_done) => OMX FillBufferDone callback with lag of 1 for buffer num (%d)", hdl->priorDispIdx);
#endif
            pBufHdr = obufs[hdl->priorDispIdx].usr_info.priv;
            pBufHdr->nFilledLen = obufs[hdl->priorDispIdx].size;
            hdl->callback(hdl->cb_ctx, pBufHdr, QOMX_FILL_BUFFER_DONE);
        }
        else {
            hdl->frameDisplayed = 1;
        }
    }
    else if (dec_status->displayed_frames == (-1)) {
        /* FillBufferDone callback of OLD "currDispIdx" */
#if defined (DEBUG_MODE)
        slogf(_SLOGC_MEDIA, _SLOG_INFO, "%s:%d Display frame (-1) AFTER DONE (%p->0)", __func__, __LINE__, obufs[hdl->currDispIdx].usr_info.priv);
        slogf(_SLOGC_MEDIA, _SLOG_INFO, "Decoder (buf_done) => OMX FillBufferDone callback for OLD currDispIdx buffer num (%d)", hdl->currDispIdx);
#endif
        pBufHdr = obufs[hdl->currDispIdx].usr_info.priv;
        pBufHdr->nFilledLen = obufs[hdl->currDispIdx].size;
        pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
        hdl->callback(hdl->cb_ctx, pBufHdr, QOMX_FILL_BUFFER_DONE);

        /* Give back all other remaining buffers */
        for(i = 0; i < (hdl->output_buf_num-1); i++) {
            index = ((hdl->output_buf_num + hdl->currDispIdx - 1) - i) % hdl->output_buf_num;
#if defined (DEBUG_MODE)
            slogf(_SLOGC_MEDIA, _SLOG_INFO, "Decoder (buf_done) => OMX FillBufferDone callback for remaining buffers buffer num (%d)", index);
#endif
            pBufHdr = obufs[index].usr_info.priv;
            pBufHdr->nFilledLen = 0;
            hdl->callback(hdl->cb_ctx, pBufHdr, QOMX_FILL_BUFFER_DONE);
        }
    }
    else {
        ; /* Do nothing in this case. We don't return buffers that aren't displayable yet */
    }
}

void *omxil_create_decoder(omxil_decode_callback cb, void *cb_ctx)
{
    codec_t *hdl = NULL;

    if( (hdl = calloc(1,sizeof(*hdl))) == NULL ) {
        LOG(LOG_ERROR,"Couldn't allocate %zubytes",sizeof(*hdl));
        return NULL;
    }

    if((hdl->hdl = vpu_codec_open()) == NULL) {
        LOG(LOG_ERROR, "Error: vpu_codec_open failed");
        free(hdl);
        return NULL;
    }

    hdl->callback = cb;
    hdl->cb_ctx = cb_ctx;

    return (void *)hdl;
}

OMX_ERRORTYPE omxil_validate_outport_parameters(void *hdl, OMX_PARAM_PORTDEFINITIONTYPE *pPortDef)
{
    codec_t *c_hdl = (codec_t *)hdl;

    if(!c_hdl)
        return OMX_ErrorBadParameter;


    if((int)pPortDef->format.video.eColorFormat != OMX_COLOR_FormatYUV420SemiPlanar &&
    (int)pPortDef->format.video.eColorFormat != (OMX_COLOR_FORMATTYPE)OMXQ_COLOR_FormatNV16) {
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

    if(pPortDef->nBufferCountActual < pPortDef->nBufferCountMin) {
        LOG(LOG_ERROR, "%s: Invalid output buffer count (%d)", __func__, pPortDef->nBufferCountActual);
        return OMX_ErrorBadParameter;
    }
    else {
        outputBufferCount = pPortDef->nBufferCountActual;
    }

    if((int)pPortDef->format.video.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
        pPortDef->nBufferSize = pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight * 3 / 2;
    else
        pPortDef->nBufferSize = pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight * 2;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omxil_validate_inport_parameters(void *hdl, OMX_PARAM_PORTDEFINITIONTYPE *pPortDef)
{
    codec_t *c_hdl = (codec_t *)hdl;

    if(!c_hdl)
        return OMX_ErrorBadParameter;

    if(pPortDef->format.video.eCompressionFormat != OMX_VIDEO_CodingAVC &&
        pPortDef->format.video.eCompressionFormat != (OMX_VIDEO_CODINGTYPE)OMXQ_VIDEO_CodingHEVC) {
        LOG(LOG_ERROR, "%s: unsupported compression format(%x)", __func__, pPortDef->format.video.eCompressionFormat);
        return OMX_ErrorBadParameter;
    }

    pPortDef->nBufferSize = VDEC_INPUT_BUF_SIZE; /* Note: this size should be: (STREAM_BUF_SIZE_HEVC / input_buf_num)  [tivpu_dec.h] */
    return OMX_ErrorNone;
}

static CodStd infmtxi2vpu(OMX_VIDEO_CODINGTYPE eCompressionFormat)
{
    CodStd fmt = STD_MAX;
    if(eCompressionFormat == OMX_VIDEO_CodingAVC) {
        fmt = STD_AVC;
    }
    else if(eCompressionFormat == (OMX_VIDEO_CODINGTYPE)OMXQ_VIDEO_CodingHEVC) {
        fmt = STD_HEVC;
    }
    else {
        LOG(LOG_ERROR, "%s: unsupported compression format(%x)", __func__, eCompressionFormat);
    }

    return fmt;
}

static FrameBufferFormat outfmtxi2vpu(OMX_COLOR_FORMATTYPE eColorFormat)
{
    FrameBufferFormat fmt = FORMAT_420;
    if(eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
        fmt = FORMAT_420;
    }
    else if(eColorFormat == (OMX_COLOR_FORMATTYPE)OMXQ_COLOR_FormatNV16) {
        fmt = FORMAT_422;
    }
    else {
        LOG(LOG_ERROR, "%s: unsupported color format(%x)", __func__, eColorFormat);
    }

    return fmt;

}

OMX_ERRORTYPE omxil_init_decoder(void *hdl, OMX_VIDEO_PORTDEFINITIONTYPE *pInPortFormat, OMX_VIDEO_PORTDEFINITIONTYPE *pOutPortFormat, uint32_t coreIdx, uint8_t error_conceal, uint8_t DecBufferCount)
{
    codec_t *c_hdl = (codec_t *)hdl;

    if(!c_hdl)
        return OMX_ErrorBadParameter;

    tivpu_dec_config_t vpu_dec_config = {0};

    if (coreIdx >= MAX_NUM_VPU_CORE) {
        LOG(LOG_ERROR, "%s:%d VPU index of %d is not supported\n", __FUNCTION__, __LINE__, coreIdx);
        return OMX_ErrorBadParameter;
    }

    vpu_dec_config.coreIdx    = coreIdx;
    vpu_dec_config.width      = pInPortFormat->nFrameWidth;
    vpu_dec_config.height     = pInPortFormat->nFrameHeight;
    vpu_dec_config.framerate  = (pInPortFormat->xFramerate >> 16);
    vpu_dec_config.bitFormat  = infmtxi2vpu(pInPortFormat->eCompressionFormat);
    vpu_dec_config.format     = outfmtxi2vpu(pOutPortFormat->eColorFormat);
    vpu_dec_config.numOutBufs = outputBufferCount;
    vpu_dec_config.numDecBufs = DecBufferCount;
    vpu_dec_config.error_conceal = error_conceal;

    if (pOutPortFormat->eColorFormat == OMX_COLOR_FormatYUV420Planar) {
        vpu_dec_config.cbcrInterleave = 0;
    }
    else {
        vpu_dec_config.cbcrInterleave = 1; /* default */
    }

    if(vpu_dec_init(c_hdl->hdl, &vpu_dec_config) != 0) {
        LOG(LOG_ERROR, "Init decoder failed");
        return OMX_ErrorHardware;
    }

    c_hdl->currDispIdx = 0;
    c_hdl->priorDispIdx = 0;
    c_hdl->frameDisplayed = 0;

    //TODO:: Needed for out of band messages.
    //vpu_dec_register_callback(c_hdl->hdl, buf_done, c_hdl);


    return OMX_ErrorNone;
}

void omxil_close_decoder(void *hdl)
{
    codec_t *c_hdl = (codec_t *)hdl;

    if(!c_hdl) {
        LOG(LOG_ERROR, "Passed invalid handle for %s", __func__);
        return;
    }

    vpu_dec_deinit(c_hdl->hdl);
    vpu_dec_close(c_hdl->hdl);

    free(c_hdl->input_bufs);
    free(c_hdl->output_bufs);
    free(c_hdl);
}

OMX_ERRORTYPE omxil_dec_start(void *hdl)
{
    codec_t *c_hdl = (codec_t *)hdl;

    if(!c_hdl)
        return OMX_ErrorBadParameter;

    vpu_dec_start_streaming(c_hdl->hdl);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE omxil_dec_stop(void *hdl)
{
    codec_t *c_hdl = (codec_t *)hdl;

    if(!c_hdl)
        return OMX_ErrorBadParameter;

    vpu_dec_stop_streaming(c_hdl->hdl);

    return OMX_ErrorNone;
}


#if defined (DEBUG_MODE)
static void dump_queue_info(codec_t *hdl, int mod_idx)
{
    int i = 0;

    slogf(_SLOGC_MEDIA, _SLOG_INFO, "r_idx = %d, w_idx = %d size = %d\n",
                               hdl->r_idx, hdl->w_idx, hdl->in_queue_size);
    for(i = 0; i < hdl->input_buf_num; i++)
        slogf(_SLOGC_MEDIA, _SLOG_INFO, "input_queue[%d] = %p", i, (void *)(hdl->input_queue[i]));
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


OMX_ERRORTYPE omxil_dec_register_buffers(void *hdl, BufferList *bufList)
{
    codec_t *c_hdl = (codec_t *)hdl;
    int i;
    vpu_buffer_t *bufs = NULL;
    int nbuf = 0;
    vpu_buf_dir buf_dir = TIVPU_BUFDIR_OUTPUT;

    if(!c_hdl || !bufList) {
        return OMX_ErrorUndefined;
    }

    /* Allocate the vup_buffer containers */
    nbuf = bufList->nAllocSize;
    bufs = (vpu_buffer_t *)malloc(sizeof(vpu_buffer_t) * nbuf);
    if(bufs == NULL) {
        LOG(LOG_ERROR, "OMXIL omxil_enc_register_buffer error no memory to allocate i/o buffers");
        return OMX_ErrorInsufficientResources;
    }

    if(bufList->eDir == OMX_DirInput) {
        buf_dir = TIVPU_BUFDIR_INPUT;
        c_hdl->input_bufs = bufs;
        c_hdl->input_buf_num = nbuf;
        /* build the buffer queue for unused input buffers */
        c_hdl->input_queue = calloc(nbuf, sizeof(OMX_BUFFERHEADERTYPE *));
    }
    else { /* (bufList->eDir == OMX_DirOutput) */
        buf_dir = TIVPU_BUFDIR_OUTPUT;
        c_hdl->output_buf_num = nbuf;
        c_hdl->output_bufs = bufs;
    }

    for(i = 0; i < nbuf; i++) {

        if (bufList->bAllocated == OMX_FALSE) { /* Handle application allocated buffers here */
            off64_t offset;
            if (mem_offset64(bufList->pAllocHdr[i]->pBuffer, NOFD, 1, &offset, NULL) == -1)
            {
                LOG(LOG_ERROR, "%s:%d Failed to get physical address ", __func__, __LINE__);
                cleanup_buffers(c_hdl);
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
    if (0 != vpu_dec_buf_prepare(c_hdl->hdl, bufs, nbuf, buf_dir)) {
        LOG(LOG_ERROR, "%s: Error returned from vxd_dec_buf_prepare for buf[%d]",__func__,i);
        return OMX_ErrorHardware;
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE attempt_decode(codec_t *hdl, OMX_BUFFERHEADERTYPE *input, OMX_BUFFERHEADERTYPE *output, vpu_dec_status_t *dec_status)
{
    OMX_ERRORTYPE omx_status = OMX_ErrorNone;
    int32_t status = EOK; 
    uint32_t in_filled_len = (input != NULL)?(input->nFilledLen):0;
    BOOL is_eos = (input)?(input->nFlags & OMX_BUFFERFLAG_EOS):0;

    if(hdl == NULL) {
        LOG(LOG_ERROR, "Invalid handle");
        return OMX_ErrorHardware;
    }

#if defined (DEBUG_MODE)
    slogf(_SLOGC_MEDIA, _SLOG_DEBUG1, "%s:%d ** input (%p) in length 0x%x", __func__, __LINE__, input,  in_filled_len);
#endif
    /* at a minimum output should NOT be NULL for a decode to happen */
    /* if terminate is set, return that as part of the status. The other fields of the status should be reset
     * so that app does not take any action on those */
    if(output != NULL) {
        status = vpu_decode_frame(hdl->hdl, (unsigned long)input, (unsigned long)output, is_eos,
                in_filled_len, dec_status);
        hdl->terminate = dec_status->terminate;
        if ( status != EOK) {
            LOG(LOG_ERROR, "%s:%d decode error", __func__, __LINE__);
            omx_status = OMX_ErrorHardware;
        } else {
            /* Yay, we got some data. Feed it back to the app. */
            output->nFilledLen = dec_status->o_filled_len;
#if defined (DEBUG_MODE)
            slogf(_SLOGC_MEDIA, _SLOG_INFO, "%s:%d decoded output (%p) with input (%p) and out length 0x%x in length 0x%x", __func__, __LINE__, output, input, output->nFilledLen, in_filled_len);
            LOG(LOG_INFO, "input (%p) output (%p) output length is %x", input, output, output->nFilledLen);
#endif
            buf_done(hdl, hdl->output_bufs, dec_status);
        }
    }

    return omx_status;
}

static inline void *omxil_get_omxbuf(codec_t *c_hdl, uintptr_t vpu_buf)
{
    vpu_buffer_t *vbufs = c_hdl->input_bufs;
    for(int i = 0; i < c_hdl->input_buf_num; i++) {
        if(vbufs[i].phys_addr == vpu_buf) {
#if defined (DEBUG_MODE)
            slogf(_SLOGC_MEDIA, _SLOG_DEBUG2, "%s:%d input buffer returned is  0x%p", __func__, __LINE__, vbufs[i].usr_info.priv);
#endif
            return vbufs[i].usr_info.priv; 
        }
    }
    return NULL;
}


static inline vpu_buffer_t *omxil_get_vpu_buf(vpu_buffer_t *vbufs, int nbuf, void *buf_hdr)
{
    for(int i = 0; i < nbuf; i++) {
        if(vbufs[i].usr_info.priv == buf_hdr)
            return &(vbufs[i]);
    }
    return NULL;
}


OMX_ERRORTYPE omxil_decodeFrame(void *hdl, OMX_BUFFERHEADERTYPE *input, OMX_BUFFERHEADERTYPE *output)
{
    codec_t *c_hdl = (codec_t *)hdl;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    if(!c_hdl)
        return OMX_ErrorBadParameter;

    /* We are done decoding. Release the buffers */
    /* Do we release the input buffers too?? Probably not. */
    if(c_hdl->terminate) {
        slogf(_SLOGC_MEDIA, _SLOG_INFO, "%s:%d no more decodes. Release output buffers", __func__, __LINE__);
        tivpu_dec_release_buffer(hdl, output);
        return OMX_ErrorNone;
    }

    /* at this point we have an input that can be used.
     * If we are not reusing, the input passed could be NULL.
     * This happens if we are holding onto all
     * (both in case of ping-pong) buffers.
     */
    if(input) {
#if defined (DEBUG_MODE)
        slogf(_SLOGC_MEDIA, _SLOG_INFO, "%s:%d input->nFlags = 0x%x", __func__, __LINE__, input->nFlags);
#endif
        /* get the eos flag too to resmgr. This needs to be handled
         * TODO: Check if this is required. The app should set stop_streaming as needed.
         */
        if(input->nFlags & OMX_BUFFERFLAG_EOS) {
            vpu_dec_stop_streaming(c_hdl->hdl);
            LOG(LOG_DEBUG1, "%s:%d EOS received", __func__, __LINE__);
#if defined (DEBUG_MODE)
            slogf(_SLOGC_MEDIA, _SLOG_INFO, "%s:%d EOS received = 0x%x", __func__, __LINE__, input->nFlags);
#endif
        }
    }

    /* try to decode a frame worth of bitstream data and return it to app via buf_done */
    if(output != NULL) {
        vpu_dec_status_t dec_status = {0};
        err = attempt_decode(c_hdl, input, output, &dec_status);
        if(err != OMX_ErrorNone) {
            LOG(LOG_ERROR, "%s:%d decode failed", __func__, __LINE__);
            return OMX_ErrorHardware;
        }

#if defined (DEBUG_MODE)
        dump_dec_status(&dec_status);
#endif
        /* Return the ip buffer, since we know we consumed it */
        if(dec_status.vpu_return_buf != 0) {
            OMX_BUFFERHEADERTYPE *in_buf_done = (OMX_BUFFERHEADERTYPE *) omxil_get_omxbuf(c_hdl, dec_status.vpu_return_buf);
            if (in_buf_done) {
#if defined (DEBUG_MODE)
                slogf(_SLOGC_MEDIA, _SLOG_INFO, "%s:%d EBD called for input (%p) to queue",  __func__, __LINE__, in_buf_done);
#endif
                /* Trigger input buffer processing here if the Decoder reports one consumed */
                c_hdl->callback(c_hdl->cb_ctx, in_buf_done, QOMX_EMPTY_BUFFER_DONE);
            }
        }
    }

    return err;
}

