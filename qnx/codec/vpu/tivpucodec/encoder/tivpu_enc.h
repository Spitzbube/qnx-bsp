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

#include "main_helper.h"

#ifndef __TIVPU_ENC_H__
#define __TIVPU_ENC_H__


typedef enum {
    ENC_INT_STATUS_NONE,        // Interrupt not asserted yet
    ENC_INT_STATUS_FULL,        // Need more buffer
    ENC_INT_STATUS_DONE,        // Interrupt asserted
    ENC_INT_STATUS_LOW_LATENCY,
    ENC_INT_STATUS_TIMEOUT,     // Interrupt not asserted during given time.
    ENC_INT_STATUS_SRC_RELEASED,
} ENC_INT_STATUS;

#if 0
//TODO:: Remove this later. This might not be needed
/**
 * \verbatim
 *  Supported callback types
 *  Application shall use this type to free/display input/output buffers \endverbatim
 */
typedef enum {
    CODED_BUFF_READY,
    SRC_FRAME_RELEASE,
    ENC_STR_END,
    ENC_ERROR_FATAL,
    ENC_FORCE32BITS = 0x7FFFFFFFU
} process_cb;


/**
 * \verbatim
 *  MM Encoder supported RC Modes
 *  Application shall use this type to configure encoder RC mode \endverbatim
 */
typedef enum {
	MM_ENC_VBR,
	MM_ENC_SVBR
} mm_enc_rcmode;


/**
 * \verbatim
 *  MM Decoder create time paramters, to be set/configured by application
 *  before MM_DEC_Create() \endverbatim
 */
typedef struct {
	/** Video frame width */
	uint32_t width;
	/** Video frame height */
	uint32_t height;
	/** Video frame rate*/
	uint32_t framerate;
	/** Refer enum mm_pixelformat for supported in_pixelformat values */
	uint32_t in_pixelformat;
	/** Refer enum mm_pixelformat for supported out_pixelformat values */
	uint32_t out_pixelformat;
} mm_vid_create_params;

/**
 * \verbatim
 *  MM Encoder supported features
 *  Application shall use this type to configure encoder features \endverbatim
 */
typedef enum {
	MM_ENC_FEATURE_CABAC = 0x0001,
	MM_ENC_FEATURE_8x8 = 0x0002,
	MM_ENC_FEATURE_DISABLE_INTRA4x4 = 0x0004,
	MM_ENC_FEATURE_DISABLE_INTRA8x8 = 0x0008,
	MM_ENC_FEATURE_DISABLE_INTRA16x16 = 0x0010,
	MM_ENC_FEATURE_DISABLE_INTER8x8 = 0x0020,
	MM_ENC_FEATURE_RESTRICT_INTER4x4 = 0x0040,
	MM_ENC_FEATURE_DISABLE_8x16_MV_DETECT = 0x0080,
	MM_ENC_FEATURE_DISABLE_16x8_MV_DETECT = 0x0100
} mm_enc_features;

/** @enum mapper::mm_pixelformat
 *  @brief MM Pixel Format
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_NV12
 *  0x01
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_NV12M
 *  0x02
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_NV16
 *  0x03
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_TI1210
 *  0x04
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_TI1210M
 *  0x05
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_TI1610
 *  0x06
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_H264
 *  0x07
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_HEVC
 *  0x08
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_MJPEG
 *  0x09
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_YUV420M
 *  0x0A
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_YUV422P
 *  0x0B
 *  @var mapper::mm_pixelformat::MM_PIX_FMT_FORCE32BITS
 *  0x7FFFFFFFU
 */
/**
 * \verbatim
 *  MM Pixel Format, only below options are supported as of today
 *  Input Format
 *     - MM_PIX_FMT_H264
 *  Output Format
 *     - MM_PIX_FMT_NV12
 *     - MM_PIX_FMT_TI1210 \endverbatim
 */
enum mm_pixelformat {
	MM_PIX_FMT_NV12 = 0x01,
	MM_PIX_FMT_NV12M = 0x02,
	MM_PIX_FMT_NV16 = 0x03,
	MM_PIX_FMT_TI1210 = 0x04,
	MM_PIX_FMT_TI1210M = 0x05,
	MM_PIX_FMT_TI1610 = 0x06,
	MM_PIX_FMT_H264 = 0x07,
	MM_PIX_FMT_HEVC = 0x08,
	MM_PIX_FMT_MJPEG = 0x09,
	MM_PIX_FMT_YUV420M = 0x0A,
	MM_PIX_FMT_YUV422P = 0x0B,
	MM_PIX_FMT_FORCE32BITS = 0x7FFFFFFFU
};


/**
 * \verbatim
 *  Enum describing smallest blocksize used during motion search
 *  MM_ENC_BLK_SZ_DEFAULT: Driver picks the best possible block size for this encode session
 *  MM_ENC_BLK_SZ_16x16: Use 16x16 block size for motion search. This is the smallest for h.263
 *  MM_ENC_BLK_SZ_8x8: Use 'upto' 8x8 block size for motion search. This is the smallest for MPEG-4
 *  MM_ENC_BLK_SZ_4x4: Use 'upto' 4x4 block size for motion search. This is the smallest for H.264
 */
typedef enum {
	MM_ENC_BLK_SZ_DEFAULT = 0,
	MM_ENC_BLK_SZ_16x16,
	MM_ENC_BLK_SZ_8x8,
	MM_ENC_BLK_SZ_4x4
} mm_enc_minblocksize;

