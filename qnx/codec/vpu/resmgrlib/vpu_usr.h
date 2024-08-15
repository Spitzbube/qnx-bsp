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

/**
 *  \defgroup TI_VPU_USR_LIB TI VPU resource manager library
 *  @{
 *
 * The TI VPU resource manager library provides a set of handy
 * APIs that are nedded to communicate with the VPU resource manager.
 * The mode of communication via QNX RM devctls. The user of this
 * library would be the applications that are interested in making
 * use of the VPU hardware to perform encode and decode operations.
 * This is the low level documentation for the libary.
 * 
 */

/* @} */


/**  \ingroup TI_VPU_USR_LIB
 *   \defgroup TI_VPU_TOP_LEVEL VPU user lib header
 *             This is the main header with all the function
 *             declarations for the usr lib.
 *
 *   @{
 */

/**
 *  \file vpu_usr.h
 *
 *  \brief TI VPU resource manager library API delcarations.
 */

#ifndef __VPU_USR_H__
#define __VPU_USR_H__



#include "vpuapi.h"

#if defined (__cplusplus)
extern "C" {
#endif

#include <sys/slog.h>
#include <sys/slogcodes.h>

#include "main_helper.h" // TODO change name for this. Needed for the tivpu_enc_config_t struct

/**
 *  \brief vpu_buf_dir enumeration. Identifies if it is an input of an output buffer
 */
/* Used to indicate the type of buffer passed to buf_prepare devctl */
typedef enum {
    TIVPU_BUFDIR_OUTPUT = 0,
    TIVPU_BUFDIR_INPUT
}vpu_buf_dir;


/* Functions exposed by the resmgr user lib */
/**
 *  \brief  vpu_codec_open
 *
 *          Open a vpu codec instance.
 *  \param [IN]  void
 *
 *  \return valid vpu codec handle, NULL otherwise
 */
void *vpu_codec_open(void);

/**
 *  \brief  vpu_codec_close
 *
 *          Closes the vpu codec instance identified by the hdl.
 *  \param [IN]  void * Address returned by the corresponding open call.
 *
 *  \return void
 */
void vpu_codec_close(void *hdl);

/**
 *  \brief  vpu_get_product_info
 *
 *          Get the vpu specific info in a VpuAttr structure
 *  \param [IN]  void *     Address returned by the corresponding open call.
 *  \param [IN]  uint32_t   The coreIdx for which the product info is being requested.
 *  \param [IN]  VpuAttr *  Address to the VpuAttr structure. This will be updated by the 
 *                          resource manager if the API succeeds.
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t vpu_get_product_info(void *hdl, uint32_t coreIdx, VpuAttr *vpuHwInfo);

/**
 *  \brief  vpu_enc_init
 *
 *          Initialize the encoder with a specific tivpu_enc_config_t
 *  \param [IN]  void *           Address returned by the corresponding open call.
 *  \param [IN]  tivpu_enc_config_t   Structure to configure the codec as an encoder.
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t vpu_enc_init(void *hdl, tivpu_enc_config_t *config);

/**
 *  \brief  vpu_enc_buf_prepare
 *
 *          Prime the encoder with the input/output buffers information
 *  \param [IN]  void *           Address returned by the corresponding open call.
 *  \param [IN]  vpu_buffer_t *   A pointer to an array of vpu_buffer_t structure.
 *  \param [IN]  int              The number of buffers in the vpu_buffer_t array
 *  \param [IN]  vpu_buf_dir      Input or output buffers being passed.
 *
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t vpu_enc_buf_prepare(void *hdl, vpu_buffer_t *bufs, int nbuf, vpu_buf_dir buf_dir);

/**
 *  \brief  vpu_encode_frame
 *
 *          Encode a single frame worth of raw video
 *  \param [IN]   void *             Address returned by the corresponding open call.
 *  \param [IN]   unsigned long      Address to the raw video data.
 *  \param [IN]   unsigned long      Address to the location where encoded data will be stored (temporarily)
                                    by the VPU hardware.
 *  \param [IN]   unsigned long      Address (can be NULL) to the first output buffer.
 *  \param [IN]   uint8_t            Flag to indicate if the input is EOS, true if it is end-of-stream;.
 *  \param [OUT]  vpu_enc_status_t * Pointer to the vpu_enc_status_t struct. Updated by the vpulib once the encode is done.
 *
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t vpu_encode_frame(void *hdl, unsigned long ip_buf, unsigned long op_buf, unsigned long op_first_buf,
                            uint8_t is_eos, vpu_enc_status_t *enc_status);

/**
 *  \brief  vpu_enc_get_buf_info
 *
 *          Get the minimum required buffer spec for the output buffers. This is based on the input stream.
 *  \param [IN]  void *           Address returned by the corresponding open call.
 *  \param [OUT] uint32_t *       Address to the location where the lib would write the number of buffers needed.
 *  \param [OUT] uint32_t *       Address to the location where the lib would write the size of a buffer.
 *
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t vpu_enc_get_buf_info(void *hdl, uint32_t *nbuffers, uint32_t *max_size);

/**
 *  \brief  vpu_enc_start_streaming
 *
 *          Mark the codec instance as ready to encode
 *  \param [IN]  void *           Address returned by the corresponding open call.
 *
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t vpu_enc_start_streaming(void *hdl);

/**
 *  \brief  vpu_enc_stop_streaming
 *
 *          Mark the codec to stop encoding
 *  \param [IN]  void *           Address returned by the corresponding open call.
 *
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t vpu_enc_stop_streaming(void *hdl);

/**
 *  \brief  vpu_enc_deinit
 *
 *          De-init the codec instance identified by the void * codec handle
 *  \param [IN]  void *           Address returned by the corresponding open call.
 *
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t vpu_enc_deinit(void *hdl);

/* Decoder apis */

/**
 *  \brief  vpu_dec_init
 *
 *          Initialize the decoder with a specific tivpu_dec_config_t
 *  \param [IN]  void *           Address returned by the corresponding open call.
 *  \param [IN]  tivpu_dec_config_t   Structure to configure the codec as an decoder.
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t  vpu_dec_init(void *hdl, tivpu_dec_config_t *config);
int32_t  vpu_dec_start_streaming(void *hdl);
int32_t  vpu_dec_stop_streaming(void *hdl);
int32_t  vpu_dec_deinit(void *hdl);
int32_t  vpu_dec_close(void *hdl);

/**
 *  \brief  vpu_dec_buf_prepare
 *
 *          Prime the decoder with the input/output buffers information
 *  \param [IN]  void *           Address returned by the corresponding open call.
 *  \param [IN]  vpu_buffer_t *   A pointer to an array of vpu_buffer_t structure.
 *  \param [IN]  int              The number of buffers in the vpu_buffer_t array
 *  \param [IN]  vpu_buf_dir      Input or output buffers being passed.
 *
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t  vpu_dec_buf_prepare(void *hdl, vpu_buffer_t *bufs, int nbuf, vpu_buf_dir buf_dir);

/**
 *  \brief  vpu_decode_frame
 *
 *          Decode bitstream to provide a single fram worth of output raw video.
 *  \param [IN]   void *             Address returned by the corresponding open call.
 *  \param [IN]   unsigned long      Address to the bitstream buffer.
 *  \param [IN]   unsigned long      Address to the location where decoded YUV data will be stored (temporarily)
                                    by the VPU hardware.
 *  \param [IN]   uint8_t            Flag to indicate if the input is EOS, true if it is end-of-stream;.
 *  \param [IN]   uint32_t           The amount of valid bitstream data available in the input buffer for the decoder to process
 *  \param [OUT]  vpu_dec_status_t * Pointer to the vpu_dec_status_t struct. Updated by the vpulib once the decode is done.
 *
 *  \return status of the API call. 0 for success, error otherwise.
 */
int32_t vpu_decode_frame(void *hdl, unsigned long ip_buf, unsigned long op_buf,
                            uint8_t is_eos, uint32_t in_filled_len, vpu_dec_status_t *dec_status);

#if defined (__cplusplus)
}
#endif

#endif // __VPU_USR_H__

/* @} */
