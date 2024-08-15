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

#ifndef OMXIL_VDEC_SESSION_H_
#define OMXIL_VDEC_SESSION_H_

#include <thread>
#include <stdbool.h>
#include <queue>

#include <screen/screen.h>

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <sys/slogcodes.h>
#include "input.h"


#define VDEC_DISP_OUTPUT_BUFFER_NUM 12 /* Needs to be same as NUM_DISP_OUT_BUFFERS (omxil_component_dec.c) */
#define VDEC_INPUT_BUFFER_NUM 2  /* Needs to be same as NUM_IN_BUFFERS (omxil_component_dec.c) */

typedef enum OmxilBailReason {
    BAIL_NOT_BAILED,
    BAIL_EOS,
    BAIL_ERROR,
    BAIL_INIT_FAIL,
    BAIL_DEC_CATCH
}OmxilBailReason;

typedef struct {
    void *addr;
    uint32_t size;
    uint64_t offset;
    screen_buffer_t screen_buf;
} OmxilDecOutputBuffer_t;


class OmxilVideoDec {
public:
    OmxilVideoDec(const char *, const char *, int, int, bool, int, int, int, int);
    ~OmxilVideoDec();

    OMX_ERRORTYPE InitDecComp();
    void CloseVdec();

    OMX_ERRORTYPE StartVdec();
    OMX_ERRORTYPE StopVdec();

    OMX_ERRORTYPE PauseVdec();

    OMX_ERRORTYPE ResumeVdec();
    OMX_ERRORTYPE FlushVdec();
    OmxilBailReason GetBailReason();
    OMX_ERRORTYPE ProcessEvent(
            OMX_HANDLETYPE hComponent,
            OMX_EVENTTYPE eEvent,
            OMX_U32 nData1,
            OMX_U32 nData2,
            OMX_PTR pEventData );
    OMX_ERRORTYPE ProcessEmptyBufferDone(
            OMX_HANDLETYPE hComponent,
            OMX_BUFFERHEADERTYPE *pBufHdr );
    OMX_ERRORTYPE ProcessFillBufferDone(
            OMX_HANDLETYPE hComponent,
            OMX_BUFFERHEADERTYPE *pBufHdr );

private:

    void saveOutputNV12(OmxilDecOutputBuffer_t *buf);
    void saveOutputNV16(OmxilDecOutputBuffer_t *buf);
    void decoder_file_push_thread();

    void timedwait(const char *caller );
    OMX_ERRORTYPE waitForCommandComplete();
    OMX_ERRORTYPE AllocatePortBuffers();
    OMX_ERRORTYPE FreeInPortBuffers();
    OMX_ERRORTYPE FreeOutPortBuffers();
    OMX_ERRORTYPE MoveToState( OMX_STATETYPE newState );

    OMX_BUFFERHEADERTYPE* GetInputFrame();
    OMX_ERRORTYPE send_config_data();
    OMX_ERRORTYPE ReconfigVdecSession();
    void init();

    std::queue<OMX_BUFFERHEADERTYPE*> qInputBufHdr;
    std::queue<OMX_BUFFERHEADERTYPE*> qOutputBufHdr;
    /* File paths*/
    const char *in_path;
    const char *out_path;
    int         out_fd;
    std::unique_ptr<OmxilVideoDecInput> mInput;

    screen_context_t screen_ctx;
    screen_window_t  screen_win;
    screen_buffer_t  *screen_buf;
    OmxilDecOutputBuffer_t *output_bufs;
    OMX_U32          output_buf_num;
    OmxilDecOutputBuffer_t *input_bufs;
    OMX_U32          input_buf_num;

    /* Threads */
    std::thread decoder_push;
    /* when using bufferqueue output, this enables
     * the decoded data to be shown on screen too
     */
    int frame_rate;
    int src_width;
    int src_height;
    int aligned_width;
    int aligned_height;
    int stride;

    //component
    OMX_HANDLETYPE compHandle;
    OMX_U32        inPortIndex;
    OMX_U32        outPortIndex;
    uint16_t numOfPorts; // Data ports
    bool           cmdComplete;
    bool           inPortFlushed;
    bool           outPortFlushed;
    bool           post_to_screen;
    bool           eos_received;
    bool           eos_sent;
    bool           portSettingChanged;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    OMX_ERRORTYPE  compError;
    OMX_COLOR_FORMATTYPE outputFormat;
    int core_idx;
    int instance;
    int opformat;
    int error_conceal;
    int dec_buf_num;

    //output port
    OMX_U32 outputPortBufSize;
    OMX_U32 nOutputBufs;
    int     nOutFrameCount;

    //Input port
    OMX_U32 nInputBufs;
    OMX_U32 inputPortBufSize;
    int     nInFrameCount;
    OMX_TICKS frame_ts;

    bool     exit_thread;
    bool     thread_running;
    static OMX_CALLBACKTYPE callbacks;

    uint64_t startTime;
    uint64_t stopTime;
};


#endif // OMXIL_VDEC_SESSION_H_
