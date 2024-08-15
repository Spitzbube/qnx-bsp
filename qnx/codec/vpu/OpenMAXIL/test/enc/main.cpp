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

/* System libraries */
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <termios.h>
#include <pthread.h>
#include <stdbool.h>
#include <inttypes.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <sys/procmgr.h>
#include <screen/screen.h>
#include <sys/slog2.h>
#include <sys/slogcodes.h>
#include <queue>
#include <iostream>
#include <stddef.h>
#include <sys/mman.h>

#include "omxil.h"

#define ALIGN64(X)  (((X)+63) &~63)
#define ALIGN32(X)  (((X)+31) &~31)
#define ALIGN16(X)  (((X)+15) &~15)
#define ALIGN8(X)   (((X)+7) &~7)

int g_log_lvl = 0;
pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;

static enc_params_t enc_params[] =
{
    {"bitrate", ( int32_t* ) offsetof(OmxilEnc_t, bitrate), {0,INT32_MAX}},
    {"idr_period", ( int32_t* ) offsetof(OmxilEnc_t, idr_period), {0, 240}},
    {"rcmode", ( int32_t* ) offsetof(OmxilEnc_t, rcmode), {0,2}},
    {0,0,{0,0}}
};


/* JB: Adding defaults */
#define DEFAULT_BITRATE             2000000
#define DEFAULT_IDR_PERIOD          30
#define DEFAULT_RATE_CONTROL_MODE   OMX_Video_ControlRateVariable  //Other option: OMX_Video_ControlRateConstant

static OMX_BUFFERHEADERTYPE* GetInputFrame(OmxilEnc_t *encH)
{
    OMX_BUFFERHEADERTYPE *rbuf = NULL;
    int line;
    uint8_t *waddr;

    if(encH->qInputBufHdr.empty()) {
        AO_LOG(AO_LOG_DEBUG2,"GetInputFrame no input buffer available");
        return NULL;
    }
    rbuf = encH->qInputBufHdr.front();

    //read Y
    waddr = (uint8_t*)rbuf->pAppPrivate;
    for(line = 0; line < encH->src_height; line++){
        int r = read(encH->in_fd,
                (waddr + (encH->src_stride * line)),
                encH->src_width);
        if(r != encH->src_width) {
            AO_LOG(AO_LOG_INFO,"GetInputFrame end of %s", encH->in_path);
            rbuf->nFlags = OMX_BUFFERFLAG_EOS;
            goto exit_eos;
        }
    }
    //read UV
    waddr = (uint8_t*)rbuf->pAppPrivate + encH->src_stride * encH->aligned_height;
    for(line = 0; line < encH->src_height/2; line++){
        int r = read(encH->in_fd,
                (waddr + (encH->src_stride * line)),
                encH->src_width);
        if(r != encH->src_width) {
            AO_LOG(AO_LOG_INFO,"GetInputFrame end of %s", encH->in_path);
            rbuf->nFlags = OMX_BUFFERFLAG_EOS;
            goto exit_eos;
        }
    }

    rbuf->nFilledLen = rbuf->nAllocLen;
    encH->qInputBufHdr.pop();
    return rbuf;
exit_eos:
    rbuf->nFilledLen = 0;
    encH->qInputBufHdr.pop();
    return rbuf;
}

/**
 * encoder_file_push_thread
 * Description: Pulls buffers out from the decoder output queue and pushes into
 *              the encoder input queue.
 *              When DEBUG_ON is defined, this function will also post those
 *              frames to screen (the default display).
 * Arguments:
 *   args: The OmxilEnc_t structure.
 *   itemsLength: Size of the qnx buffer items (that we gave)
 * Returns:
 *   OMX_ErrorNone  on success
 *   OMX_ERRORTYPE type code on failure
 */
