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
* All resmgr userlib calls should land here. The library
* handles calls to the resource manager.
* It exposes APIs to call in an encoder or a decoder that
* is in turn managed by the resource manager.
**/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>     /* SIGIO */
#include <fcntl.h>      /* fcntl */
#include <pthread.h>
#include <sys/mman.h>   /* mmap */
#include <sys/ioctl.h>
#include <errno.h>
#include <inttypes.h>
#include <devctl.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>

#include "vpu_usr.h"
#include "tivpu_mgr.h"
//#include "mm_common.h"

#include <sys/slog2.h>


#define vpulib_slogerr(...)    slogf(_SLOG_SETCODE(_SLOGC_PRIVATE_START, 55), _SLOG_ERROR, __VA_ARGS__)
#define vpulib_sloginfo(...)   slogf(_SLOG_SETCODE(_SLOGC_PRIVATE_START, 55), _SLOG_INFO, __VA_ARGS__)
#define vpulib_slogdbg(...)   slogf(_SLOG_SETCODE(_SLOGC_PRIVATE_START, 55), _SLOG_DEBUG2, __VA_ARGS__)



#if VPU_FEATURE_OOB
/* VPU resource manager boiler plate functions */
static int32_t notify_arm( tivpu_codec_t *hdl )
{
    int32_t err;
    if( (err = ionotify(hdl->fd, _NOTIFY_ACTION_POLLARM,  _NOTIFY_COND_OBAND, &hdl->event)) == -1 ) {
        err= errno;
    }
    else if( err > 0 ) {
        // The condition is already satisfied
        if( (err = MsgSendPulse(hdl->coid, -1, PULSE_CODE_VPUENC, 0)) == -1) {
            err = errno;
        }
    }
    return err;
}


static void *notify_thread( void *arg )
{
    tivpu_codec_t *hdl  = (tivpu_codec_t *) arg;
    int32_t         di   = -1;
    int32_t         ecount = 0;
    int32_t         rcvid;
    struct _pulse   msg;
    int32_t         err;

    pthread_setname_np(0, "vpuenc_listener");
    while( 1 ) {
        if( (err = notify_arm(hdl)) != EOK ) {
            if(hdl->mm_ret_resource != NULL)
                hdl->mm_ret_resource(NULL, MM_CB_ENC_ERROR_FATAL, hdl->cb_ctx);
            break;
        }
        else {
            if( (rcvid = MsgReceive(hdl->chid, &msg, sizeof(msg), NULL)) != EOK ) {
                if(hdl->mm_ret_resource != NULL)
                    hdl->mm_ret_resource(NULL, MM_CB_ENC_ERROR_FATAL, hdl->cb_ctx);
                break;
            }

            if( msg.code != PULSE_CODE_VPUENC ) {
                break;
            } /* TODO:: enable this later
            else {
                VPUENC_GetOutBand_t io = {0};
                if( (err = devctl(hdl->fd, DCMD_MM_ENC_GETOUTBAND, &io, sizeof(io), &di)) != EOK ) {
                    ecount++;
                    if( ecount >= MAX_OBAND_ERROR ) {
                        if(hdl->mm_ret_resource != NULL)
                            hdl->mm_ret_resource(NULL, MM_CB_ENC_ERROR_FATAL, hdl->cb_ctx);
                        break;
                    }
                }
                else {
                    // success
                    ecount = 0;
                    if(hdl->mm_ret_resource != NULL) {
                        struct mm_buffer *buf = io.buf.rm_private;
                        if(buf)
                            memcpy(buf, &io.buf, sizeof(struct mm_buffer));
                        hdl->mm_ret_resource(buf, io.type, hdl->cb_ctx);
                    }
                }
            } */
        }
    }
    return NULL;
}


