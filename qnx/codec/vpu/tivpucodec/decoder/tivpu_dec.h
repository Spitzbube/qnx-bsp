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

#include "vpuapi.h"
#include "main_helper.h"

#define STREAM_BUF_SIZE_DEFAULT (4*1024*1024)
#define STREAM_BUF_SIZE_HEVC   (10*1024*1024)  // bitstream size(HEVC:10MB)

//TODO:: This is moved to main_help.h. The following lines will go away after fully
//       moving to resmgr
#if 0
typedef enum {
    GET_PARAM_COM_STATE,                    /*!<< It returns state of component. Param: ComponentState* */
    GET_PARAM_COM_IS_CONTAINER_CONUSUMED,   /*!<< pointer of PortContainer */
    GET_PARAM_FEEDER_BITSTREAM_BUF,         /*!<< to a feeder component  : ParamDecBitstreamBuffer */
    GET_PARAM_FEEDER_EOS,                   /*!<< to a feeder component  : BOOL */
    GET_PARAM_VPU_STATUS,                   /*!<< to a component. Get status information of the VPU : ParamVpuStatus. */
    GET_PARAM_DEC_HANDLE,
    GET_PARAM_DEC_CODEC_INFO,               /*!<< It returns a codec information. Param: DecInitialInfo */
    GET_PARAM_DEC_BITSTREAM_BUF_POS,        /*!<< to a decoder component in ring-buffer mode. */
    GET_PARAM_DEC_FRAME_BUF_NUM,            /*!<< to a decoder component : ParamDecNeedFrameBufferNum*/
    GET_PARAM_RENDERER_FRAME_BUF,           /*!<< to a renderer component. ParamDecFrameBuffer */
    GET_PARAM_RENDERER_PPU_FRAME_BUF,       /*!<< to a renderer component. ParamDecPPUFrameBuffer */
    GET_PARAM_SRC_FRAME_INFO,
    GET_PARAM_ENC_HANDLE,
    GET_PARAM_ENC_FRAME_BUF_NUM,
    GET_PARAM_ENC_FRAME_BUF_REGISTERED,
    GET_PARAM_YUVFEEDER_FRAME_BUF,
    GET_PARAM_READER_BITSTREAM_BUF,
    GET_PARAM_MAX
} GetParameterCMD;
#endif

typedef enum {
    // Common commands
    SET_PARAM_COM_PAUSE,                        /*!<< Makes a component pause. A concrete component needs to implement its own pause state. */
    // Decoder commands
    SET_PARAM_DEC_SKIP_COMMAND,                 /*!<< Send a skip command to a decoder component. */
    SET_PARAM_DEC_TARGET_TID,                   /*!<< Send a target temporal id to a decoder component.
                                                      A parameter is pointer of ParamDecTargetTid structure. */
    SET_PARAM_DEC_RESET,                        /*!<< Reset VPU */
    SET_PARAM_DEC_FLUSH,                        /*!<< Flush command */
    //Encoder commands
    SET_PARAM_ENC_SUBFRAMESYNC,
    SET_PARAM_ENC_READ_BS_WHEN_FULL_INTERRUPT,  /*!<< Consume the bitstream buffer when the bitstream buffer full interrupt is asserted.
                                                      The parameter is a pointer of BOOL(TRUE or FALSE)
                                                 */
    // Renderer commands
    SET_PARAM_RENDERER_FLUSH,                   /*!<< Drop all frames in the internal queue depending on the ParamDecFlush struct*/
    SET_PARAM_RENDERER_ALLOC_FRAMEBUFFERS,
    SET_PARAM_RENDERER_REALLOC_FRAMEBUFFER,     /*!<< Re-allocate a framebuffer with given parameters.
                                                      A component which is linked with a decoder as a sink component MUST implement this command. : ParamReallocFB
                                                 */
    SET_PARAM_RENDERER_INTRES_CHANGED_ALLOC_FRAMEBUFFERS, /*!<< allocate a framebuffer for Inter resolution changed */
    SET_PARAM_RENDERER_FREE_FRAMEBUFFERS,       /*!<< A command to free framebuffers */
    SET_PARAM_RENDERER_CHANGE_COM_STATE,        /*!<< A command to change a component state for renderer */
    SET_PARAM_RENDERER_INTRES_CHANGED_FREE_FRAMEBUFFERS, /*!<< A command to free framebuffers in case of inter
                                                         resolution changed */
    SET_PARAM_RENDERER_RELEASE_FRAME_BUFFRES, /* A command to release all framebuffers allocated in renderer */
    // Feeder commands
    SET_PARAM_FEEDER_START_INJECT_ERROR,        /* The parameter is null. */
    SET_PARAM_FEEDER_STOP_INJECT_ERROR,         /* The parameter is null. */
    SET_PARAM_FEEDER_RESET,
    SET_PARAM_FEEDER_REFILL_BS_BUFFER,
    SET_PARAM_MAX
} SetParameterCMD;

