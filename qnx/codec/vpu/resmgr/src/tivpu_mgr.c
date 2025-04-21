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

/*
 * Define THREAD_POOL_PARAM_T such that we can avoid a compiler
 * warning when we use the dispatch*() functions below
 */
#define THREAD_POOL_PARAM_T dispatch_context_t
#define DEV_SHMEM "/dev/shmemallocator"
#define VPU_RM_PER_MARKER _NTO_TRACE_USERFIRST+1
#include <sys/trace.h>
#include "tivpu_mgr.h"
#include "tivpu_mgr_priv.h"
#include "vpuapi.h"

static int g_log_level = _SLOG_INFO;
static int g_enable_polling = 0;
static uint8_t g_coreIdx = 0; // TODO: Is this needed

// Holds all the codec contexts this resource manager handles.
static tivpu_context_t g_codec_ctxs[MAX_CODEC_HANDLES];
static pthread_mutex_t g_instance_lock;
static pthread_mutex_t g_per_core_lock[MAX_NUM_VPU_CORE];

static char* g_firmware_path = CORE_6_BIT_CODE_FILE_PATH;


static int tivpu_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb);
static int tivpu_io_notify(resmgr_context_t *ctp, io_notify_t *msg, RESMGR_OCB_T *ocb);
static int tivpu_io_close_dup(resmgr_context_t *ctp, io_close_t* msg, RESMGR_OCB_T *ocb);
static int tivpu_cleanup_codec_instance(resmgr_context_t *ctp, RESMGR_OCB_T *i_ocb);

static void logtofile(int fd, struct timespec *ts, const char *fmt, ... )
{
    static char linebuf[ LINE_MAX + 2 ];
    size_t offs = 0;
    size_t space = sizeof linebuf;
    int    len;
    struct tm tm;
    if ( localtime_r( &ts->tv_sec, &tm ) != NULL
            && ( len = strftime( linebuf, sizeof linebuf, "%F %T", &tm ) ) > 0 ) {
        offs  += len;
        space -= len;
        len = snprintf( linebuf+offs, space, ".%03u ", (unsigned)( ts->tv_nsec / 1000000 ) );
        if ( (size_t) len < space ) {
            offs  += len;
            space -= len;
        }
    }

    len = snprintf( linebuf+offs, space, "[OpenMAXIL.] ");
    if ( (size_t) len < space ) {
        offs  += len;
        space -= len;
    }
    va_list arglist;
    va_start(arglist, fmt);
    if ( ( len = vsnprintf( linebuf+offs, space, fmt, arglist ) ) > 0 ) {
        if ( len < (int)space ) {
            offs += len;
        } else {
            // It's been truncated.
            offs = sizeof linebuf - 1;
        }
        linebuf[ offs++ ] = '\n';
        write(fd, linebuf, offs);
    }
    va_end(arglist);
}

#define PERFORMANCE_LOG(fd, ...) (logtofile(fd, __VA_ARGS__))