static int32_t notify_create( tivpu_codec_t* hdl )
{
    int32_t err;
    vpulib_slogdbg("%s: %d\n",__func__, __LINE__);
    // Create a channel where notifications can trickle in on the different events
    if( (hdl->chid = ChannelCreate(_NTO_CHF_PRIVATE)) == -1 ) {
        err =  errno;
        return err;
    }
    else {
        if( (hdl->coid = ConnectAttach(0, 0, hdl->chid, _NTO_SIDE_CHANNEL, _NTO_COF_REG_EVENTS )) == -1) {
            err = errno;
        }
        else {
            SIGEV_PULSE_INIT(&hdl->event, hdl->coid, SIGEV_PULSE_PRIO_INHERIT, PULSE_CODE_VPUENC, 0);
            if( (err = MsgRegisterEvent_r(&hdl->event, hdl->fd)) == EOK ) {
                if( (err = pthread_create( &hdl->tid, NULL, notify_thread, hdl ) ) == EOK ) {
                    // success
                    vpulib_slogdbg("Success: %s coid is %di chid is %d\n", __func__, hdl->coid, hdl->chid);
                    return err;
                }
                else {
                    MsgUnregisterEvent(&hdl->event);
                }
            }
            ConnectDetach(hdl->coid);
        }
        vpulib_slogdbg("%s coid is %di chid is %d\n", __func__, hdl->coid, hdl->chid);
        ChannelDestroy( hdl->chid );
    }
    return err;
}

static void notify_destroy( tivpu_codec_t* hdl )
{
  MsgSendPulse(hdl->coid, -1, PULSE_CODE_VPUENC_CLOSE, 0);
  pthread_join(hdl->tid, NULL);
  MsgUnregisterEvent(&hdl->event);
  ConnectDetach(hdl->coid);
  ChannelDestroy(hdl->chid);
}
#endif //VPU_FEATURE_OOB

/* VPU resource manager devctls. TODO: need to pass codec info to the open call or have a devctl post that */
void *vpu_codec_open(void)
{
    tivpu_codec_t *hdl = (tivpu_codec_t *)calloc(1, sizeof(tivpu_codec_t));
    if(hdl == NULL) {
        return NULL;
    }
    vpulib_slogdbg("%s: %d\n",__func__, __LINE__);

    hdl->fd = open(TIVPU_DEVICE_NAME, O_RDWR);
    if(hdl->fd == -1) {
        free(hdl);
        return NULL;
    }

#if VPU_FEATURE_OOB
    if( notify_create(hdl) != EOK ) {
        close(hdl->fd);
        free(hdl);
        return NULL;
    }
#endif //VPU_FEATURE_OOB

    return hdl;
}

//TODO:: Remove this. Not needed.
int32_t vpu_register_encoder(tivpu_codec_t* hdl, vpu_buf_dir dir)
{
    if(hdl == NULL) {
        return -1 ;
    }
    return -1;
}

int32_t vpu_enc_buf_prepare(void *hdl, vpu_buffer_t *bufs, int nbuf, vpu_buf_dir buf_dir)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if(hdl == NULL) {
        return -1;
    }
    TIVPU_CmdArgs cargs;

    iov_t iov_i[2];

    /* This should not be used. Instead use a token that can fetch the handle stored in the resmgr */
    cargs.args.prepareBuf.vpu_hdl = *vpu_hdl;
    cargs.args.prepareBuf.nbuf = nbuf;
    cargs.args.prepareBuf.bufDir = buf_dir;

    SETIOV(&(iov_i[0]), &cargs, sizeof(TIVPU_CmdArgs));
    SETIOV(&(iov_i[1]), bufs, sizeof(vpu_buffer_t)* nbuf);

    return devctlv(vpu_hdl->fd, DCMD_TIVPU_ENC_BUF_PREPARE, 2, 0, iov_i, NULL, NULL);
}

void vpu_codec_close(void *hdl)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    vpulib_slogdbg("%s: %d\n",__func__, __LINE__);

    if(hdl == NULL) {
        return;
    }

#if VPU_FEATURE_OOB
    notify_destroy(hdl);
#endif //VPU_FEATURE_OOB

    close(vpu_hdl->fd);
    free(vpu_hdl);
}

int32_t vpu_enc_start_streaming(void *hdl)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if(!hdl)
        return -1;
    vpulib_slogdbg("%s: %d\n",__func__, __LINE__);

    return devctl(vpu_hdl->fd, DCMD_TIVPU_ENC_START, NULL, 0, NULL);
}

int32_t vpu_enc_stop_streaming(void *hdl)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if(!hdl)
        return -1;
    vpulib_slogdbg("%s: %d\n",__func__, __LINE__);

    return devctl(vpu_hdl->fd, DCMD_TIVPU_ENC_STOP, NULL, 0, NULL);
}

