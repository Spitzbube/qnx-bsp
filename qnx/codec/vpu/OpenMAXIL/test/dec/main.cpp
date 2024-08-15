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
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <screen/screen.h>
#include <queue>
#include <iostream>
#include <sys/procmgr.h>
#include <sys/slogcodes.h>

#include "omxil.h"
#include "log.h"


using namespace std;

int g_log_lvl = 0;
bool user_request_exit;

void LOG(int lvl, const char *fmt, ...)
{
    va_list arglist;
    va_start(arglist, fmt);
    if (lvl <= g_log_lvl) {
        vfprintf(stderr, fmt, arglist);
#if defined (DEBUG_MODE)
        slogf(_SLOGC_MEDIA, lvl, fmt, arglist);
#endif
        fprintf(stderr, "\n");
        fflush(stderr);
    }
    va_end(arglist);
}

static void print_usage_and_exit(const char *argv0)
{
    printf("Usage: %s [options] [input (absolute path)]\n"
           "  Command line options:\n"
           "    -C: VPU Core to choose (0 only for j721s2, 0,1 for j784s4)\n"
           "    -i: input file\n"
           "    -o: output file\n"
           "    -L: number of buffers for decode, less than 32 \n"
           "        - default value decided by codec hardware \n"
           "    -M: number of buffers for display, between 3 and 64 (default 12)\n"
           "    -v: increase verbosity, max 7\n"
           "    -n: use the second instance of the carveout. Used for multi-instance testing\n"
           "    -p: pseudo-YUV422 output using YUV420 source\n"
           "    -E: spatial & temporal error concealment\n"
           "\n"
           , argv0);
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

int main(int argc, char **argv)
{
    std::unique_ptr<OmxilVideoDec> decH;
    const char *outputpath = NULL;
    const char *inputpath = NULL;
    int dec_outbuff = 0;
    int disp_outbuff = VDEC_DISP_OUTPUT_BUFFER_NUM;
    bool post_to_screen = false;
    int core_idx = 0;
    int instance = 0;
    int opformat = 0;
    int err_conceal = 0;

    if (argc < 1) {
        print_usage_and_exit(argv[0]);
    }

    int opt;
    int ret = 0;
    while ((opt = getopt(argc, argv, "i:o:L:M:C:D:nvpE")) != -1) {
        switch (opt) {
        case 'i':
            inputpath = optarg;
            break;
        case 'o':
            outputpath = optarg;
            break;
        case 'v':
            g_log_lvl++;
            break;
        case 'L':
            dec_outbuff = atoi(optarg);
            if(dec_outbuff > 32)
                print_usage_and_exit(argv[0]);
            break;
        case 'M':
            disp_outbuff = atoi(optarg);
            if(disp_outbuff < 3 || disp_outbuff > 64)
                print_usage_and_exit(argv[0]);
            break;
        case 'C':
            core_idx = atoi(optarg);
            validate_core_idx(core_idx, argv[0]);
            break;
        case 'p':
            opformat = 1;
            break;
        case 'E':
            err_conceal = 1;
            break;
        case 'D': /* Currently an unsupported option */
                ;
#if 0
            post_to_screen = true;
            LOG(LOG_DEBUG2,"Enable posting to screen");
#endif
            break;

        case 'n':
            instance = 1;
            break;
        default:
            print_usage_and_exit(argv[0]);
        }
    }

    if (inputpath == NULL) {
        print_usage_and_exit(argv[0]);
    }

    LOG(LOG_DEBUG2,"Input file path %s", inputpath);

    if (procmgr_ability(0,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_KEYDATA,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_IO,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_MEM_PHYS,
                        PROCMGR_AOP_ALLOW | PROCMGR_ADN_NONROOT | PROCMGR_AID_PRIORITY,
                        PROCMGR_AOP_DENY  | PROCMGR_ADN_NONROOT | PROCMGR_AOP_LOCK      | PROCMGR_AID_EOL)
        != EOK) {
        LOG(LOG_ERROR, "Unable to gain procmgr abilities for nonroot operation.");
        return 0;
    }

    ThreadCtl( _NTO_TCTL_IO, 0);

    try {
        decH = std::unique_ptr<OmxilVideoDec>(new OmxilVideoDec(inputpath, outputpath, disp_outbuff, dec_outbuff, post_to_screen, core_idx, instance, opformat, err_conceal));
    }
    catch (const std::exception &e) {
        printf("%s", e.what());
        ret = BAIL_DEC_CATCH;
        goto bail;
    }

    /* Modifying terminal to allow key-press interaction */
    struct termios new_termios;
    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO | ECHOCTL | ECHONL);
    new_termios.c_cflag |= HUPCL;
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 10;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    printf("Press 'q' to quit.\n");

    for (;;) {
        if (decH->GetBailReason() != BAIL_NOT_BAILED || user_request_exit) {
            LOG(LOG_ERROR,"bailed: %d", __LINE__);
            break;
        }
        char ch[8];
        int chnum = 0;

        chnum = read(STDIN_FILENO, ch, 8);
        if (chnum == 1) {
            switch (ch[0]) {
                case 'q':
                case 'Q':
                    user_request_exit = true;
                    LOG(LOG_INFO,"bailed at user request: %d", __LINE__);
                    break;
                default:
                    printf("Unkown command [%x], valid commands are :\n"
                            "-----------------------------------------\n"
                            " q : stop playback and quit program.\n"
                            "-----------------------------------------\n",
                            ch[0]);
            }
        }
        else if (chnum == 0) {
            continue;
        }

    }

    /* Restore the terminal to its original state */
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

bail:
    if (decH) {
        if(decH->GetBailReason() == BAIL_EOS)
            printf("Decoding finished, exiting.\n");
        else if(decH->GetBailReason() == BAIL_ERROR){
            printf("Error happened, exiting.\n");
            ret = BAIL_ERROR;
        }
    } else {
        printf("failed to created the decoder test Handle. Exiting...");
        ret = BAIL_INIT_FAIL;
    }

    return ret;
}