/**
 * \verbatim
 *  MM Encoder Control Parameters, to be set/configured by application
 *  before MM_ENC_Create() \endverbatim
 */
typedef struct {
	/** Bit flags for encoding features */
	uint32_t features;
	/** RC Mode */
	mm_enc_rcmode rcmode;
	/** IDR-period */
	uint32_t idr_period;
	/** I-period */
	uint32_t i_period;
	/** Bitrate */
	uint32_t bitrate;
	/** Framerate */
	uint8_t framerate;
	/** Crop settings */
	uint32_t crop_left;
	uint32_t crop_right;
	uint32_t crop_top;
	uint32_t crop_bottom;
	/** # Slices */
	uint8_t nslices;
	/** base pipe */
	uint8_t base_pipe;
	/** Qp Settings */
	uint32_t initial_qp_i;
	uint32_t initial_qp_p;
	uint32_t initial_qp_b;
	uint32_t min_qp;
	uint32_t max_qp;
	/** Min Block Size for motion search */
	mm_enc_minblocksize min_blk_size;
	/** Controls H264COMP_INTRA_PRED_MODES register. Leave 0 for default. See TRM for details */
	uint32_t intra_pred_modes;
} mm_enc_ctrl_params;

/**
 * \verbatim
 *  MM Encoder Init Parameters, to be set/configured by application
 *  before MM_ENC_Init() \endverbatim
 */
typedef struct {
	void *ocm_ram_addr;
	uint32_t ocm_ram_size;
} mm_enc_init_params;


static inline void MM_ENC_SetDefaultInitParams(mm_enc_init_params *init_params)
{
	init_params->ocm_ram_addr = NULL;
	init_params->ocm_ram_size = 0;
};


/**
 *  \brief MM_ENC_Init - Initialize the encoder
 *  This function should called only once.
 *  \verbatim
 *  Return: 0          -> Success
 *  Return: -ve values -> Failure \endverbatim
 */
int32_t MM_ENC_Init(mm_enc_init_params *init_params);

/**
 *  \brief MM_ENC_Control - Set encoder controls
 *  This function should only be called before MM_ENC_Create
 *  \verbatim
 *  Return: 0          -> Success
 *  Return: -ve values -> Failure \endverbatime
 */
int32_t MM_ENC_Control();
#endif

/* \brief   Parameter Return Test: returns TRUE to go to the next step, else returns FALSE
*/
BOOL ParamReturnTest(ComponentParamRet ret, BOOL* success);

/* \brief   Initialize the encoder opening params in a newly created Encoder Context structure
*/
int32_t tivpu_enc_open_params(void** ctx, tivpu_enc_config_t* enc_config);

/* \brief   Configure encoder internal framebuffer and set up source buffers
 */
int32_t tivpu_enc_src_buf_config(EncoderContext_t* ctx, tivpu_enc_config_t* enc_config);

/* \brief   Get Encoder parameters or status
 */
ComponentParamRet tivpu_get_parameter_encoder(EncoderContext_t* ctx, GetParameterCMD commandType, void* data);

/* \brief   Execute Encoder functions based on Encoder state
 */
BOOL tivpu_execute_encoder(EncoderContext_t* ctx, vpu_buffer_t* inbuf, vpu_buffer_t* outbuf, vpu_buffer_t* outbuf_first);

/* \brief   Prepare the Encoder with configuration of the bitstream buffers
 */
BOOL tivpu_prepare_encoder(EncoderContext_t* ctx);

/* \brief   Shutdown the VPU Encoder properly
 */
BOOL tivpu_destroy_encoder(EncoderContext_t* ctx);

/* \brief   Register the bitstream buffers with the VDI driver
 */
BOOL tivpu_enc_bitstream_prepare(EncoderContext_t* ctx);

/* \brief   Get YUV framebuffer status and return elements into EncoderContext_t struct members
 */
ComponentParamRet tivpu_get_parameter_yuv_feeder(EncoderContext_t* ctx, GetParameterCMD commandType, void* data);

/* \brief   Set up parameters for taking in YUV inputs
 */
BOOL tivpu_create_yuv_feeder(EncoderContext_t* ctx);

/* \brief   Prepare the Encoder by registering the YUV input buffers and allocating the Reconstructed frame memory
 */
BOOL tivpu_prepare_yuv_feeder(EncoderContext_t* ctx, vpu_buffer_t* inbuf);

/* \brief   Execute the YUV feeder to format the YUV source buffer into the FrameBuffer format expected by the Encoder
 */
BOOL tivpu_execute_yuv_feeder(EncoderContext_t* ctx, vpu_buffer_t* inbuf);

/* \brief   Free up any FB memory that was previously allocated
 */
void tivpu_release_fb_mem(EncoderContext_t* ctx);

int32_t tivpu_enc_register_buffers(tivpu_context_t *vpu_ctx, vpu_buffer_t *bufs, int32_t nbuf, uint8_t bufDir);
int32_t tivpu_enc_process(tivpu_context_t *vpu_ctx, void *in_buf, void *out_buf, void *op_first_buf, uint8_t is_eos,
                            codec_dbg_info_t *info);
int32_t tivpu_enc_start(tivpu_context_t *vpu_ctx);
int32_t tivpu_enc_stop(tivpu_context_t *vpu_ctx);
int32_t tivpu_enc_destroy(tivpu_context_t *vpu_ctx);


#endif // __TIVPU_ENC_H__