typedef enum {
    DEC_INT_STATUS_NONE,        // Interrupt not asserted yet
    DEC_INT_STATUS_EMPTY,       // Need more es
    DEC_INT_STATUS_DONE,        // Interrupt asserted
    DEC_INT_STATUS_TIMEOUT,     // Interrupt not asserted during given time.
} DEC_INT_STATUS;

/**
 * \verbatim
 *  Supported callback types
 *  Application shall use this type to free/display input/output buffers \endverbatim
 */
typedef enum {
	STRUNIT_PROCESSED,
	SPS_RELEASE,
	PPS_RELEASE,
	PICT_DECODED,
	PICT_DISPLAY,
	PICT_RELEASE,
	PICT_END,
	STR_END,
	ERROR_FATAL,
	FORCE32BITS = 0x7FFFFFFFU
} process_cb;


/* \brief   Initialize the VPU instance, based on coreIdx, with the firmware at the given path
 */
int32_t tivpu_codec_init(uint32_t coreIdx, char* path, int op_polling);

/* \brief   Initialize the decoder opening params in a newly created Decoder Context structure
*/
int32_t tivpu_dec_open_params(void** hdl, tivpu_dec_config_t* dec_config);

/* \brief   Get the VPU product info and fill the attributes into the Decoder Context struct
 */
int32_t tivpu_dec_get_product_info(uint32_t coreIdx, DecoderContext_t* ctx);

/* \brief   Get Decoder parameters or status
 */
ComponentParamRet tivpu_get_parameter_decoder(DecoderContext_t* ctx, GetParameterCMD commandType, void* data);

/* \brief   Execute Decoder functions based on Decoder state
 */
BOOL tivpu_execute_decoder(tivpu_context_t* vpu_ctx, vpu_buffer_t* inbuf, vpu_buffer_t* outbuf, void *ret_buf);

/* \brief   Release the Decoder state
 */
void tivpu_release_decoder(DecoderContext_t* ctx);

/* \brief   Shutdown the VPU Decoder properly
 */
BOOL tivpu_destroy_decoder(DecoderContext_t* ctx);

/* \brief   Get Bitstream buffer status and return elements into DecoderContext_t struct members
 */
ComponentParamRet tivpu_get_parameter_feeder(DecoderContext_t* ctx, GetParameterCMD commandType, void* data);

/* \brief   Prepare the Decoder by registering the input bitstream buffers
 */
BOOL tivpu_prepare_feeder(DecoderContext_t* ctx);

/* \brief   Detach the input bitstream buffers to prepare to shutdown
 */
void tivpu_release_feeder(DecoderContext_t* ctx);

/* \brief   Get Renderer parameters or status
 */
ComponentParamRet tivpu_get_parameter_renderer(DecoderContext_t* ctx, GetParameterCMD commandType, void* data);

/* \brief   Set Renderer parameters or status
 */
ComponentParamRet tivpu_set_parameter_renderer(DecoderContext_t* ctx, SetParameterCMD commandType, void* data);

/* \brief   Prepare to render by completing AllocateFrameBuffer and setting framebuffers status
 */
BOOL tivpu_prepare_renderer(DecoderContext_t* ctx, vpu_buffer_t* outbuf);


/* \brief   Stop the decoder by updating the last attribute
 */
int32_t tivpu_dec_stop(DecoderContext_t *vpu_ctx);

/* \brief   Start the decoder by setting the first attribute
 */
int32_t tivpu_dec_start(DecoderContext_t *vpu_ctx);

/* \brief   register the buffers needed for decode. These would be called once for output and one for input
 */
int32_t tivpu_dec_register_buffers(tivpu_context_t *vpu_ctx, vpu_buffer_t *bufs, int32_t nbuf, uint8_t bufDir);


int32_t tivpu_dec_process(tivpu_context_t *vpu_ctx, void *in_buf, void *out_buf, uint32_t in_len, uint8_t is_eos,
                                 vpu_dec_status_t *dec_status);