IOFUNC_OCB_T *
ocb_calloc (resmgr_context_t * ctp, IOFUNC_ATTR_T * device)
{
    ti_vpu_ocb_t *ocb = NULL;

    /* Allocate the OCB */
    ocb = (ti_vpu_ocb_t *) calloc (1, sizeof (ti_vpu_ocb_t));
    if (ocb == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    pthread_mutex_init(&ocb->lock, NULL);
    /* Initialize OCB */

    memset(&ocb->codec_stats, 0, sizeof(ocb->codec_stats));

    return (IOFUNC_OCB_T *)(ocb);
}

void
ocb_free (IOFUNC_OCB_T * ocb)
{
    ti_vpu_ocb_t *vpu_ocb = (ti_vpu_ocb_t *)ocb;

    if (vpu_ocb) {
        pthread_mutex_destroy(&vpu_ocb->lock);
        free (vpu_ocb);
    }
}

/**
 * VPU debug logger
 *
 * @param level Verbosity level
 * @param fmt format
 * @param ap Variadic argument list
 */
void vpu_qnx_logger(int level, const char *fmt, ...)
{
    va_list arglist;
    va_start(arglist, fmt);

    if (level <= g_log_level) {
        vslogf(_SLOG_SETCODE(_SLOGC_MEDIA, 2), level, fmt, arglist);
    }
    va_end(arglist);
}

static int32_t find_new_instance_slot()
{
    int32_t i = 0;

    pthread_mutex_lock(&g_instance_lock);
    for(i = 0; i < MAX_CODEC_HANDLES; i++) {
        if ((g_codec_ctxs[i].ch_id == MAX_CODEC_HANDLES)
             && (g_codec_ctxs[i].in_use == false)) {
                g_codec_ctxs[i].ch_id = i;
                g_codec_ctxs[i].in_use = true;
                break;
             }
    }
    pthread_mutex_unlock(&g_instance_lock);

    return i;
}




/**
 * Parse input options.
 *
 * @param argc Number of options.
 * @param argv Array of options.
 */
static int options(int argc, char *const argv[])
{
    int opt;
    int loglevel = g_log_level;

    while ((opt = getopt(argc, argv, "f:c:pv")) != -1) {
        switch (opt) {
            case 'c':
                g_coreIdx = atoi(optarg);
                if (g_coreIdx >= MAX_NUM_VPU_CORE) {
                    fprintf(stderr,"Core not supported - %c %d\n", opt, g_coreIdx);
                    return -1;
                }
                codec_sloginfo("%s: Core selected is %d\n", __func__ , g_coreIdx);
                break;
            case 'f':
                g_firmware_path = optarg;
                codec_sloginfo("%s: Firmware path is %s\n", __func__, g_firmware_path);
                break;
            case 'v':
                loglevel++;
                break;
            case 'p':
                g_enable_polling = 1;
                codec_sloginfo("%s: Enabling polling %d\n", __func__, g_enable_polling);
                break;
            default:
                fprintf(stderr,"Unsupported option '-%c'\n", opt);
                return -1;
        }
    }
    if(loglevel > 0)
        g_log_level = loglevel;

    return 0;
}

static int codec_debug_print_stats(ti_vpu_ocb_t *vpu_ocb)
{
    if(!vpu_ocb)
        return -1;

    codec_slogdbg("frame count is = %d", vpu_ocb->codec_stats.e_stats.enc_frame_cnt);

    return EOK;
}

static resmgr_connect_funcs_t  connect_funcs;
static resmgr_io_funcs_t       io_funcs;
static iofunc_attr_t           ioattr;
static iofunc_mount_t          mattr;
static iofunc_funcs_t          ocb_funcs;

int main(int argc, char *const argv[])
{
    struct stat             sbuf;
    int                     id;
    dispatch_t             *dpp;
    resmgr_attr_t           rattr;
    thread_pool_attr_t      tattr;
    thread_pool_t          *tpool;
    sigset_t                set;
    siginfo_t               info;
    int                     ret = 0;
    int32_t                 i = 0;

    /* Only allow one instance */
    if (-1 != stat(TIVPU_DEVICE_NAME, &sbuf)) {
        perror("VPU resmgr already running...");
        return (-1);
    }

    if (-1 == stat(DEV_SHMEM, &sbuf)) {
        perror("Shared memory allocator not available. Need to launch that first");
        codec_slogerr("Shared memory allocator not available. Need to launch that first");
        return (-1);
    }

    printf ("Starting VPU Codec resource manager...\n");
    codec_sloginfo("Starting VPU Codec resource manager...\n");


    if (-1 == options(argc, argv)) {
        return (-1);
    }

    ret = procmgr_ability(0,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_IO,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_MEM_PHYS,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_PRIORITY,
                        PROCMGR_AOP_DENY  | PROCMGR_ADN_NONROOT | PROCMGR_AOP_LOCK      | PROCMGR_AID_EOL);
    if(ret != EOK) {
        codec_slogerr("%s: Unable to gain procmgr abilities for nonroot operation", argv[0]);
        perror("Unable to gain procmgr abilities for nonroot operation");
        return ret;
    }

    /* Get IO priveleges */
    ret = ThreadCtl(_NTO_TCTL_IO, NULL);
    if(ret != EOK) {
        codec_slogerr("%s: ThreadCtl(_NTO_TCTL_IO) failed", argv[0]);
        perror("ThreadCtl(_NTO_TCTL_IO");
        return 1;
    }

    pthread_mutex_init(&g_instance_lock, NULL);
    for(i = 0; i < MAX_NUM_VPU_CORE; i++)
        pthread_mutex_init(&g_per_core_lock[i], NULL);


    for(i = 0; i < MAX_CODEC_HANDLES; i++) {
        g_codec_ctxs[i].ch_id = MAX_CODEC_HANDLES;
        g_codec_ctxs[i].in_use = false;
    }
    /* Init the vpu codecs */
    for(int idx = 0; idx < MAX_NUM_VPU_CORE; idx++) {
        codec_sloginfo("Starting VPU core %d\n", idx);
        ret = tivpu_codec_init(idx, g_firmware_path, g_enable_polling);
        if(ret != 0) {
            codec_slogerr("%s: VPU_Init for core %d failed", argv[0], idx);
            perror("VPU_Init failed");
            return ret;
        }
    }

    /*TODO: Need to see if we need 2 dispatchers, or threaded per core. */

    /* Initialize the dispatch interface */
    /*TODO: Should we de-init the codecs if any of the following calls fail */
    dpp = dispatch_create();
    if (!dpp) {
        codec_slogerr("%s: Failed to create dispatch interface", argv[0]);
        return (errno);
    }


    /*
     * Mask out all signals before creating a thread pool.
     * This prevents other threads in the thread pool
     * from intercepting signals such as SIGTERM.
     */
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    /* Initialize the thread pool */
    memset (&tattr, 0x00, sizeof(thread_pool_attr_t));
    tattr.handle = dpp;
    tattr.context_alloc = dispatch_context_alloc;
    tattr.context_free = dispatch_context_free;
    tattr.block_func = dispatch_block;
    tattr.unblock_func = dispatch_unblock;
    tattr.handler_func = dispatch_handler;
    tattr.lo_water = 2;
    tattr.hi_water = 8;
    tattr.increment = 1;
    tattr.maximum = 50;

    /* Initialize the resource manager attributes */
    memset(&rattr, 0, sizeof(rattr));
    rattr.nparts_max = 10;
    rattr.msg_max_size = 2048;

    memset (&mattr, 0, sizeof(iofunc_mount_t));
    mattr.flags = 0;
    mattr.conf = IOFUNC_PC_CHOWN_RESTRICTED | IOFUNC_PC_NO_TRUNC | IOFUNC_PC_SYNC_IO;
    mattr.dev = 0;
    mattr.funcs = &ocb_funcs;
    memset(&ocb_funcs, 0, sizeof(iofunc_funcs_t));
    ocb_funcs.nfuncs = _IOFUNC_NFUNCS;
    ocb_funcs.ocb_calloc = ocb_calloc;
    ocb_funcs.ocb_free = ocb_free;

    /* Initialize the connect functions */
    memset(&io_funcs, 0, sizeof(resmgr_io_funcs_t));
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS, &io_funcs);
    io_funcs.devctl = tivpu_io_devctl;
    io_funcs.notify = tivpu_io_notify;
    io_funcs.close_dup = tivpu_io_close_dup;

    iofunc_attr_init(&ioattr, S_IFNAM | 0777, NULL, NULL);
    ioattr.mount = &mattr;

    /* Attach the device name */
    id = resmgr_attach(dpp,
                       &rattr,
                       TIVPU_DEVICE_NAME,
                       _FTYPE_ANY,
                       0,
                       &connect_funcs,
                       &io_funcs,
                       &ioattr);
    if (id == -1) {
        codec_slogerr("%s: Failed to attach pathname", argv[0]);
        return (errno);
    }

    if ((tpool = thread_pool_create(&tattr, 0)) == NULL) {
        codec_slogerr("%s: thread pool create failed", argv[0]);
        return (errno);
    }

    /* Make this a daemon process */
    if (procmgr_daemon(EXIT_SUCCESS,
                       PROCMGR_DAEMON_NOCLOSE | PROCMGR_DAEMON_NODEVNULL ) == -1) {
        codec_slogerr("%s: procmgr_daemon", argv[0]);
        return (errno);
    }

    thread_pool_start(tpool);

    /* Unmasik signals to be caught */
    sigdelset (&set, SIGINT);
    sigdelset (&set, SIGTERM);
    pthread_sigmask (SIG_BLOCK, &set, NULL);

    /* Wait for one of these signals */
    sigemptyset (&set);
    sigaddset (&set, SIGINT);
    sigaddset (&set, SIGQUIT);
    sigaddset (&set, SIGTERM);

    printf("Resource Manager loop starting\n");
    while (1) {
        switch (SignalWaitinfo (&set, &info)) {
            case SIGTERM:
            case SIGQUIT:
            case SIGINT:
                ret = EOK;
                codec_slogerr("resource manager received signal %d", info.si_signo);
                goto done;

            default:
                break;
        }
    }
    ret = EOK;

done:
    codec_slogerr("%s: VPU resource manager exiting", argv[0]);

    ret = thread_pool_destroy(tpool);
    if (ret < 0) {
        codec_slogerr("%s: VPU thread_pool_destroy returned an error", argv[0]);
    }

    resmgr_detach(dpp, id, _RESMGR_DETACH_ALL);

    for(int idx = 0; idx < MAX_NUM_VPU_CORE; idx++) {
        int status = RETCODE_SUCCESS;
        if ((status = VPU_DeInit(idx)) != RETCODE_SUCCESS)
            codec_slogerr("Failed DeInit for VPU %d with status %d", idx, status);
    }
    return (ret);
}

