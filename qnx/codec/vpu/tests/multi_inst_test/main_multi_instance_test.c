//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include "main_helper.h"
#include "cnm_app.h"
#include "decoder_listener.h"
#include "encoder_listener.h"

enum waitStatus {
    WAIT_AFTER_INIT         = 0,

    WAIT_AFTER_DEC_OPEN     = 1,
    WAIT_AFTER_SEQ_INIT     = 2,
    WAIT_AFTER_REG_BUF      = 3,
    WAIT_BEFORE_DEC_START   = 4,
    WAIT_BEFORE_DEC_CLOSE   = 5,

    WAIT_AFTER_ENC_OPEN     = 6,
    WAIT_AFTER_INIT_SEQ     = 7,
    WAIT_AFTER_REG_FRAME    = 8,
    WAIT_BEFORE_ENC_START   = 9,
    WAIT_BEFORE_ENC_CLOSE   = 10,
    WAIT_NOT_DEFINED        = 11,
    WAIT_STATUS_MAX
};

enum get_result_status{
    GET_RESULT_INIT,
    GET_RESULT_BEFORE_WAIT_INT, //before calling wait interrupt.
    GET_RESULT_AFTER_WAIT_INT   //got interrupt.
};

static Uint32           sizeInWord;
static Uint16*          pusBitCode;


typedef struct {
    char            inputFilePath[256];
    char            localFile[256];
    char            outputFilePath[256];
    char            refFilePath[256];
    CodStd          stdMode;
    BOOL            afbce;
    BOOL            afbcd;
    BOOL            scaler;
    Uint32          sclw;
    Uint32          sclh;
    Int32           bsmode;
    BOOL            enableWTL;
    BOOL            enableMVC;
    int             cbcrInterleave;
    int             compareType;
    TestDecConfig   decConfig;
    TestEncConfig   encConfig;
    BOOL            isEncoder;
} InstTestConfig;

typedef struct {
    Uint32          totProcNum;             /* the number of process */
    Uint32          curProcNum;
    Uint32          numMulti;
    Uint32          cores;
    BOOL            performance;            /* for performance measurement */
    Uint32          pfClock;                /* for performance measurement */
    Uint32          bandwidth;              /* for bandwidth measurement */
    Uint32          fps;
    Uint32          numFrames;
    InstTestConfig  instConfig[MAX_NUM_INSTANCE];
} TestMultiConfig;


static void WaitForNextStep(Int32 instIdx, Int32 curStepName)
{
}

static void Help(struct OptionExt *opt, const char *programName)
{
    int i;

    VLOG(INFO, "------------------------------------------------------------------------------\n");
    VLOG(INFO, "%s(API v%d.%d.%d)\n", GetBasename(programName), API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    VLOG(INFO, "\tAll rights reserved by Chips&Media(C)\n");
    VLOG(INFO, "\tSample program controlling the Chips&Media VPU\n");
    VLOG(INFO, "------------------------------------------------------------------------------\n");
    VLOG(INFO, "%s [option] --input <stream list file, aka cmd file>\n", GetBasename(programName));
    VLOG(INFO, "-h                          help\n");
    VLOG(INFO, "-c                          enable comparison mode\n");
    VLOG(INFO, "                            1 : compare with golden stream that specified --ref_file_path option\n");
    VLOG(INFO, "-e                          0 : decoder, 1: encoder\n");
    VLOG(INFO, "-n                          number of frames\n");
    VLOG(INFO, "-f                          conformance test without reset CNM FPGA\n");
    for (i = 0;i < MAX_GETOPT_OPTIONS;i++) {
        if (opt[i].name == NULL)
            break;
        VLOG(INFO, "%s", opt[i].help);
    }
}

static void SetEncMultiParam(TestMultiConfig* multiConfig, Uint32 idx, Int32 productId)
{
    InstTestConfig *instConfig = &multiConfig->instConfig[idx];
    TestEncConfig *encCfg = &instConfig->encConfig;

    encCfg->numMulti      = multiConfig->numMulti;
    encCfg->stdMode       = instConfig->stdMode;
    encCfg->cbcrInterleave = instConfig->cbcrInterleave;
    encCfg->frame_endian  = 0;
    encCfg->stream_endian = 0;
    encCfg->yuv_mode      = 0; /* use default YuvFeeder that allows encCfg options to set YUV format */
    encCfg->mapType       = LINEAR_FRAME_MAP;
    if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
        encCfg->mapType   = COMPRESSED_FRAME_MAP;
    }

    strcpy(encCfg->cfgFileName, instConfig->inputFilePath);
    strcpy(encCfg->bitstreamFileName, instConfig->outputFilePath);
    if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
        if (instConfig->compareType == MD5_COMPARE) {
            encCfg->compareType |= (1 << MODE_COMP_RECON);
            strcpy(encCfg->ref_recon_md5_path, instConfig->refFilePath);
        }
        else if (instConfig->compareType == STREAM_COMPARE) {
            encCfg->compareType |= (1 << MODE_COMP_ENCODED);
            strcpy(encCfg->ref_stream_path, instConfig->refFilePath);
        }
    }
    instConfig->encConfig.performance = multiConfig->performance;
    instConfig->encConfig.pfClock     = multiConfig->pfClock;
    instConfig->encConfig.bandwidth   = multiConfig->bandwidth;
    instConfig->encConfig.fps         = multiConfig->fps;
    instConfig->encConfig.outNum      = multiConfig->numFrames;
    encCfg->srcFormat = FORMAT_420;

    instConfig->encConfig.subFrameSyncMode = REGISTER_BASE_SUB_FRAME_SYNC;
    instConfig->encConfig.productId = productId;
}