int32_t vpu_get_product_info(void *hdl, uint32_t coreIdx, VpuAttr *vpuHwInfo)
{
    TIVPU_CmdArgs cargs;
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;


    vpulib_slogdbg("%s: %d\n",__func__, __LINE__);
    cargs.args.vpuHwInfo.coreIdx = coreIdx;
    int32_t status = devctl(vpu_hdl->fd, DCMD_TIVPU_GET_PRODUCT_INFO, &cargs, sizeof(cargs), NULL);
    memcpy(vpuHwInfo, &(cargs.args.vpuHwInfo.attr), sizeof(VpuAttr));

    return status;
}

int  vpu_enc_get_buf_info(void *hdl, uint32_t *nbuffers, uint32_t *max_size)
{
    TIVPU_CmdArgs cargs;
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if(!hdl)
        return -1;
    vpulib_slogdbg("%s: %d\n",__func__, __LINE__);

    int32_t status = devctl(vpu_hdl->fd, DCMD_TIVPU_ENC_BUF_INFO, &cargs, sizeof(cargs), NULL);

    if(status != EOK) {
        vpulib_slogerr("ERROR: %s: %d\n",__func__, __LINE__);
        return status;
    }

    *nbuffers = cargs.args.getBufInfo.nbuf;
    *max_size = cargs.args.getBufInfo.streamBufSize;
    return 0;
}

int32_t vpu_encode_frame(void *hdl, unsigned long ip_buf, unsigned long op_buf, unsigned long op_first_buf,
                            uint8_t is_eos, vpu_enc_status_t *enc_status)
{
    TIVPU_CmdArgs cargs;
    iov_t enc_iov[1];
    int32_t status = EOK;
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if (hdl == NULL)
    {
        return EINVAL;
    }

    //vpulib_slogdbg("%s: %d\n",__func__, __LINE__);

    cargs.args.encodeFrameParams.ipBuf = ip_buf;
    cargs.args.encodeFrameParams.opBuf = op_buf;
    cargs.args.encodeFrameParams.isEos = is_eos;
    cargs.args.encodeFrameParams.opfirstBuf = op_first_buf;

    SETIOV(&enc_iov[0], &cargs, sizeof(TIVPU_CmdArgs));
                   // check whether we should return here with return _RESMGR_NPARTS(1);
    status = devctlv(vpu_hdl->fd, DCMD_TIVPU_ENC_PROCESS, 1, 1, enc_iov, enc_iov, NULL);

    if(status != EOK) {
        vpulib_slogerr("ERROR: %s: %d\n",__func__, __LINE__);
        return status;
    } else {
        status = cargs.status;
        memcpy(enc_status, &cargs.args.encodeFrameParams.encStatus, sizeof(vpu_enc_status_t));
    }
    return status;
}

/* An encoder has to take care of configuring the codec. It puts this information for the decoder
 * to figure out how it should be set before decoding the stream.
 * */
int32_t  vpu_enc_init(void *hdl, tivpu_enc_config_t *config)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if (hdl == NULL)
    {
        return EINVAL;
    }

    iov_t iov[1];
    SETIOV(&(iov[0]), config, sizeof(tivpu_enc_config_t));

    int32_t status = devctlv(vpu_hdl->fd, DCMD_TIVPU_ENC_CREATE, 1, 1, iov, iov, NULL);

    if (status != EOK) {
        vpulib_slogerr("ERROR: %s: %d\n",__func__, __LINE__);
        return status;
    }

    return EOK;

}

int32_t  vpu_enc_deinit(void *hdl)
{
  tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

  if(hdl == NULL) {
            return EINVAL;
  }
        vpulib_slogdbg("%s: %d\n",__func__, __LINE__);
        return devctl(vpu_hdl->fd, DCMD_TIVPU_ENC_DESTROY, 0, 0, NULL);
}



/* An deccoder has to take care of configuring the codec. It puts this information for the decoder
 * to figure out how it should be set before decoding the stream.
 * */
int32_t  vpu_dec_init(void *hdl, tivpu_dec_config_t *config)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;
    TIVPU_CmdArgs cargs;

    if (hdl == NULL)
    {
        return EINVAL;
    }

    cargs.args.cfg.dec_config = *config; 
    int32_t status = devctl(vpu_hdl->fd, DCMD_TIVPU_DEC_CREATE, &cargs, sizeof(cargs), NULL);

    if (status != EOK) {
        vpulib_slogerr("ERROR: %s: %d\n",__func__, __LINE__);
        return status;
    }

    return EOK;

}