static void *encoder_file_push_thread(void *args)
{
    pthread_setname_np(pthread_self(), "encoder_file_push_thread");
    AO_LOG(AO_LOG_DEBUG2,"encoder_file_push_thread(%d) has started \n", gettid());

    OmxilEnc_t *encH = (OmxilEnc_t *)args;
    OMX_ERRORTYPE omxErr;

    for (;;) {
        pthread_mutex_lock(&encH->mutex);
        if (encH->compError != OMX_ErrorNone || encH->eos_received) {
            AO_LOG(AO_LOG_DEBUG2,"encoder_player_push_thread bailed or eos(%d) at %d\n",encH->eos_received, __LINE__);
            pthread_cond_signal(&g_cond);
            pthread_mutex_unlock(&encH->mutex);
            break;
        }

        while(!(encH->qOutputBufHdr.empty()))
        {
            OMX_BUFFERHEADERTYPE *oBufHdr = encH->qOutputBufHdr.front();
            omxErr = OMX_FillThisBuffer( encH->compHandle, oBufHdr);
            if( omxErr != OMX_ErrorNone ) {
                AO_LOG( AO_LOG_ERROR, "OmxilEnc=> OMX_FillThisBuffer: return omxErr=%08x", omxErr );
                break;
            }
            encH->qOutputBufHdr.pop();
        }
        OMX_BUFFERHEADERTYPE *buffer = NULL;
        if(!encH->eos_sent)
            buffer = GetInputFrame(encH);

        if (buffer) {
            if(encH->user_request_exit) {
                AO_LOG(AO_LOG_INFO,"encoder_file_push_thread send eos");
                buffer->nFlags |= OMX_BUFFERFLAG_EOS;
            }

            omxErr = OMX_EmptyThisBuffer( encH->compHandle, buffer);
            if( omxErr != OMX_ErrorNone ) {
                AO_LOG( AO_LOG_ERROR, "OmxilEnc=> OMX_EmptyThisBuffer: return omxErr=%08x", omxErr );
                pthread_mutex_unlock(&encH->mutex);
                break;
            }
            if(buffer->nFlags & OMX_BUFFERFLAG_EOS)
                encH->eos_sent = true;
        } else {
            AO_LOG(AO_LOG_DEBUG2,"Wait for input buffer or eos from codec(eos_sent=%d))!", encH->eos_sent);
            pthread_cond_wait( &encH->cond, &encH->mutex );
        }
        pthread_mutex_unlock(&encH->mutex);
    }
    AO_LOG(AO_LOG_DEBUG2,"encoder_file_push_thread has stopped");
    return NULL;
}


/**
 * get_display_info
 * Description: Gets some information about display, and intialize some screen
 *              variables inside OmxilEnc_t structure.
 * Arguments:
 *   encH: The OpenMAX IL Objects structure
 * Returns:
 *   OMX_ErrorNone     on success
 *   OMX_ERRORTYPE type code    on failure
 */