static void SetDecMultiParam(TestMultiConfig* multiConfig, Uint32 idx, Int32 productId)
{
    InstTestConfig *instConfig = &multiConfig->instConfig[idx];
    TestDecConfig *decCfg = &instConfig->decConfig;

    decCfg->numMulti             = multiConfig->numMulti;
    decCfg->bitFormat            = instConfig->stdMode;
    decCfg->streamEndian         = VPU_STREAM_ENDIAN;
    decCfg->frameEndian          = VPU_FRAME_ENDIAN;
    decCfg->cbcrInterleave       = instConfig->cbcrInterleave;
    decCfg->nv21                 = 0;
    decCfg->enableWTL            = instConfig->enableWTL;
    decCfg->wtlMode              = FF_FRAME;
    decCfg->wtlFormat            = FORMAT_420;

    if (decCfg->bitFormat == STD_VP9) {
        decCfg->bitstreamMode = BS_MODE_PIC_END;
        decCfg->feedingMode = FEEDING_METHOD_FRAME_SIZE;
    } else {
        decCfg->bitstreamMode        = instConfig->bsmode;
        decCfg->feedingMode          = (instConfig->bsmode == BS_MODE_INTERRUPT) ? FEEDING_METHOD_FIXED_SIZE : FEEDING_METHOD_FRAME_SIZE;
    }

    if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
        decCfg->mapType          = COMPRESSED_FRAME_MAP;
    }
    else {
        decCfg->mapType          = LINEAR_FRAME_MAP;
    }
    if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
        VLOG(WARN, "INS_NUM : %d , bit stream buffer size : 0x%x \n", idx, decCfg->bsSize);
        VLOG(WARN, "If bs-size is 0, it use defined values in component_dec_feeder.c according to standars \n");
    } else {
        decCfg->bsSize               = (5*1024*1024);
    } 
    decCfg->productId            = productId;


    strcpy(decCfg->inputPath, instConfig->inputFilePath);
    if (instConfig->localFile[0]) {
        strcpy(decCfg->inputPath, instConfig->localFile);
    }
    strcpy(decCfg->outputPath, instConfig->outputFilePath);
    if (instConfig->compareType == MD5_COMPARE) {
        decCfg->compareType = MD5_COMPARE;
        strcpy(decCfg->md5Path, instConfig->refFilePath);
    }
    else if (instConfig->compareType == YUV_COMPARE) {
        decCfg->compareType = YUV_COMPARE;
        strcpy(decCfg->refYuvPath, instConfig->refFilePath);
    }
    else {
        decCfg->compareType = NO_COMPARE;
    }

    decCfg->performance = multiConfig->performance;
    decCfg->pfClock     = multiConfig->pfClock;
    decCfg->bandwidth   = multiConfig->bandwidth;
    decCfg->fps         = multiConfig->fps;
    decCfg->forceOutNum = multiConfig->numFrames;
}