static int tivpu_io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
    int     status, nbytes;
    int     err = EOK;
    ti_vpu_ocb_t * vpu_ocb = (ti_vpu_ocb_t *)ocb;
    static const char logenvname[] = "VPU_PERFORMANCE_LOG_DIR";
    char plog_path[128];

    if (vpu_ocb == NULL) {
        codec_slogerr("%s:%d Error: vpu_ocb is NULL", __FUNCTION__, __LINE__);
        return EINVAL;
    }

    if (msg == NULL) {
        codec_slogerr("%s:%d Error: msg is NULL", __FUNCTION__, __LINE__);
        return EINVAL;
    }

    if ((status = iofunc_devctl_default(ctp, msg, ocb)) != _RESMGR_DEFAULT) {
        codec_slogerr("%s:%d Error: iofunc_devctl_default failed with status %d", __FUNCTION__, __LINE__, status);
        return status;
    }

    pthread_mutex_lock(&vpu_ocb->lock);
    status = nbytes = 0;

    switch(msg->i.dcmd) {
        case DCMD_TIVPU_GET_PRODUCT_INFO:
            {

                TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *)(_DEVCTL_DATA (msg->i));
                cargs->status = VPU_GetProductInfo(cargs->args.vpuHwInfo.coreIdx,
                                             &cargs->args.vpuHwInfo.attr);
                break;
            }

        case DCMD_TIVPU_ENC_BUF_PREPARE:
           {
                //validate and ensure this is an encoder
                codec_slogdbg("devctl: DCMD_TIVPU_ENC_BUF_PREPARE");
                TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->i));
                vpu_buffer_t *bufs = (vpu_buffer_t *)(cargs+1);
                int32_t nbuf = cargs->args.prepareBuf.nbuf;
                uint8_t bufDir = cargs->args.prepareBuf.bufDir;
                tivpu_context_t *ti_codec_ctx = &(g_codec_ctxs[vpu_ocb->ch_id]);
                EncoderContext_t *ctx = ti_codec_ctx->codec_ctx;
                int coreIdx = ctx->encOpenParam.coreIdx;

                pthread_mutex_lock(&g_per_core_lock[coreIdx]);

                cargs->status = tivpu_enc_register_buffers(ti_codec_ctx, bufs, nbuf, bufDir);

                pthread_mutex_unlock(&g_per_core_lock[coreIdx]);

                break;
           }

        case DCMD_TIVPU_ENC_DESTROY:
           {
               TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->i));
               tivpu_context_t *ti_codec_ctx = &(g_codec_ctxs[vpu_ocb->ch_id]);
               EncoderContext_t *ctx = ti_codec_ctx->codec_ctx;
               int coreIdx = VPU_HANDLE_CORE_INDEX(((EncoderContext_t *)ctx)->handle);

               codec_slogdbg("devctl: DCMD_TIVPU_ENC_DESTROY");
               pthread_mutex_lock(&g_per_core_lock[coreIdx]);
               cargs->status = tivpu_cleanup_codec_instance(ctp, ocb);
               pthread_mutex_unlock(&g_per_core_lock[coreIdx]);

               break;
           }

        case DCMD_TIVPU_ENC_START:
        {
            //validate and ensure this is an encoder
            TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->i));

            codec_slogdbg("devctl: DCMD_TIVPU_ENC_START");

            cargs->status = tivpu_enc_start(&g_codec_ctxs[vpu_ocb->ch_id]);

            break;
        }

        case DCMD_TIVPU_ENC_STOP:
        {
            //validate and ensure this is an encoder
            TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->i));

            codec_slogdbg("devctl: DCMD_TIVPU_ENC_STOP");

            cargs->status = tivpu_enc_stop(&g_codec_ctxs[vpu_ocb->ch_id]);

            break;
        }
        case DCMD_TIVPU_ENC_PROCESS:
        {
            //validate and ensure this is an encoder
            TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->i));
            uint64_t enc_st_time = 0L;

            void * in_buf = (void *)(cargs->args.encodeFrameParams.ipBuf);
            void * out_buf = (void *)(cargs->args.encodeFrameParams.opBuf);
            void * op_first_buf = (void *)(cargs->args.encodeFrameParams.opfirstBuf);
            uint8_t is_eos = cargs->args.encodeFrameParams.isEos;
            tivpu_context_t *ti_codec_ctx = &(g_codec_ctxs[vpu_ocb->ch_id]);
            EncoderContext_t *ctx = ti_codec_ctx->codec_ctx;
            BOOL is_first = ctx->first;
            int coreIdx = VPU_HANDLE_CORE_INDEX(((EncoderContext_t *)ctx)->handle);

            codec_dbg_info_t info = {0};
            info.cnt = vpu_ocb->codec_stats.e_stats.enc_frame_cnt;
            info.pid = ctp->info.pid;

            if(vpu_ocb->plog_fd != -1) {
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                enc_st_time = (timespec2nsec( &ts ) / 1000LL);
                if(vpu_ocb->codec_stats.e_stats.enc_frame_cnt == 0) {
                    vpu_ocb->start_ts = enc_st_time;
                }

                if(is_first) {
                    PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "started encoding\n");
                    trace_logf(VPU_RM_PER_MARKER, "VPU_ENCODER MARKER: started encoding");
                }
            }


            /* This is a long running call. Unlock the ocb attributes. Ideally we want to put this in a queue
             * and serviced when the instance interrupt is servivced.
             */
            pthread_mutex_unlock(&vpu_ocb->lock);
            iofunc_unlock_ocb_default(ctp, msg, ocb);
            pthread_mutex_lock(&g_per_core_lock[coreIdx]);

            cargs->status = tivpu_enc_process(ti_codec_ctx, in_buf, out_buf, op_first_buf, is_eos, &info);

            pthread_mutex_unlock(&g_per_core_lock[coreIdx]);
            iofunc_lock_ocb_default(ctp, msg, ocb);

            pthread_mutex_lock(&vpu_ocb->lock);
            vpu_ocb->codec_stats.e_stats.enc_frame_cnt += 1;

            if(is_first)
                cargs->args.encodeFrameParams.encStatus.hdr_size = ctx->headerSize;

            cargs->args.encodeFrameParams.encStatus.out_size = ctx->outSize;
            cargs->args.encodeFrameParams.encStatus.is_first = is_first;


            if(vpu_ocb->plog_fd != -1) {
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
#if defined(DEBUG_MODE)
                uint64_t curr_time = (timespec2nsec( &ts ) / 1000LL);
                int frame_enc_time = (curr_time - enc_st_time)/1000;
                PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "Encoding latency for frame = %d ms\n", frame_enc_time);
                int coreIdx = VPU_HANDLE_CORE_INDEX(ctx->handle);
                trace_logf(VPU_RM_PER_MARKER, "core[%d] encoded frame %d", coreIdx , vpu_ocb->codec_stats.e_stats.enc_frame_cnt);