int32_t  vpu_dec_start_streaming(void *hdl)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if(!hdl)
        return -1;
    vpulib_slogdbg("%s: %d\n",__func__, __LINE__);

    return devctl(vpu_hdl->fd, DCMD_TIVPU_DEC_START, NULL, 0, NULL);
}

int32_t vpu_dec_stop_streaming(void *hdl)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if(!hdl)
        return -1;
    vpulib_slogdbg("%s: %d\n",__func__, __LINE__);

    return devctl(vpu_hdl->fd, DCMD_TIVPU_DEC_STOP, NULL, 0, NULL);
}

int32_t  vpu_dec_deinit(void *hdl)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if(hdl == NULL) {
        return EINVAL;
    }

    vpulib_slogdbg("%s: %d\n",__func__, __LINE__);
    return devctl(vpu_hdl->fd, DCMD_TIVPU_DEC_DESTROY, 0, 0, NULL);
}


int32_t  vpu_dec_close(void *hdl)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;
    if(hdl == NULL) {
        return -1 ;
    }

    //TODO: FOR VPU_FEATURE_OOB need to stop the notify thread too.

    close(vpu_hdl->fd);
    free(vpu_hdl);

    return (EOK);
}

int32_t vpu_dec_buf_prepare(void *hdl, vpu_buffer_t *bufs, int nbuf, vpu_buf_dir buf_dir)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if(hdl == NULL) {
        return -1;
    }
    TIVPU_CmdArgs cargs;

    iov_t iov_i[2];

    /* This should not be used. Instead use a token that can fetch the handle stored in the resmgr */
    cargs.args.prepareBuf.vpu_hdl = *vpu_hdl;
    cargs.args.prepareBuf.nbuf = nbuf;
    cargs.args.prepareBuf.bufDir = buf_dir;

    SETIOV(&(iov_i[0]), &cargs, sizeof(TIVPU_CmdArgs));
    SETIOV(&(iov_i[1]), bufs, sizeof(vpu_buffer_t)* nbuf);

    return devctlv(vpu_hdl->fd, DCMD_TIVPU_DEC_BUF_PREPARE, 2, 0, iov_i, NULL, NULL);
}

int32_t vpu_decode_frame(void *hdl, unsigned long ip_buf, unsigned long op_buf,
                            uint8_t is_eos, uint32_t in_filled_len, vpu_dec_status_t *dec_status)
{
    TIVPU_CmdArgs cargs;
    iov_t enc_iov[1];
    int32_t status = EOK;
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if (hdl == NULL)
    {
        return EINVAL;
    }

    //vpulib_slogdbg("%s: %d\n",__func__, __LINE__);

    cargs.args.decodeFrameParams.ipBuf = ip_buf;
    cargs.args.decodeFrameParams.opBuf = op_buf;
    cargs.args.decodeFrameParams.isEos = is_eos;
    cargs.args.decodeFrameParams.inFilledLen = in_filled_len;

    SETIOV(&enc_iov[0], &cargs, sizeof(TIVPU_CmdArgs));
                   // check whether we should return here with return _RESMGR_NPARTS(1);
    status = devctlv(vpu_hdl->fd, DCMD_TIVPU_DEC_PROCESS, 1, 1, enc_iov, enc_iov, NULL);

    if(status != EOK) {
        vpulib_slogerr("ERROR: %s: %d\n",__func__, __LINE__);
        return status;
    } else {
        status = cargs.status;
        memcpy(dec_status, &cargs.args.decodeFrameParams.decStatus, sizeof(vpu_dec_status_t));
    }
    return status;
}

#if VPU_FEATURE_OOB
int32_t  vpu_codec_register_callback(void *hdl,
        void (*mm_ret_resource)(struct mm_buffer *buf, mm_enc_process_cb cb_type, void *cb_ctx),
        void *cb_ctx)
{
    tivpu_codec_t *vpu_hdl = (tivpu_codec_t *)hdl;

    if(hdl == NULL) {
        return EINVAL;
    }
    vpu_hdl->mm_ret_resource = mm_ret_resource;
    vpu_hdl->cb_ctx = cb_ctx;

    return EOK;
}
#endif //VPU_FEATURE_OOB