static void MultiDecoderListener(Component com, Uint64 event, void* data, void* context)
{
    DecHandle               decHandle = NULL;

    ComponentGetParameter(NULL, com, GET_PARAM_DEC_HANDLE, &decHandle);
    if (decHandle == NULL && event != COMPONENT_EVENT_DEC_DECODED_ALL) {
        // Terminated state
        return;
    }

    switch (event) {
    case COMPONENT_EVENT_DEC_OPEN:
        WaitForNextStep(decHandle->instIndex, WAIT_AFTER_DEC_OPEN);
        break;
    case COMPONENT_EVENT_DEC_COMPLETE_SEQ:
        WaitForNextStep(decHandle->instIndex, WAIT_AFTER_SEQ_INIT);
        HandleDecCompleteSeqEvent(com, (CNMComListenerDecCompleteSeq*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_REGISTER_FB:
        WaitForNextStep(decHandle->instIndex, WAIT_AFTER_REG_BUF);
        HandleDecRegisterFbEvent(com, (CNMComListenerDecRegisterFb*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_READY_ONE_FRAME:
        WaitForNextStep(decHandle->instIndex, WAIT_BEFORE_DEC_START);
        break;
    case COMPONENT_EVENT_DEC_INTERRUPT:
        HandleDecInterruptEvent(com, (CNMComListenerDecInt*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_GET_OUTPUT_INFO:
        HandleDecGetOutputEvent(com, (CNMComListenerDecDone*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_DECODED_ALL:
        // It isn't possible to get handle when a component is terminated state.
        decHandle = (DecHandle)data;
        if (decHandle) WaitForNextStep(decHandle->instIndex, WAIT_BEFORE_DEC_CLOSE);
        break;
    default:
        break;
    }
}

static BOOL CreateDecoderTask(CNMTask task, CNMComponentConfig* config, DecListenerContext* lsnCtx)
{
    BOOL    success = FALSE;
    RetCode ret     = RETCODE_SUCCESS;

    if (STD_AV1 == config->testDecConfig.bitFormat) {
        if (0 == strcmp("obu", GetFileExtension(config->testDecConfig.inputPath))) {
            // Actually, the streams having the obu extension are annex-b format streams.
            config->testDecConfig.wave.av1Format = 2;
        }
        else {
            config->testDecConfig.wave.av1Format = 0;
        }
    }

    if (RETCODE_SUCCESS != (ret=SetUpDecoderOpenParam(&(config->decOpenParam), &(config->testDecConfig)))) {
        VLOG(ERR, "%s:%d SetUpDecoderOpenParam failed Error code is 0x%x \n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    Component feeder   = ComponentCreate("feeder", config);
    Component renderer = ComponentCreate("renderer", config);
    Component decoder  = ComponentCreate("wave_decoder",  config);

    CNMTaskAdd(task, feeder);
    CNMTaskAdd(task, decoder);
    CNMTaskAdd(task, renderer);

    if ((success=SetupDecListenerContext(lsnCtx, config, renderer)) == TRUE) {
        ComponentRegisterListener(decoder, COMPONENT_EVENT_DEC_ALL, MultiDecoderListener, (void*)lsnCtx);
    }

    return success;
}

static void MultiEncoderListener(Component com, Uint64 event, void* data, void* context)
{
    EncHandle                       encHandle   = NULL;
    CNMComListenerEncStartOneFrame* lsnStartEnc = NULL;
    ProductId                       productId;

    UNREFERENCED_PARAMETER(lsnStartEnc);

    ComponentGetParameter(NULL, com, GET_PARAM_ENC_HANDLE, &encHandle);
    if (encHandle == NULL && event != COMPONENT_EVENT_ENC_ENCODED_ALL) {
        // Terminated state
        return;
    }

    switch (event) {
    case COMPONENT_EVENT_ENC_OPEN:
        WaitForNextStep(encHandle->instIndex, WAIT_AFTER_ENC_OPEN);
        break;
    case COMPONENT_EVENT_ENC_COMPLETE_SEQ:
        WaitForNextStep(encHandle->instIndex, WAIT_AFTER_INIT_SEQ);
        HandleEncCompleteSeqEvent(com, (CNMComListenerEncCompleteSeq*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_REGISTER_FB:
        WaitForNextStep(encHandle->instIndex, WAIT_AFTER_REG_FRAME);
        break;
    case COMPONENT_EVENT_ENC_READY_ONE_FRAME:
        WaitForNextStep(encHandle->instIndex, WAIT_BEFORE_ENC_START);
        break;
    case COMPONENT_EVENT_ENC_START_ONE_FRAME:
        break;
    case COMPONENT_EVENT_ENC_HANDLING_INT:
        HandleEncHandlingIntEvent(com, (CNMComListenerHandlingInt*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_GET_OUTPUT_INFO:
        encHandle = ((CNMComListenerEncDone*)data)->handle;
        productId = VPU_GetProductId(VPU_HANDLE_CORE_INDEX(encHandle));
        if (TRUE == PRODUCT_ID_W_SERIES(productId)) {
            HandleEncGetOutputEvent(com, (CNMComListenerEncDone*)data, (EncListenerContext*)context);
        }
        break;
    case COMPONENT_EVENT_ENC_ENCODED_ALL:
        // It isn't possible to get handle when a component is terminated state.
        encHandle = (EncHandle)data;
        if (encHandle)
            WaitForNextStep(encHandle->instIndex, WAIT_BEFORE_ENC_CLOSE);
        break;
    case COMPONENT_EVENT_ENC_FULL_INTERRUPT:
        HandleEncFullEvent(com, (CNMComListenerEncFull*)data, (EncListenerContext*)context);
        break;
    case COMPONENT_EVENT_ENC_CLOSE:
        HandleEncGetEncCloseEvent(com, (CNMComListenerEncClose*)data, (EncListenerContext*)context);
        break;
    default:
        break;
    }
}

static BOOL CreateEncoderTask(CNMTask task, CNMComponentConfig* config, EncListenerContext* lsnCtx)
{
    Component feeder;
    Component encoder;
    Component reader;
    BOOL      success = FALSE;

    if (SetupEncoderOpenParam(&config->encOpenParam, &config->testEncConfig, NULL) == FALSE) {
        return FALSE;
    }

    feeder  = ComponentCreate("yuvfeeder",      config);
    encoder = ComponentCreate("wave_encoder",   config);
    reader  = ComponentCreate("reader",         config);

    CNMTaskAdd(task, feeder);
    CNMTaskAdd(task, encoder);
    CNMTaskAdd(task, reader);

    if ((success=SetupEncListenerContext(lsnCtx, config)) == TRUE) {
        ComponentRegisterListener(encoder, COMPONENT_EVENT_ENC_ALL, MultiEncoderListener, (void*)lsnCtx);
    }

    return success;
}

static BOOL MultiInstanceTest(TestMultiConfig* multiConfig, Uint16* fw, Uint32 size)
{
    Uint32              i;
    CNMComponentConfig  config;
    CNMTask             task;
    DecListenerContext* decListenerCtx = (DecListenerContext*)osal_malloc(sizeof(DecListenerContext) * MAX_NUM_INSTANCE);
    EncListenerContext* encListenerCtx = (EncListenerContext*)osal_malloc(sizeof(EncListenerContext) * MAX_NUM_INSTANCE);
    BOOL                ret     = FALSE;
    BOOL                success = TRUE;
    BOOL                match   = TRUE;

    CNMAppInit();

    for (i=0; i < multiConfig->numMulti; i++) {
        task = CNMTaskCreate();
        memset((void*)&config, 0x00, sizeof(CNMComponentConfig));
        config.bitcode       = (Uint8*)fw;
        config.sizeOfBitcode = size;
        if (multiConfig->instConfig[i].isEncoder == TRUE) {
            memcpy((void*)&config.testEncConfig, &multiConfig->instConfig[i].encConfig, sizeof(TestEncConfig));
            success = CreateEncoderTask(task, &config, &encListenerCtx[i]);
        }
        else
        {
            memcpy((void*)&config.testDecConfig, &multiConfig->instConfig[i].decConfig, sizeof(TestDecConfig));
            success = CreateDecoderTask(task, &config, &decListenerCtx[i]);
        }
        CNMAppAdd(task);
        if (success == FALSE) {
            CNMAppStop();
            return FALSE;
        }
    }

    ret = CNMAppRun();

    for (i=0; i<multiConfig->numMulti; i++) {
        if (multiConfig->instConfig[i].isEncoder == TRUE) {
            match &= (encListenerCtx[i].match == TRUE && encListenerCtx[i].matchOtherInfo == TRUE);
            ClearEncListenerContext(&encListenerCtx[i]);
        }
        else
        {
            match &= decListenerCtx[i].match;
            ClearDecListenerContext(&decListenerCtx[i]);
        }
    }
    if (ret == TRUE) ret = match;

    osal_free(decListenerCtx);
    osal_free(encListenerCtx);

    return ret;
}

int main(int argc, char **argv)
{
    Int32           coreIndex   = 0;
    Uint32          productId   = 0;
    TestMultiConfig multiConfig;
    int             opt, index, i;
    char*           tempArg;
    char*           optString   = "fc:e:hn:";

    struct option options[MAX_GETOPT_OPTIONS];
    struct OptionExt options_help[MAX_GETOPT_OPTIONS] = {
        {"instance-num",          1, NULL, 0, "--instance-num              total instance-number to run\n"},
        {"codec",                 1, NULL, 0, "--codec                     The index of codec (AVC = 0, HEVC = 12)\n"},
        {"input",                 1, NULL, 0, "--input                     bitstream(decoder) or cfg(encoder) path\n"},
        {"output",                1, NULL, 0, "--output                    yuv(decoder) or bitstream(encoder) path\n"},
        {"ref_file_path",         1, NULL, 0, "--ref_file_path             Golden md5 or stream path\n"},
        {"coreIdx",               1, NULL, 0, "--coreIdx                   core index: default 0\n"},
        {"bs-size",               1, NULL, 0, "--bs-size                   bit stream buffer size in byte \n"},
        {"bsmode",                1, NULL, 0, "--bsmode                    set bitstream mode.\n"},
        {"enable-wtl",            1, NULL, 0, "--enable-wtl                enable wtl option. default 0\n"},
        {"enable-uvintlv",        1, NULL, 0, "--enable-uvintlv            enable UV (CbCr) Interleave option (for NV12 support). default 0\n"},
        {"sfs",                   1, NULL, 0, "--sfs                       subFrameSync 0: off 1: on, default off\n"},
        {"pf",                    1, NULL, 0, "--pf                        0: disable peformance report(default), 1: enable performance report\n"},
        {"pf-clock",              1, NULL, 0, "--pf-clock                  peformance clock in Hz(It must be BCLK for WAVE5 series).\n"},
        {"bw",                    1, NULL, 0, "--bw                        0: disable bandwidth report(default) 1: enable bandwidth report\n"},
        {"fps",                   1, NULL, 0, "--fps                       frame per seconds\n"},
        {"cores",                 1, NULL, 0, "--cores                     The number of vcores for each instance\n"},
        {NULL,                    0, NULL, 0},
    };
    const char*     name;
    char*           firmwarePath = NULL;
    Uint32          ret = 0;


    osal_memset(&multiConfig, 0x00, sizeof(multiConfig));




    for (i = 0; i < MAX_GETOPT_OPTIONS;i++) {
        if (options_help[i].name == NULL)
            break;
        osal_memcpy(&options[i], &options_help[i], sizeof(struct option));
    }

    while ((opt=getopt_long(argc, argv, optString, options, &index)) != -1) {
        switch (opt) {
        case 'c':
            tempArg = strtok(optarg, ",");
            for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                multiConfig.instConfig[i].compareType = atoi(tempArg);
                tempArg = strtok(NULL, ",");
                if (tempArg == NULL)
                    break;
            }
            break;
        case 'e':
            tempArg = strtok(optarg, ",");
            for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                multiConfig.instConfig[i].isEncoder = atoi(tempArg);
                tempArg = strtok(NULL, ",");
                if (tempArg == NULL)
                    break;
            }
            break;
        case 'h':
            Help(options_help, argv[0]);
            return 0;
        case 'n':
            multiConfig.numFrames = atoi(optarg);
            break;
        case 0:
            name = options[index].name;
            if (strcmp("instance-num", name) == 0) {
                multiConfig.numMulti = atoi(optarg);
            }
            else if (strcmp("codec", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].stdMode = (CodStd)atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("input", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    osal_memcpy(multiConfig.instConfig[i].inputFilePath, tempArg, strlen(tempArg));
                    ChangePathStyle(multiConfig.instConfig[i].inputFilePath);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("output", name) == 0) {
                char output_temp[730] = { 0, };
                char *output_name;
                sprintf(output_temp, "%s", optarg);
                output_name = output_temp;
                i = 0;
                while ((tempArg = strsep(&output_name, ",")) != NULL) {
                    osal_memcpy(multiConfig.instConfig[i].outputFilePath, tempArg, strlen(tempArg));
                    ChangePathStyle(multiConfig.instConfig[i].outputFilePath);
                    i++;
                }
            }
            else if (strcmp("ref_file_path", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    osal_memcpy(multiConfig.instConfig[i].refFilePath, tempArg, strlen(tempArg));
                    ChangePathStyle(multiConfig.instConfig[i].refFilePath);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("bs-size", name) == 0) {
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].decConfig.bsSize = atoi(optarg);
                }
            }
            else if (!strcmp(options[index].name, "coreIdx")) {
                VLOG(INFO, "just DPI constraint");
            }
            else if (strcmp("bsmode", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].bsmode = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("enable-wtl", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].enableWTL = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("enable-uvintlv", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].cbcrInterleave = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL)
                        break;
                }
            }
            else if (strcmp("sfs", name) == 0) {
                tempArg = strtok(optarg, ",");
                for(i = 0; i < MAX_NUM_INSTANCE; i++) {
                    multiConfig.instConfig[i].encConfig.subFrameSyncEn = atoi(tempArg);
                    tempArg = strtok(NULL, ",");
                    if (tempArg == NULL) break;
                }
            }
            else if (!strcmp(options[index].name, "pf")) {
                multiConfig.performance = (BOOL)atoi(optarg);
            }
            else if (!strcmp(options[index].name, "bw")) {
                multiConfig.bandwidth = (BOOL)atoi(optarg);
            }
            else if (!strcmp(options[index].name,  "pf-clock")) {
                multiConfig.pfClock = (Uint32)atoi(optarg);
            }
            else if (!strcmp(options[index].name,  "fps")) {
                multiConfig.fps = (Uint32)atoi(optarg);
            }
            else {
                VLOG(ERR, "unknown --%s\n", name);
                Help(options_help, argv[0]);
                return 1;
            }
            break;
        case '?':
            return 1;
        }
    }

    InitLog();



    productId = VPU_GetProductId(coreIndex);
    switch (productId) {
    case PRODUCT_ID_521: firmwarePath = CORE_6_BIT_CODE_FILE_PATH; break;
    default:
        VLOG(ERR, "<%s:%d> Unknown productId(%d)\n", __FUNCTION__, __LINE__, productId);
        return 1;
    }

    if (LoadFirmware(productId, (Uint8**)&pusBitCode, &sizeInWord, firmwarePath) < 0) {
        VLOG(ERR, "%s:%d Failed to load firmware: %s\n", __FUNCTION__, __LINE__, firmwarePath);
        return 1;
    }
    for (i = 0; i < MAX_NUM_INSTANCE; i++) {
        if (multiConfig.instConfig[i].isEncoder == FALSE)
            SetDecMultiParam(&multiConfig, i, productId);
        else
            SetEncMultiParam(&multiConfig, i, productId);
    }


    do {

        if (MultiInstanceTest(&multiConfig, pusBitCode, sizeInWord) == FALSE) {
            VLOG(ERR, "Failure in MultiInstanceTest()\n");
            ret = 1;
            break;
        }
        else {
            VLOG(INFO, "[RESULT] SUCCESS\n");
        }
    } while(FALSE);

    osal_free(pusBitCode);

    return ret;
}