static OMX_ERRORTYPE get_display_info(OmxilEnc_t *encH)
{
    AO_LOG(AO_LOG_DEBUG2,"Getting display information");

    int size[] = { 0, 0 };
    /* Find the display of interest */
    AO_LOG(AO_LOG_DEBUG2,"Getting display count");
    int num_displays;
    if (screen_get_context_property_iv(encH->screen_ctx, SCREEN_PROPERTY_DISPLAY_COUNT, &num_displays) != 0) {
        AO_LOG(AO_LOG_ERROR,"Failed to get number of displays! Error: %s", strerror(errno));
        return OMX_ErrorInsufficientResources;
    }
    AO_LOG(AO_LOG_DEBUG2,"Mallocing and getting displays");
    screen_display_t *displays = (screen_display_t *)malloc(num_displays * sizeof(screen_display_t));
    if (displays == NULL) {
        AO_LOG(AO_LOG_ERROR,"Failed to allocate screen display structure! Error: %s", strerror(errno));
        return OMX_ErrorInsufficientResources;
    }
    if (screen_get_context_property_pv(encH->screen_ctx, SCREEN_PROPERTY_DISPLAYS, (void **)displays) != 0) {
        AO_LOG(AO_LOG_ERROR,"Failed to get number of displays! Error: %s", strerror(errno));
        free(displays);
        return OMX_ErrorInsufficientResources;
    }

    encH->display = displays[0];
    if (encH->display == NULL) {
        AO_LOG(AO_LOG_ERROR,"Failed to get default display! Error: %s", strerror(errno));
        free(displays);
        return OMX_ErrorUndefined;
    }

    AO_LOG(AO_LOG_DEBUG2,"Getting display size");
    if (screen_get_display_property_iv(encH->display, SCREEN_PROPERTY_SIZE, size) != 0) {
        AO_LOG(AO_LOG_ERROR,"Failed to get screen size! Error: %s", strerror(errno));
        free(displays);
        return OMX_ErrorInsufficientResources;
    }

    AO_LOG(AO_LOG_DEBUG2,"Screen width %d, height %d", size[0], size[1]);
    if (size[0] <= 0) {
        AO_LOG(AO_LOG_ERROR,"Invalid screen width!");
        free(displays);
        return OMX_ErrorBadParameter;
    }
    if (size[1] <= 0) {
        AO_LOG(AO_LOG_ERROR,"Invalid screen height!");
        free(displays);
        return OMX_ErrorBadParameter;
    }

    free(displays);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE omxil_init_display(OmxilEnc_t *encH)
{
    AO_LOG(AO_LOG_DEBUG2,"Creating screen context");
    if (screen_create_context(&(encH->screen_ctx), SCREEN_DISPLAY_MANAGER_CONTEXT) != 0) {
        AO_LOG(AO_LOG_ERROR,"Failed to create screen context! Error: %s", strerror(errno));
        return OMX_ErrorInsufficientResources;
    }

    OMX_ERRORTYPE res = get_display_info(encH);
    if (res != OMX_ErrorNone) {
        return res;
    }

    AO_LOG(AO_LOG_DEBUG2,"Creating screen window");
    if (screen_create_window(&(encH->screen_win), encH->screen_ctx) != 0) {
        AO_LOG(AO_LOG_ERROR,"Failed to create screen window! Error: %s", strerror(errno));
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE omxil_init(OmxilEnc_t *encH)
{

    encH->in_fd = open(encH->in_path, O_RDONLY);
    if(encH->in_fd == -1) {
        AO_LOG(AO_LOG_ERROR,"Error: unable to open input file (%s)", encH->in_path);
        return OMX_ErrorInsufficientResources;
    }

    encH->out_fd = open(encH->out_path, O_WRONLY | O_CREAT, 0644);
    if(encH->out_fd == -1) {
        AO_LOG(AO_LOG_ERROR,"Error: unable to open output file (%s)", encH->out_path);
        return OMX_ErrorInsufficientResources;
    }

#if defined (USE_SCREEN)
    if (encH->post_to_screen == true) {
        int v;
        screen_buffer_t screen_buf[VENC_INPUT_BUFFER_NUM];

        encH->aligned_height = ALIGN16(encH->src_height);
        int size[] = { ALIGN64(encH->src_width), encH->aligned_height};

        if (screen_set_window_property_iv(encH->screen_win, SCREEN_PROPERTY_SOURCE_SIZE, size) != 0) {
            AO_LOG(AO_LOG_ERROR,"Failed to set source dimensions for window! Error: %s", strerror(errno));
            return OMX_ErrorBadParameter;
        }

        if (screen_set_window_property_iv(encH->screen_win, SCREEN_PROPERTY_BUFFER_SIZE, size) < 0) {
            AO_LOG(AO_LOG_ERROR,"Failed to set SCREEN_PROPERTY_BUFFER_SIZE for window! Error: %s", strerror(errno));
            return OMX_ErrorBadParameter;
        }

        v = SCREEN_USAGE_READ | SCREEN_USAGE_WRITE | SCREEN_USAGE_NATIVE;
        if (screen_set_window_property_iv(encH->screen_win, SCREEN_PROPERTY_USAGE, &v) < 0) {
            AO_LOG(AO_LOG_ERROR,"Failed to set SCREEN_PROPERTY_USAGE for window! Error: %s", strerror(errno));
            return OMX_ErrorBadParameter;
        }

        v = encH->input_format;
        if (screen_set_window_property_iv(encH->screen_win, SCREEN_PROPERTY_FORMAT, &v) < 0) {
            AO_LOG(AO_LOG_ERROR,"Failed to set SCREEN_PROPERTY_FORMAT for window! Error: %s", strerror(errno));
            return OMX_ErrorBadParameter;
        }

        v = SCREEN_TRANSPARENCY_NONE;
        if (screen_set_window_property_iv(encH->screen_win, SCREEN_PROPERTY_TRANSPARENCY, &v) < 0) {
            AO_LOG(AO_LOG_ERROR,"Failed to set SCREEN_PROPERTY_TRANSPARENCY for window! Error: %s", strerror(errno));
            return OMX_ErrorBadParameter;
        }

        if (screen_set_window_property_pv(encH->screen_win, SCREEN_PROPERTY_DISPLAY, (void **)&encH->display) < 0) {
            AO_LOG(AO_LOG_ERROR,"Failed to set SCREEN_PROPERTY_DISPLAY for window! Error: %s", strerror(errno));
            return OMX_ErrorBadParameter;
        }

        if (screen_create_window_buffers(encH->screen_win, VENC_INPUT_BUFFER_NUM) < 0) {
            AO_LOG(AO_LOG_ERROR,"Failed to create window buffers for window! Error: %s", strerror(errno));
            return OMX_ErrorBadParameter;
        }

        if (screen_get_window_property_pv(encH->screen_win, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)&screen_buf) < 0) {
            AO_LOG(AO_LOG_ERROR,"Failed to get SCREEN_PROPERTY_RENDER_BUFFERS for window! Error: %s", strerror(errno));
            return OMX_ErrorBadParameter;
        }

        if (screen_get_buffer_property_iv(screen_buf[0], SCREEN_PROPERTY_STRIDE, &encH->src_stride)) {
            AO_LOG(AO_LOG_ERROR,"Failed to get SCREEN_PROPERTY_STRIDE Error: %s",strerror(errno));
            return OMX_ErrorBadParameter;
        }

        AO_LOG(AO_LOG_INFO,"Input stride is %d", encH->src_stride);

        for (int i = 0; i < VENC_INPUT_BUFFER_NUM; i++) {
            void *pointer = NULL;
            int frame_size;
            long long offset;

            if (screen_get_buffer_property_pv(screen_buf[i], SCREEN_PROPERTY_POINTER, &pointer) != EOK) {
                AO_LOG(AO_LOG_ERROR,"Failed to get SCREEN_PROPERTY_POINTER for buf[%d]! Error: %s",i, strerror(errno));
                return OMX_ErrorBadParameter;
            }

            if (screen_get_buffer_property_llv(screen_buf[i], SCREEN_PROPERTY_PHYSICAL_ADDRESS,
                                               &offset) < 0) {
                AO_LOG(AO_LOG_ERROR,"Failed to get SCREEN_PROPERTY_POINTER for buf[%d]! Error: %s",i, strerror(errno));
                return OMX_ErrorBadParameter;
            }

            if (screen_get_buffer_property_iv(screen_buf[i], SCREEN_PROPERTY_SIZE, &frame_size)) {
                AO_LOG(AO_LOG_ERROR,"Failed to get SCREEN_PROPERTY_POINTER for buf[%d]! Error: %s",i, strerror(errno));
                return OMX_ErrorBadParameter;
            }
            encH->input_bufs[i].addr = pointer;
            encH->input_bufs[i].size = frame_size;
            encH->input_bufs[i].offset = offset;
            //encH->input_buf_q.push(&encH->input_bufs[i]);
            AO_LOG(AO_LOG_DEBUG2,"Get frame size for buf[%d]: %d, %p, %llx",i, frame_size, pointer, offset);
        }


        v = 1;
        if (screen_set_window_property_iv(encH->screen_win, SCREEN_PROPERTY_VISIBLE, &v) < 0) {
            AO_LOG(AO_LOG_ERROR,"Failed to set SCREEN_PROPERTY_VISIBLE for window! Error: %s", strerror(errno));
            return OMX_ErrorBadParameter;
        }
    }
    else
#endif
    {
        encH->aligned_height = ALIGN8(encH->src_height);
        encH->src_stride = ALIGN32(encH->src_width);
        AO_LOG(AO_LOG_INFO,"Input stride is %d", encH->src_stride);
        for (int i = 0; i < VENC_INPUT_BUFFER_NUM; i++) {
            int frame_size;

            frame_size = encH->src_stride * encH->aligned_height * 3 / 2;
            encH->input_bufs[i].size = frame_size;
            AO_LOG(AO_LOG_INFO,"Get frame size for buf[%d]: %d", i, frame_size);
        }
    }


    if((InitEncComp(encH)) != OMX_ErrorNone) {
        AO_LOG(AO_LOG_ERROR,"Failed to create encoder:");
        return OMX_ErrorInsufficientResources;
    }
    /* We're not going to register a buffer callback for our encoder,
     * having a seperate thread to push buffers is faster.
     * We'll pull buffers from the decoder output queue and push it into the
     * encoder input queue.
     */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&encH->encoder_push, &attr, encoder_file_push_thread, (void *)encH);
    pthread_attr_destroy(&attr);

    return OMX_ErrorNone;
}

static int32_t parse_input_format(const char *sformat, OmxilEnc_t* hdl)
{
    int n;
    int ret = EOK;

    char *res = strchr(sformat, ',');
    if(res == NULL) {
        AO_LOG(AO_LOG_ERROR, "%s:%d failed to parse input format(%s)", __func__, __LINE__, sformat);
        return EINVAL;
    }

    n = res - sformat;
    if(n != 4) {
        AO_LOG(AO_LOG_ERROR, "%s:%d failed to parse input format(%s)", __func__, __LINE__, sformat);
        return EINVAL;
    }

    if (sscanf(res + 1, "%dx%d@%d", &hdl->src_width, &hdl->src_height,
                &hdl->frame_rate) != 3) {
        AO_LOG(AO_LOG_ERROR, "%s:%d failed to parse input format(%s)", __func__, __LINE__, sformat);
        return EINVAL;
    }

    if(hdl->src_width == 0 || hdl->src_height == 0 || hdl->frame_rate ==0) {
        AO_LOG(AO_LOG_ERROR, "%s:%d Invalid input format", __func__, __LINE__);
        return EINVAL;
    }

    if (!strncasecmp(sformat, "nv12", n)) {
        hdl->input_format = SCREEN_FORMAT_NV12;
    }
    else {
        ret = EINVAL;
    }

    return ret;
}

static char *ltrim( char *str ) {
	while ( isspace( (unsigned char) *str ) ) {
		++str;
	}
	return str;
}

static size_t rtrim( char *str ) {
	char *p = str + strlen( str );
	while ( p != str && isspace( (unsigned char) *--p ) ) {
		*p = '\0';
	}
	return p-str;
}

static void set_default_enc_params(OmxilEnc_t* hdl)
{
    hdl->bitrate = DEFAULT_BITRATE;
    hdl->idr_period = DEFAULT_IDR_PERIOD;
    hdl->rcmode = (int32_t)DEFAULT_RATE_CONTROL_MODE;

}

static int32_t config_load(OmxilEnc_t* hdl)
{
    int32_t ret = EOK;
    FILE  *fp;

    if ( ( fp = fopen( hdl->conf_path, "rt" ) ) == NULL ) {
        AO_LOG(AO_LOG_ERROR, "Unable to open \"%s\" (%s).", hdl->conf_path, strerror(errno) );
        return EIO;
    }

    enc_params_t *p = enc_params;
    while( p->name ) {
        char *ph = ( char* ) hdl;
        p->value = ( int32_t* ) ( &ph[(uintptr_t) p->value] );
        p++;
    }

    char lbuf[ LINE_MAX+1 ];
    unsigned lnum = 0;
    while ( fgets( lbuf, sizeof(lbuf), fp ) ) {
        char *line = ltrim( lbuf );
        char ltype = line[0];
        ++lnum;
        if ( ltype != '\0' && ltype != '#' ) {
            rtrim( line );
            char *val = strchr( line, '=' );
            if ( val != NULL ) {
                *val = '\0';
                val = ltrim( val+1 );
                rtrim( line );
                if ( line[0] != '\0' ) {
                    uint32_t n;
                    for( n = 0; n < ( sizeof( enc_params) / sizeof(enc_params_t) - 1); n++ ) {
                        if( strcmp( enc_params[n].name, line ) == 0 ) {
                            int32_t v = strtol(val, NULL, 0);
                            int32_t *mm = ( int32_t * ) enc_params[n].range;

                            if( v < mm[0] ) {
                                AO_LOG(AO_LOG_WARNING, "%s:%d param %s:%s out of range, set it to %d", __func__, __LINE__, line, val, mm[0] );
                                v = mm[0];
                            }
                            if( v > mm[1] ) {
                                AO_LOG(AO_LOG_WARNING, "%s:%d param %s:%s out of range, set it to %d", __func__, __LINE__, line, val, mm[1] );
                                v = mm[1];
                            }

                            *( enc_params[n].value ) = v;
                            break;
                        }
                    }
                    if(n == ( sizeof( enc_params) / sizeof(enc_params_t) - 1)) {
                        AO_LOG(AO_LOG_WARNING, "%s:%d configure parameter name not recognized", __func__, __LINE__);
                    }
                }
                else {
                    AO_LOG(AO_LOG_ERROR, "%s:%d Error: invalid configure line: %s", __func__, __LINE__, val);
                    ret = EINVAL;
                    break;
                }
            }
            else {
                AO_LOG(AO_LOG_ERROR, "%s:%d Error: configure parameter(%s) not recognized", __func__, __LINE__, line );
                ret = EINVAL;
                break;
            }
        }
    }
    fclose( fp );

    return ret;

}

static void print_usage_and_exit(const char *argv0)
{
    printf("Usage: %s [options] \n"
           "  Command line options:\n"
           "    -C: VPU Core to choose (0 only for j721s2, 0,1 for j784s4)\n"
           "    -v: increase verbosity, max 7\n"
           "    -n: not to use stdin\n"
           "    -i: input file\n"
           "    -o: output file\n"
           "    -s: coding standard (0 = AVC, 1 = HEVC)\n"
           "    -c: config file for enc parameters\n"
           "    -d: disable display\n"
           "    -a multi instance mode:\n"
           "        0 or no option - Use the default memory layout\n"
           "        1 or 2 - Use either one of the memory layout\n"
           "    -f input format for raw input (.yuv/.rgba/etc.)\n"
           "       e.g.: nv12,1920x1080@30\n"
           "             nv12 is the input color format\n"
           "             1920x1080 is resolution(width x height)\n"
           "             30 is frame rate.\n"
           "  Supported input format: nv12.\n"
           "    -L: Enable lossless encoding\n"
           "    -G: Select GOP preset\n"
           "        0 for custom_GOP (default / user defined structure)\n"
           "        1 for all I frames\n"
           "        9 for consecutive P frames, with single reference I frame\n",
           argv0
           );
    exit(EXIT_FAILURE);
}


static void validate_core_idx(int idx, const char *argv0)
{

    /* Default max_vpu_idx is 1 as that of J721S2 */
    int max_vpu_idx = 1;

#if defined(SOC_J784S4)
    max_vpu_idx = 2;
#endif

    if(idx < 0 || idx >= max_vpu_idx) {
        printf("passed invalid index %d\n", idx);
        print_usage_and_exit(argv0);
    }

}


static void bg_handler(int signo)
{
    AO_LOG( AO_LOG_ERROR, "%s: signal %d received ", __func__, signo);
    AO_LOG( AO_LOG_ERROR, "%s: Please run with -n to not using stdin", __func__);
    _exit(0);
}

int main(int argc, char **argv)
{

    OMX_ERRORTYPE res = OMX_ErrorNone;
    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);

    if (argc < 4) {
        print_usage_and_exit(argv[0]);
    }

    OmxilEnc_t encH = { 0 };
    encH.in_fd = -1;
    encH.out_fd = -1;
    encH.post_to_screen = true;
    encH.core_idx = 0;
    encH.use_alt_mem = 0;

    int opt;
    int ret = 0;
    while ((opt = getopt(argc, argv, "i:o:s:c:f:C:a:G:Ldvn")) != -1) {
        switch (opt) {
        case 'd':
            encH.post_to_screen = false;
            AO_LOG(AO_LOG_DEBUG2,"Not posting to screen");
            break;
        case 'i':
            encH.in_path = optarg;
            break;
        case 'a':
            encH.use_alt_mem = atoi(optarg);
        case 'o':
            encH.out_path = optarg;
            break;
        case 's':
            encH.coding_std = ((strchr(optarg,'1') == NULL) ? 0 : 1);
            break;
        case 'c':
            encH.conf_path = optarg;
            break;
        case 'f':
            if(parse_input_format(optarg, &encH) != EOK) {
                print_usage_and_exit(argv[0]);
            }
            break;
        case 'v':
            g_log_lvl++;
            break;
        case 'n':
            encH.no_stdin = true;
            break;
        case 'L':
            encH.setLossless = 1;
            AO_LOG( AO_LOG_INFO, "set lossless flag L\n");
            break;
        case 'C':
            encH.core_idx = atoi(optarg);
            validate_core_idx(encH.core_idx, argv[0]);
            break;
        case 'G':
            encH.setGOP = atoi(optarg);
            break;
        default:
            print_usage_and_exit(argv[0]);
        }
    }

    if(encH.in_path == NULL || encH.out_path == NULL) {
        print_usage_and_exit(argv[0]);
        return 0;
    }

    if (procmgr_ability(0,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_KEYDATA,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_IO,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_MEM_PHYS,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_PRIORITY,
                        PROCMGR_AOP_DENY  | PROCMGR_ADN_NONROOT | PROCMGR_AOP_LOCK      | PROCMGR_AID_EOL)
        != EOK) {
        AO_LOG(AO_LOG_ERROR, "Unable to gain procmgr abilities for nonroot operation.");
        return 0;
    }

    ThreadCtl( _NTO_TCTL_IO, 0);

    if(encH.conf_path != NULL) {
        if(config_load(&encH) != EOK) {
            return 0;
        }
    }
    else {
        set_default_enc_params(&encH);
    }

    if (encH.post_to_screen == true) {
        /*Initializing data structure for OpenMax Objects */
        res = omxil_init_display(&encH);
        if (res != OMX_ErrorNone) {
            printf("Initialization of display failed!\n"
                    "Cleaning up and quitting!\n");
            encH.compError = res;
            ret = BAIL_INIT_DISP_FAIL;
            goto bail;
        }
    }

    //thread lock for input/output port
    if(pthread_mutex_init( &encH.mutex, NULL) != EOK) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=>%s failure, mutex init err ", __func__ );
        ret = BAIL_MUTEX_INIT_ERROR;
        goto bail;
    }

    pthread_condattr_t attr;
    if (pthread_condattr_init(&attr) != EOK) {
        AO_LOG(AO_LOG_ERROR, "OmxilEnc=>%s failure, condattr init err", __func__);
        ret = BAIL_CONDATTR_INIT_ERROR;
        goto bail;
    }
    if (pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) != EOK) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=>%s failure, condattr_setclock err ", __func__ );
        pthread_condattr_destroy(&attr);
        ret = BAIL_CONDATTR_CLK_ERROR;
        goto bail;
    }

    if(pthread_cond_init( &encH.cond, &attr ) != EOK) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=>%s failure, cond init err ", __func__ );
        pthread_condattr_destroy(&attr);
        ret = BAIL_CONDATTR_INIT_ERROR;
        goto bail;
    }
    pthread_condattr_destroy(&attr);

    res = omxil_init(&encH);
    if (res != OMX_ErrorNone) {
        printf("Initialization of OpenMAX IL failed!\n"
                "Cleaning up and quitting!\n");
        encH.compError = res;
        ret = BAIL_INIT_FAIL;
        goto bail;
    }


    encH.frame_size = encH.src_stride * encH.aligned_height * 3 / 2;//default yuv420 format.

    struct termios new_termios;
    if(!encH.no_stdin) {
        struct sigaction act;
        act.sa_flags = 0;
        act.sa_handler = bg_handler;

        /*
         * Define a handler for SIGTTOU
         */
        sigaction( SIGTTOU, &act, NULL );

        /* Modifying terminal to allow key-press interaction */
        new_termios = orig_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO | ECHOCTL | ECHONL);
        new_termios.c_cflag |= HUPCL;
        new_termios.c_cc[VMIN] = 0;
        new_termios.c_cc[VTIME] = 10;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

        printf("Press 'p' to pause/resume.\n");
        printf("Press 'q' to quit.\n");

        for (;;) {
            pthread_mutex_lock(&encH.mutex);
            if(encH.user_request_exit) {
                if(encH.eos_received) {
                    AO_LOG(AO_LOG_INFO,"eos received: %d", __LINE__);
                    pthread_mutex_unlock(&encH.mutex);
                    break;
                }
                else{
                    AO_LOG(AO_LOG_INFO,"wait for eos: %d", __LINE__);
                    pthread_cond_wait(&encH.cond, &encH.mutex);
                    pthread_mutex_unlock(&encH.mutex);
                    continue;
                }
            }
            if(encH.eos_received) {
                AO_LOG(AO_LOG_INFO,"eos received: %d", __LINE__);
                pthread_mutex_unlock(&encH.mutex);
                break;
            }
            if (encH.compError != OMX_ErrorNone) {
                AO_LOG(AO_LOG_ERROR,"bailed: %d", __LINE__);
                pthread_mutex_unlock(&encH.mutex);
                ret = BAIL_ERROR;
                break;
            }
            pthread_mutex_unlock(&encH.mutex);
            int ch[8];
            int chnum = 0;

            chnum = read(STDIN_FILENO, ch, 8);

            if (chnum == 1) {
                switch (ch[0]) {
                    case 'p':
                    case ' ':
                        break;
                    case 'q':
                        pthread_mutex_lock(&encH.mutex);
                        encH.user_request_exit = true;
                        pthread_mutex_unlock(&encH.mutex);
                        AO_LOG(AO_LOG_INFO,"bailed at user request: %d", __LINE__);
                        break;
                    default:
                        printf("Unkown command [%x], valid commands are :\n"
                            "-----------------------------------------\n"
                            " p : pause/unpause playback.\n"
                            " q : stop playback and quit program.\n"
                            "-----------------------------------------\n",
                            ch[0]);
                }
            }
            else if (chnum ==0) {
                continue;
            }
        }
    }
    else {
        while(1) {
            pthread_mutex_lock(&encH.mutex);
            if(encH.eos_received || encH.compError != OMX_ErrorNone) {
                AO_LOG(AO_LOG_INFO,"eos received: %d", __LINE__);
                pthread_mutex_unlock(&encH.mutex);
                break;
            }
            else{
                AO_LOG(AO_LOG_INFO,"wait for eos: %d", __LINE__);
                pthread_cond_wait(&g_cond, &encH.mutex);
                pthread_mutex_unlock(&encH.mutex);
            }
        }
    }

bail:
    /* Terminate our buffer push thread before continuing */
    if (pthread_join(encH.encoder_push, NULL) != 0) {
        if (errno != ESRCH) {
            AO_LOG(AO_LOG_ERROR,"Failed to terminate encoder_player_push_thread! Error: %s", strerror(errno));
        }
    }

    if(encH.compHandle) {
        FlushVenc(&encH);
        CloseVenc(&encH);
    }

    pthread_mutex_destroy( &encH.mutex );
    pthread_cond_destroy( &encH.cond );

#if defined (USE_SCREEN)
    if (encH.screen_win) {
        screen_destroy_window(encH.screen_win);
    }

    if (encH.screen_ctx) {
        screen_destroy_context(encH.screen_ctx);
    }
#endif //#if defined (USE_SCREEN)


    if(encH.out_fd != -1)
        close(encH.out_fd != -1);
    if(encH.in_fd != -1)
        close(encH.in_fd);

    if(encH.output_bufs) {
        free(encH.output_bufs);
    }

    if(!encH.no_stdin) {
        /* Restore the terminal to its original state */
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }

    printf("OpenMAX IL enc done, exiting.\n");
    AO_LOG(AO_LOG_ERROR,"OpenMAX IL enc done, exiting.");

    return ret;
}