#endif
                if(is_eos == true) {
                    struct timespec ts;
                    clock_gettime(CLOCK_MONOTONIC, &ts);

                    uint64_t stop_ts = (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
                    uint64_t enc_time = (stop_ts - vpu_ocb->start_ts)/1000;
                    float enc_frame_rate = vpu_ocb->codec_stats.e_stats.enc_frame_cnt * 1000.0 / (float)enc_time;
                    PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "Total encoding time %lu ms.", enc_time);
                    PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "Number of encoded frames %d ", vpu_ocb->codec_stats.e_stats.enc_frame_cnt);
                    PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "Encoding frame rate is %.2f fps.", enc_frame_rate);
                    close(vpu_ocb->plog_fd);
                    vpu_ocb->plog_fd = -1;
                    trace_logf(VPU_RM_PER_MARKER, "VPU_ENCODER MARKER: finished encoding");
                }

            }
            SETIOV(&ctp->iov[0],&cargs, sizeof(TIVPU_CmdArgs));

            // check whether we should return here with return _RESMGR_NPARTS(1);
            break;
        }
        case DCMD_TIVPU_ENC_BUF_INFO:
        {
            //validate and ensure this is an encoder
            TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->o));
            tivpu_context_t *ti_codec_ctx = &(g_codec_ctxs[vpu_ocb->ch_id]);
            EncoderContext_t *ctx = ti_codec_ctx->codec_ctx;

            cargs->args.getBufInfo.nbuf = ctx->encOpenParam.streamBufCount;
            cargs->args.getBufInfo.streamBufSize = ctx->encOpenParam.streamBufSize;

            cargs->status = EOK;

            msg->o.nbytes = 0;
            msg->o.ret_val = EOK;

            SETIOV(&ctp->iov[0],&cargs, sizeof(TIVPU_CmdArgs));
            if( resmgr_msgwrite(ctp, &cargs, sizeof(*cargs), sizeof(msg->o)) < 0 ) {
                status = errno;
                codec_slogerr("failed to write reply structure");
            }

            break;

        }
        case DCMD_TIVPU_ENC_CREATE:
        {
            //validate and ensure this is an encoder
            EncoderContext_t **p_ctx = NULL;
            const char *env;

            if(vpu_ocb->created) {
                codec_slogerr("Only one instance per connection allowed");
                pthread_mutex_unlock(&vpu_ocb->lock);
                return (EINVAL);
            }
            codec_slogdbg("devctl: DCMD_TIVPU_ENC_CREATE");

            // Get a specific instance ID. Is this part of the codec instance, or should be just define our own?
            uint32_t c_id = find_new_instance_slot();

            TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *)(_DEVCTL_DATA (msg->i));
            tivpu_enc_config_t  *enc_config = &(cargs->args.cfg.enc_config);

            /* get a new token and assign it to the g_codec_ctxs[x].ch_id. Use that as the token to identify the ctx */
            if (c_id == MAX_CODEC_HANDLES) {
                codec_slogerr("MAX codec instance reached.");
                pthread_mutex_unlock(&vpu_ocb->lock);
                return (EBUSY);
            }

            //validate and ensure this is an encoder
            p_ctx = (EncoderContext_t **)(&(g_codec_ctxs[c_id].codec_ctx));

            if ((err = tivpu_enc_open_params((void **)p_ctx, enc_config)) != EOK) {
                codec_slogerr("tivpu_enc_open_params failed %d", err);
                break;
            }
            if ((err = tivpu_enc_src_buf_config(*p_ctx, enc_config)) != EOK) {
                codec_slogerr("tivpu_enc_src_buf_config %d", err);
                break;
            }
            if((err = tivpu_codec_get_product_info(&((*p_ctx)->attr), enc_config->coreIdx, &((*p_ctx)->cyclePerTick))) != EOK) {
                codec_slogerr("tivpu_codec_get_product_info %d", err);
                break;
            }

            // associate the ocb with the c_id. This will not work when someone does a dup of the handle. Need to check that later.
            vpu_ocb->ch_id = c_id;
            vpu_ocb->created = true;
            vpu_ocb->codec_stats.e_stats.enc_frame_cnt = 0;
            g_codec_ctxs[c_id].pid = ctp->info.pid;
            g_codec_ctxs[c_id].core_idx = enc_config->coreIdx;
            cargs->status = err;

           if ( ( env = getenv( logenvname ) ) != NULL ) {
               sprintf(plog_path, "%s/performance_log_%d_ch_%d.log", env, enc_config->coreIdx, c_id);
               vpu_ocb->plog_fd = open(plog_path, O_WRONLY | O_CREAT, 0644);
               if(vpu_ocb->plog_fd == -1)
                   codec_slogerr("Failed to open performance log file(%s).", plog_path);
           }
           else
               vpu_ocb->plog_fd = -1;

            break;
        }

