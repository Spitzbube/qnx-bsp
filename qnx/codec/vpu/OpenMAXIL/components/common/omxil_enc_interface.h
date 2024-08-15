/*
 * Copyright 2022, QNX Software Systems Ltd.
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

#ifndef OMXIL_ENC_INTERFACE_H
#define OMXIL_ENC_INTERFACE_H

#include "list.h"
#include "vpuapi.h"

typedef struct encoder_config_t
{
    OMX_COLOR_FORMATTYPE iFormat;
    OMX_VIDEO_CODINGTYPE oFormat;
    OMX_U32 width;
    OMX_U32 height;
    OMX_U32 stride;
    OMX_U32 profile;
    OMX_U32 level;
    OMX_U32 framerate;
    OMX_U32 keyFrameInterval;
    OMX_U32 qpI;
    OMX_U32 qpP;
    OMX_U32 bitrate;
    OMX_U32 rateControl;
    OMX_BOOL arithmeticEncoding;
    OMX_U32 sliceType;
    OMX_U32 sliceSize;
    OMX_U32 coreIdx;
    OMX_U32 setLossless;
    OMX_U32 setGOP;
    OMX_U32 sourceBufCount;
    OMX_U32 streamBufCount;
    OMX_U32 streamBufSize;
} encoder_config;

/**
 *  Event types of callback
 */
typedef enum {
	QOMX_EMPTY_BUFFER_DONE = 0,
	QOMX_FILL_BUFFER_DONE,
	QOMX_EOS,
	QOMX_ERROR
} encoder_cb_type;
/*
 * callback
 */
typedef OMX_ERRORTYPE (*omxil_encode_callback)(void *ctx, void *data, encoder_cb_type type);

/*
 * Create encoder handle
 */
void *omxil_create_encoder(omxil_encode_callback cb, void *cb_ctx);

/**
 * @brief Query buffer information for encoder output
 * @details
 * query the buffer size and buffer number for encoder output.
 *
 * @param: nBuffer: output, number of buffer
 *         uBufferSize: output, buffer size
 *
 * @return @c OMX_ErrorNone on success - error code on failure.
 */
OMX_ERRORTYPE omxil_query_buf_info(void *hdl, OMX_U32 *nBuffer, OMX_U32 *uBufferSize);
/**
 * @brief Validate parameters for input port
 *
 * @param: pPortDef: pointer to OMX_PARAM_PORTDEFINITIONTYPE
 *
 * @return @c OMX_ErrorNone on success - error code on failure.
 */
OMX_ERRORTYPE omxil_validate_inport_parameters(void *hdl, OMX_PARAM_PORTDEFINITIONTYPE *pPortDef);
/**
 * @brief Validate parameters for output port
 *
 * @param: pPortDef: pointer to OMX_PARAM_PORTDEFINITIONTYPE
 *
 * @return @c OMX_ErrorNone on success - error code on failure.
 */
OMX_ERRORTYPE omxil_validate_outport_parameters(void *hdl, OMX_PARAM_PORTDEFINITIONTYPE *pPortDef);
/**
 * @brief Initialization of the encoder
 * @details
 * This initialization needs to be done before the encoder can be used to encode frames.
 * Once encoding is complete, you must call shutdown() to free up any memory allocated
 * during the initialization.
 *
 * @param pPortDefIn: pointer to OMX_PARAM_PORTDEFINITIONTYPE (input port def struct)
 *        pPortDefOut: pointer to OMX_PARAM_PORTDEFINITIONTYPE (output port def struct)
 *        config: The initial configuration for the video encoder
 *
 * @return @c OMX_ErrorNone on success - error code on failure.
 */
OMX_ERRORTYPE omxil_enc_config(void *hdl, OMX_PARAM_PORTDEFINITIONTYPE *pPortDefIn, OMX_PARAM_PORTDEFINITIONTYPE *pPortDefOut, encoder_config *config);

/**
 * @brief close the encoder
 * @details
 * Must be called to clean up after the encoding session is complete.
 *
 * @return @c OMX_ErrorNone on success - error code on failure.
 */
OMX_ERRORTYPE omxil_close_encoder(void *hdl);

/**
 * @brief Start our encoding session
 * @details
 * Must be called after config() to permit the start of an encoding session.
 *
 * @return @c OMX_ErrorNone on success - error code on failure.
 */
OMX_ERRORTYPE omxil_enc_start(void *hdl);

/**
 * @brief Stop our encoding session
 * @details
 * Must be called to flush buffers out.
 *
 * @return @c OMX_ErrorNone on success - error code on failure.
 */
OMX_ERRORTYPE omxil_enc_stop(void *hdl);

/**
 * @brief Register buffers for input
 * @details
 * This method is called to register buffers for,
 * input frames. This method must be called after omxil_enc_start,
 * @param bufList pointer of BufferList
 *
 * @return @c OMX_ErrorNone on success - error code on failure.
 */
OMX_ERRORTYPE omxil_enc_register_buffers(void *hdl, BufferList *bufList);

/**
 * @brief Encodes the supplied frame
 * @details
 * The generated output from the encoded frame is not yet copied to an output buffer.
 * to copy this output in subsequent call to getOutput.
 *
 * @param input  The input buffer header that contains input data and informations.
 *        output The output buffer header that will get the encoded frame output data.
 *        output_first First frame only: the first output buffer header that will get the bitstream header.
 * @return @c OMX_ErrorNone on success - error code on failure.
 */
OMX_ERRORTYPE omxil_enc_encodeFrame(void *hdl, OMX_BUFFERHEADERTYPE *input, OMX_BUFFERHEADERTYPE *output, OMX_BUFFERHEADERTYPE *output_first);

/**
 * @brief Dynamically requests that the next encoded frame be a key frame
 * @details
 * For error recovery, it is useful to request a key frame to recover
 * quickly from an error that has occurred.
 *
 * @return @c OMX_ErrorNone on success - error code on failure.
 */
OMX_ERRORTYPE omxil_enc_forceKeyFrame(void *hdl);

#endif  // !OMXIL_ENC_INTERFACE_H


