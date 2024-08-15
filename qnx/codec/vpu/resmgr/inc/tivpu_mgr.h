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

#ifndef _TI_VPUMGR_H_INCLUDED
#define _TI_VPUMGR_H_INCLUDED

#include <stdint.h>
#include <sys/neutrino.h>

#include "vpuapi.h"
#include "vdi.h"

#include "tivpu_enc.h"
#include "tivpu_dec.h"
/*
 * The following devctls are used by a client application to access VPU
 */
#include <devctl.h>

#if defined (__cplusplus)
extern "C" {
#endif

#define TIVPU_DEVICE_NAME       "/dev/vpu-codec"
#define _DCMD_TIVPU_CODEC         0x3A


#define MAX_OBAND_ERROR  5 // Tolerated maximum value of consecutive 'Out Of Band' event fetching errors

enum pulse_codes
{
    PULSE_CODE_VPUENC        = _PULSE_CODE_MINAVAIL,  // Event pulses from VPUENC when oband notification (callback)  needs processing
    PULSE_CODE_VPUENC_CLOSE,                          // Pulse we send to ourselves when an OCB is being closed
};

// Move to tivpu_mgr.h
typedef struct tivpu_codec_s {
    int fd;
    int chid;
    int coid;
    struct sigevent event;
#if VPU_FEATURE_OOB
    void (*mm_ret_resource)(struct mm_buffer *buf, mm_enc_process_cb cb_type, void *cb_ctx);
#endif
    void *cb_ctx;
    void *ref_hdl; //Will hold dec/dec handle here. Is internal to the resmgr. Only holding a ref here. User does not know about it. at least not directly from here.
    pthread_t tid;
    // maybe need an enum to say if it is encoder/decoder and use that for error checking
} tivpu_codec_t;




#if VPU_FEATURE_OOB
/**
 * \verbatim
 *  MM Encoder supported callback types
 *  Application shall use this type to free/display input/output buffers \endverbatim
 */
typedef enum {
	MM_CB_CODED_BUFF_READY,
	MM_CB_SRC_FRAME_RELEASE,
	MM_CB_ENC_STR_END,
	MM_CB_ENC_ERROR_FATAL,
	MM_CB_ENC_FORCE32BITS = 0x7FFFFFFFU
} mm_enc_process_cb;


struct mm_buffer {
    void *temp_ptr;
};


typedef struct TIVPU_encGetOutBand_s {
    struct mm_buffer  buf;
    mm_enc_process_cb type;
} TIVPU_encGetOutBand_t;
#endif //VPU_FEATURE_OOB

typedef struct TIVPU_CmdArgs {
    union {
        struct {
            uint32_t coreIdx;
            int32_t state;
        } isInit;
        union {
            tivpu_enc_config_t enc_config;
            tivpu_dec_config_t dec_config;
        }cfg;
        struct {
            tivpu_codec_t vpu_hdl;
            int32_t nbuf;
            uint8_t bufDir;
        }prepareBuf;
        struct {
            uint32_t nbuf;
            uint32_t streamBufSize;
        }getBufInfo;
        struct {
            uint32_t coreIdx;
            int32_t instanceNum;
        } openInstanceNum;
        struct {
            unsigned long ipBuf;
            unsigned long opBuf;
            unsigned long opfirstBuf;
            uint8_t isEos;
            //out from the resmgr
            vpu_enc_status_t encStatus;
        } encodeFrameParams;
        struct {
            unsigned long ipBuf;
            unsigned long opBuf;
            unsigned long inFilledLen;
            uint8_t isEos;
            //out from the resmgr
            vpu_dec_status_t decStatus;
        } decodeFrameParams;
#if 0
        struct {
            vpu_enc_status_t encStatus;
        } ioEncodeStatus;
#endif
        struct {
            uint32_t coreIdx;
            VpuAttr attr;
        } vpuHwInfo;
        struct {
            tivpu_codec_t ref_hdl; // This should be a token instead of the actual object.
        }destroy_params;
    } args;
    uint32_t status;
} TIVPU_CmdArgs;


typedef enum {
    VPU_ENCODER,   /* Identifies the codec instance as an encoder */
    VPU_DECODER    /* Identifies the codec instance as a decoder */
} vpu_codec_type;


/**
* DEVCTL command IDs for the tivpu resource manager
*/

/*  ----------------------------------------------------------------------------
 * DEVCTL command IDs for the resource manager
 *  ----------------------------------------------------------------------------
 */
#define DCMD_TIVPU_ENC_CREATE            __DIOTF(_DCMD_TIVPU_CODEC,\
                                         1,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_ENC_BUF_PREPARE       __DIOTF(_DCMD_TIVPU_CODEC,\
                                         2,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_ENC_DESTROY           __DIOTF(_DCMD_TIVPU_CODEC,\
                                         3,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_ENC_START             __DIOTF(_DCMD_TIVPU_CODEC,\
                                         4,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_ENC_STOP              __DIOTF(_DCMD_TIVPU_CODEC,\
                                         5,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_ENC_PROCESS           __DIOTF(_DCMD_TIVPU_CODEC,\
                                         6,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_ENC_BUF_INFO      __DIOTF(_DCMD_TIVPU_CODEC,\
                                         7,\
                                         TIVPU_CmdArgs)

#if VPU_FEATURE_OOB
#define DCMD_TIVPU_GETOUTBAND           __DIOTF(_DCMD_TIVPU_CODEC,\
                                         8,\
                                         TIVPU_encGetOutBand_t)
#endif //VPU_FEATURE_OOB

#define DCMD_TIVPU_GET_PRODUCT_INFO     __DIOTF(_DCMD_TIVPU_CODEC,\
                                         9,\
                                         TIVPU_CmdArgs)

// Not implemented/unwanted
#define DCMD_TIVPU_GET_FRAMEBUF_SIZE    __DIOTF(_DCMD_TIVPU_CODEC,\
                                         10,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_DEC_CREATE            __DIOTF(_DCMD_TIVPU_CODEC,\
                                         21,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_DEC_BUF_PREPARE       __DIOTF(_DCMD_TIVPU_CODEC,\
                                         22,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_DEC_DESTROY           __DIOTF(_DCMD_TIVPU_CODEC,\
                                         23,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_DEC_START             __DIOTF(_DCMD_TIVPU_CODEC,\
                                         24,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_DEC_STOP              __DIOTF(_DCMD_TIVPU_CODEC,\
                                         25,\
                                         TIVPU_CmdArgs)

#define DCMD_TIVPU_DEC_PROCESS           __DIOTF(_DCMD_TIVPU_CODEC,\
                                         26,\
                                         TIVPU_CmdArgs)

// Not implemented/unwanted
#define DCMD_TIVPU_DEC_GET_FRAMEBUF_SIZE __DIOTF(_DCMD_TIVPU_CODEC,\
                                         27,\
                                         TIVPU_CmdArgs)

#if defined (__cplusplus)
}
#endif


#endif /* _TI_TIVPUMGR_H_INCLUDED */