#if VPU_FEATURE_OOB
        case DCMD_TIVPU_GETOUTBAND:
        {
            TIVPU_encGetOutBand_t io;

            break;
        }
#endif // VPU_FEATURE_OOB


        /******************************** DECODE DEVCTLS START HERE *****************************/
        case DCMD_TIVPU_DEC_CREATE:
        {
           //validate and ensure this is a decoder
           DecoderContext_t **p_ctx = NULL;
           const char *env;

           if(vpu_ocb->created) {
               codec_slogerr("Only one instance per connection allowed");
               pthread_mutex_unlock(&vpu_ocb->lock);
               return (EINVAL);
           }

           codec_slogdbg("devctl: DCMD_TIVPU_DEC_CREATE");

           // Get a specific instance ID. Is this part of the codec instance, or should be just define our own?
           uint32_t c_id = find_new_instance_slot(); 

           TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *)(_DEVCTL_DATA (msg->i));
           tivpu_dec_config_t  *dec_config = &(cargs->args.cfg.dec_config);

           /* get a new token and assign it to the g_codec_ctxs[x].ch_id. Use that as the token to identify the ctx */
           if (c_id == MAX_CODEC_HANDLES) {
                codec_slogerr("MAX codec instance reached.");
                pthread_mutex_unlock(&vpu_ocb->lock);
                return (EBUSY);
           }

           // set this context as a decoder
           g_codec_ctxs[c_id].is_dec = true;
           //validate and ensure this is an encoder
           p_ctx = (DecoderContext_t **) (&(g_codec_ctxs[c_id].codec_ctx));

           if ((err = tivpu_dec_open_params((void **)p_ctx, dec_config)) != EOK) {
               codec_slogerr("tivpu_dec_open_params failed %d", err);
               break;
           }
           if((err = tivpu_codec_get_product_info(&((*p_ctx)->attr), dec_config->coreIdx, &((*p_ctx)->cyclePerTick))) != EOK) {
               codec_slogerr("tivpu_codec_get_product_info %d", err);
               break;
           }

           // associate the ocb with the c_id. This will not work when someone does a dup of the handle. Need to check that later.
           vpu_ocb->ch_id = c_id;
           vpu_ocb->created = true;
           if ( ( env = getenv( logenvname ) ) != NULL ) {
               sprintf(plog_path, "%s/performance_log_%d_ch_%d.log", env, dec_config->coreIdx, c_id);
               vpu_ocb->plog_fd = open(plog_path, O_WRONLY | O_CREAT, 0644);
               if(vpu_ocb->plog_fd == -1)
                   codec_slogerr("Failed to open performance log file(%s).", plog_path);
           }
           else
               vpu_ocb->plog_fd = -1;

           vpu_ocb->codec_stats.d_stats.dec_frame_cnt = 0;
           g_codec_ctxs[c_id].pid = ctp->info.pid;
           g_codec_ctxs[c_id].core_idx = dec_config->coreIdx;
           cargs->status = err;

            break;
        }

        case DCMD_TIVPU_DEC_START:
        {
            //validate and ensure this is a decoder
            TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->i));
            DecoderContext_t *ctx = g_codec_ctxs[vpu_ocb->ch_id].codec_ctx;

            codec_slogdbg("devctl: DCMD_TIVPU_DEC_START");

            cargs->status = tivpu_dec_start(ctx);

            break;
        }

        case DCMD_TIVPU_DEC_STOP:
        {
            //validate and ensure this is a decoder
            TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->i));
            DecoderContext_t *ctx = g_codec_ctxs[vpu_ocb->ch_id].codec_ctx;

            codec_slogdbg("devctl: DCMD_TIVPU_DEC_STOP");

            cargs->status = tivpu_dec_stop(ctx);

            break;
        }

        case DCMD_TIVPU_DEC_DESTROY:
        {
            tivpu_context_t *ti_codec_ctx = &(g_codec_ctxs[vpu_ocb->ch_id]);
            DecoderContext_t *ctx = ti_codec_ctx->codec_ctx;
            int coreIdx = VPU_HANDLE_CORE_INDEX(((DecoderContext_t *)ctx)->handle);

            codec_slogdbg("devctl: DCMD_TIVPU_DEC_DESTROY");
            pthread_mutex_lock(&g_per_core_lock[coreIdx]);
            TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->i));
            cargs->status = tivpu_cleanup_codec_instance(ctp, ocb);
            pthread_mutex_unlock(&g_per_core_lock[coreIdx]);

            break;
        }

        case DCMD_TIVPU_DEC_BUF_PREPARE:
           {
                //validate and ensure this is a decoder
                codec_slogdbg("devctl: DCMD_TIVPU_DEC_BUF_PREPARE");
                TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->i));
                vpu_buffer_t *bufs = (vpu_buffer_t *)(cargs+1);
                int32_t nbuf = cargs->args.prepareBuf.nbuf;
                uint8_t bufDir = cargs->args.prepareBuf.bufDir;
                tivpu_context_t *ti_codec_ctx = &(g_codec_ctxs[vpu_ocb->ch_id]);
                DecoderContext_t *ctx = ti_codec_ctx->codec_ctx;
                int coreIdx = ctx->decOpenParam.coreIdx;

                pthread_mutex_lock(&g_per_core_lock[coreIdx]);

                cargs->status = tivpu_dec_register_buffers(ti_codec_ctx, bufs, nbuf, bufDir);

                pthread_mutex_unlock(&g_per_core_lock[coreIdx]);

                break;
           }
        case DCMD_TIVPU_DEC_PROCESS:
           {
               uint64_t dec_st_time = 0L;
                //validate and ensure this is a decoder
                codec_slogdbg("devctl: DCMD_TIVPU_DEC_PROCESS");

                TIVPU_CmdArgs *cargs = (TIVPU_CmdArgs *) (_DEVCTL_DATA(msg->i));

                void * in_buf = (void *)(cargs->args.decodeFrameParams.ipBuf);
                void * out_buf = (void *)(cargs->args.decodeFrameParams.opBuf);
                uint32_t in_filled_len = cargs->args.decodeFrameParams.inFilledLen;
                uint8_t is_eos = cargs->args.decodeFrameParams.isEos;
                vpu_dec_status_t *dec_status = &(cargs->args.decodeFrameParams.decStatus);
                tivpu_context_t *ti_codec_ctx = &(g_codec_ctxs[vpu_ocb->ch_id]);
                DecoderContext_t *ctx = ti_codec_ctx->codec_ctx;
                int coreIdx = VPU_HANDLE_CORE_INDEX(((DecoderContext_t *)ctx)->handle);

                if(vpu_ocb->plog_fd != -1) {

                    if (vpu_ocb->codec_stats.d_stats.dec_frame_cnt == 0) {
                        trace_logf(VPU_RM_PER_MARKER, "VPU_DECODER MARKER: started decoding");
                    }

                    struct timespec ts;
                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    dec_st_time = (timespec2nsec( &ts ) / 1000LL);
                    PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "[%d] start decoding frame\n", vpu_ocb->ch_id);
                    if(vpu_ocb->codec_stats.d_stats.dec_frame_cnt == 0) {
                        vpu_ocb->start_ts = dec_st_time;
                    }
                }

                /* This is a long running call. Unlock the ocb attributes */
                pthread_mutex_unlock(&vpu_ocb->lock);
                iofunc_unlock_ocb_default(ctp, msg, ocb);
                pthread_mutex_lock(&g_per_core_lock[coreIdx]);

                cargs->status = tivpu_dec_process(ti_codec_ctx, in_buf, out_buf, in_filled_len, is_eos, dec_status);

                pthread_mutex_unlock(&g_per_core_lock[coreIdx]);
                iofunc_lock_ocb_default(ctp, msg, ocb);
                pthread_mutex_lock(&vpu_ocb->lock);

                if(dec_status->displayed_frames >= 0)
                    vpu_ocb->codec_stats.d_stats.dec_frame_cnt++;

                codec_slogdbg("%s: dec_frame_cnt = %d", __func__, vpu_ocb->codec_stats.d_stats.dec_frame_cnt);

                if(vpu_ocb->plog_fd != -1) {
                    struct timespec ts;
                    clock_gettime(CLOCK_MONOTONIC, &ts);
                    uint64_t curr_time = (timespec2nsec( &ts ) / 1000LL);
                    PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "[%d] done decoding frame\n", vpu_ocb->ch_id);
                    int frame_dec_time = (curr_time - dec_st_time)/1000;
                    PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "Decoding time for frame = %d ms\n", frame_dec_time);

                    if (dec_status->terminate) {
                        struct timespec ts;
                        clock_gettime(CLOCK_MONOTONIC, &ts);

                        uint64_t stop_ts = (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
                        uint64_t dec_time = (stop_ts - vpu_ocb->start_ts)/1000;
                        float dec_frame_rate = vpu_ocb->codec_stats.d_stats.dec_frame_cnt * 1000.0 / (float)dec_time;
                        PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "Total decoding time %lu ms.", dec_time);
                        PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "Number of decoded frames %d ", vpu_ocb->codec_stats.d_stats.dec_frame_cnt);
                        PERFORMANCE_LOG(vpu_ocb->plog_fd, &ts, "Decoding frame rate is %.2f fps.", dec_frame_rate);
                        close(vpu_ocb->plog_fd);
                        vpu_ocb->plog_fd = -1;
                        trace_logf(VPU_RM_PER_MARKER, "VPU_DECODER MARKER: finished decoding");
                    }
                }

                SETIOV(&ctp->iov[0],&cargs, sizeof(TIVPU_CmdArgs));

                // check whether we should return here with return _RESMGR_NPARTS(1);
                break;
           }
 
        default:
            codec_slogerr("Unhandled devctl: %s:%d", __func__, __LINE__);
            err = EINVAL;
    }

    if (err != EOK) {
        codec_slogerr("%s: ERROR nbytes/%d err/%d EOK/%d\n",__func__,nbytes,err,EOK);
        pthread_mutex_unlock(&vpu_ocb->lock);
        return (err);
    }

    msg->o.ret_val = 0;
    pthread_mutex_unlock(&vpu_ocb->lock);
    return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o) + sizeof(TIVPU_CmdArgs)));
}

