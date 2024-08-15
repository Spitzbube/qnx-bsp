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

#ifndef OMXIL_VENC_SESSION_H_
#define OMXIL_VENC_SESSION_H_

#include <pthread.h>
#include <stdbool.h>
#include <queue>

#include <screen/screen.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#define AO_LOG(DEV, ...)       slogf(_SLOGC_MEDIA, DEV, __VA_ARGS__)
#define AO_LOG_DEBUG2 SLOG2_DEBUG2
#define AO_LOG_DEBUG1 SLOG2_DEBUG1
#define AO_LOG_INFO   SLOG2_INFO
#define AO_LOG_ERROR  SLOG2_ERROR
#define AO_LOG_WARNING  SLOG2_WARNING

#define VENC_INPUT_BUFFER_NUM 3 /* JB: Need to be set same as NUM_IN_BUFFERS (omxil_component_enc.c) */

typedef enum OmxilBailReason {
    BAIL_NOT_BAILED,
    BAIL_ERROR,
    BAIL_INIT_FAIL,
    BAIL_INIT_DISP_FAIL,
    BAIL_MUTEX_INIT_ERROR,
    BAIL_CONDATTR_INIT_ERROR,
    BAIL_CONDATTR_CLK_ERROR,
    BAIL_COND_INIT_ERR
}OmxilBailReason;

typedef struct {
    void *addr;
    uint32_t size;
    off64_t offset; /* offset or physical address of the buffer. */
} OmxilEncIOBuffer_t;

typedef struct enc_params_s {
    const char *name;
    int32_t    *value;
    const int32_t range[2];
} enc_params_t;

typedef struct OmxilVideoEncDec_ {
    /* File paths*/
    const char *out_path;
    const char *in_path;
    const char *conf_path;
    int in_fd;
    int out_fd;
    /* For screen output or capture on Jacinto 6,
     * we need both NV12 and RGB to workaround the screen framebuffer
     * restriction, i.e. read into pixel map and convert to buffer.
     * might not be necessary on other platforms that support NV12 format
     * frame buffer from screen.
     */
    screen_context_t screen_ctx;
    screen_display_t display;
    screen_window_t  screen_win;
    OmxilEncIOBuffer_t input_bufs[VENC_INPUT_BUFFER_NUM];
    OmxilEncIOBuffer_t *output_bufs;

    /* Threads */
    pthread_t encoder_push;

    int input_format;

    /* when using bufferqueue output, this enables
     * the decoded data to be shown on screen too
     */
    bool post_to_screen;
    bool user_request_exit;

    int frame_rate;
    int src_width;
    int src_height;
    int aligned_height;
    int frame_size;
    int src_stride;

    //component
    OMX_HANDLETYPE compHandle;
    OMX_VIDEO_CODINGTYPE compressFmt;
    OMX_U32        inPortIndex;
    OMX_U32        outPortIndex;
    uint16_t numOfPorts; // Data ports
    bool           cmdComplete;
    bool           inPortFlushed;
    bool           outPortFlushed;
    bool           eos_received;
    bool           eos_sent;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    OMX_ERRORTYPE  compError;
    int core_idx;

    //output port
    OMX_U32 outputPortBufSize;
    OMX_U32 nOutputBufs;
    std::queue<OMX_BUFFERHEADERTYPE*> qInputBufHdr;
    std::queue<OMX_BUFFERHEADERTYPE*> qOutputBufHdr;

    //Input port
    OMX_U32 nInputBufs;
    OMX_U32 inputPortBufSize;

    uint32_t srcWidth; // source width, width from configure data, output buffer is allocated with this size
    uint32_t srcHeight; // source height, height from configure data
    bool     no_stdin;

    //encoder configure parameters
    int32_t coding_std;
    int32_t bitrate;
    int32_t idr_period;
    int32_t rcmode;
    int32_t use_alt_mem;

    int32_t input_frame_cnt;
    int32_t last_frame_num;
    int32_t record_duration;

    uint64_t enc_start_time_ms;
    uint64_t enc_eos_time_ms;
} OmxilEnc_t;

// -------------------
// interfaces
// -------------------
OMX_ERRORTYPE InitEncComp( OmxilEnc_t *encH );
void CloseVenc( OmxilEnc_t *encH );

OMX_ERRORTYPE StartVenc( OmxilEnc_t *encH );
OMX_ERRORTYPE StopVenc( OmxilEnc_t *encH );

OMX_ERRORTYPE PauseVenc( OmxilEnc_t *encH );

OMX_ERRORTYPE ResumeVenc( OmxilEnc_t *encH );
OMX_ERRORTYPE FlushVenc( OmxilEnc_t *encH );

#endif // OMXIL_VENC_SESSION_H_





