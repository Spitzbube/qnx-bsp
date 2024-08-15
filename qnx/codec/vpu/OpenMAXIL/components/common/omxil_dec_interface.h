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

#ifndef _OMXIL_DEC_INTERFACE_H_
#define _OMXIL_DEC_INTERFACE_H_

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Video.h>

#include <list.h>

#include "tivpu_dec.h"

#define VDEC_INPUT_BUF_SIZE (5*1024*1024) // Input Buffer size - set to: ((1 / NUM_IN_BUFFERS) * 10MB)

/**
 *  Event types of callback
 */
typedef enum {
	QOMX_EMPTY_BUFFER_DONE = 0,
	QOMX_FILL_BUFFER_DONE,
	QOMX_RELEASE_OUTPUT_BUFFER,
	QOMX_EOS,
	QOMX_ERROR
} decoder_cb_type;

typedef OMX_ERRORTYPE (*omxil_decode_callback)(void *ctx, void *data, decoder_cb_type type);

typedef struct codec_handle
{
    void              *hdl;
    DecoderContext_t  *ctx;
    vpu_buffer_t      *input_bufs;
    vpu_buffer_t      *output_bufs;
    int               input_buf_num;    //number of input buffers
    int               output_buf_num;   //number of output buffers
    uint32_t          output_buf_cnt;   //count of buffers send to codec
    uint32_t          output_frame_cnt; //count of frames returned from codec.
    uint32_t          currDispIdx;
    uint32_t          priorDispIdx;
    uint32_t          frameDisplayed;
    bool              terminate;        // Do not process decode after terminate.
    OMX_BUFFERHEADERTYPE    **input_queue;      // Stores info about input bufs that have not been processed yet. 
                                        // there is data left in the input to be reused for a sub
                                        // sequent decode.
    int                     w_idx;
    int                     r_idx;
    int                     in_queue_size;
    
    omxil_decode_callback    callback;
    void*                    cb_ctx;
}codec_t;

/*
 * Create decoder handle
 */
void *omxil_create_decoder(omxil_decode_callback cb, void *cb_ctx);

/*
 * Init decoder
 */
OMX_ERRORTYPE omxil_init_decoder(void *hdl, OMX_VIDEO_PORTDEFINITIONTYPE *pInPortFormat,OMX_VIDEO_PORTDEFINITIONTYPE *pOutPortFormat, uint32_t coreIdx, uint8_t error_conceal, uint8_t DecBufferCount);

/*
 * Decode a frame
 */
OMX_ERRORTYPE omxil_decodeFrame(void *hdl, OMX_BUFFERHEADERTYPE *input, OMX_BUFFERHEADERTYPE *output);

/*
 * Register output buffers
 */
OMX_ERRORTYPE omxil_dec_register_buffers(void *hdl, BufferList *pBufList);

/*
 * Start decoder
 */
OMX_ERRORTYPE omxil_dec_start(void *hdl);

/*
 * Stop decoder
 */
OMX_ERRORTYPE omxil_dec_stop(void *hdl);

/*
 * Free decoder handle and resources.
 */
void omxil_close_decoder(void *hdl);

/*
 * Validate parameters for input port.
 */
OMX_ERRORTYPE omxil_validate_inport_parameters(void *hdl, OMX_PARAM_PORTDEFINITIONTYPE *pPortDef);

/*
 * Validate parameters for output port.
 */
OMX_ERRORTYPE omxil_validate_outport_parameters(void *hdl, OMX_PARAM_PORTDEFINITIONTYPE *pPortDef);
#endif