// TODO: Out of band is not supported now. The resmgr lib does not have the plumbing.
        // Neither does the codec.
static int tivpu_io_notify(resmgr_context_t *ctp, io_notify_t *msg, RESMGR_OCB_T *i_ocb)
{
    int32_t trigger = 0;
    int32_t ret;
    ti_vpu_ocb_t * ocb = (ti_vpu_ocb_t *)i_ocb;

    codec_slogtrace("%s", __func__);
    if (ocb->nitems > 0 )
         trigger = _NOTIFY_COND_OBAND;

    // the iofunc_notify() will do any necessary handling, including adding the client to the notification list if need be.
    ret = iofunc_notify( ctp, msg, ocb->notify, trigger, NULL, NULL);
    return ret;
}

static int tivpu_cleanup_codec_instance(resmgr_context_t *ctp, RESMGR_OCB_T *i_ocb)
{
    int ret = EOK;
    ti_vpu_ocb_t * ocb = (ti_vpu_ocb_t *)i_ocb;
    int32_t ch_id = ocb->ch_id;
    tivpu_context_t *ti_codec_ctx = &(g_codec_ctxs[ocb->ch_id]);

    pthread_mutex_lock(&g_instance_lock);
    if (ocb->created == false) {
        codec_sloginfo("%s: already cleaned up. Nothing to do here", __func__);
        pthread_mutex_unlock(&g_instance_lock);
        return EOK; 
    }

    /* mark this as stale */
    ocb->created = false;
    pthread_mutex_unlock(&g_instance_lock);

    if (ti_codec_ctx->is_dec) { /* This is a decoder */
        DecoderContext_t *ctx = ti_codec_ctx->codec_ctx;
        tivpu_release_feeder(ctx);
        tivpu_destroy_decoder(ctx);
    } else { /* Cleanup the encoder */
        tivpu_release_fb_mem(g_codec_ctxs[ch_id].codec_ctx);
        if (!tivpu_destroy_encoder(g_codec_ctxs[ch_id].codec_ctx)) {
            codec_slogerr("tivpu_destroy_encoder failed");
            ret = -1; /* need to define a error for this */
        }
    }

    ret = tivpu_cleanup_buffers(&g_codec_ctxs[ch_id]);

    pthread_mutex_lock(&g_instance_lock);
    memset(ti_codec_ctx, 0, sizeof(*ti_codec_ctx));
    ti_codec_ctx->ch_id = MAX_CODEC_HANDLES;
    pthread_mutex_unlock(&g_instance_lock);

    codec_debug_print_stats(ocb);

    return ret;
}

static int tivpu_io_close_dup(resmgr_context_t *ctp, io_close_t* msg, RESMGR_OCB_T *i_ocb)
{
    int ret;
    ti_vpu_ocb_t * ocb = (ti_vpu_ocb_t *)i_ocb;

    codec_slogtrace("%s", __func__);
    /*
    * A client has closed its file descriptor or has terminated.
    * Unblock any threads waiting for notification, then
    * remove the client from the notification list.
    */
    iofunc_notify_trigger_strict( ctp, ocb->notify, INT_MAX, IOFUNC_NOTIFY_OBAND );
    iofunc_notify_remove(ctp, ocb->notify);
    // cleanup the codec instance here

    ret = tivpu_cleanup_codec_instance(ctp, ocb);

    if (ret != EOK)
        codec_slogerr("%s: %d: cleanup failed", __func__, __LINE__);

    ret = iofunc_close_dup_default(ctp, msg, &ocb->ocb);
    return ret;
}
