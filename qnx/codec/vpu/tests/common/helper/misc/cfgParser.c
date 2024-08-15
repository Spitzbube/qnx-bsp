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
#include <errno.h>
#include "vpuapi.h"
#include "vpuapifunc.h"
#include "main_helper.h"

#include <string.h>

const WaveCfgInfo waveCfgInfo[] = {
    //name                          min            max              default
    {"InputFile",                   0,              0,                      0}, //0
    {"SourceWidth",                 0,              W5_MAX_ENC_PIC_WIDTH,   0},
    {"SourceHeight",                0,              W5_MAX_ENC_PIC_HEIGHT,  0},
    {"InputBitDepth",               8,              10,                     8},
    {"FrameRate",                   0,              480,                    0},
    {"FrameSkip",                   0,              INT_MAX,                0},
    {"FramesToBeEncoded",           0,              INT_MAX,                0},
    {"IntraPeriod",                 0,              2047,                   0},
    {"DecodingRefreshType",         0,              2,                      1},
    {"GOPSize",                     1,              MAX_GOP_NUM,            1},
    {"IntraNxN",                    0,              1,                      1},// 10
    {"EnCu8x8",                     0,              1,                      1},
    {"EnCu16x16",                   0,              1,                      1},
    {"EnCu32x32",                   0,              1,                      1},
    {"IntraTransSkip",              0,              2,                      1},
    {"ConstrainedIntraPred",        0,              1,                      0},
    {"IntraCtuRefreshMode",         0,              4,                      0},
    {"IntraCtuRefreshArg",          0,              UI16_MAX,               0},
    {"MaxNumMerge",                 0,              2,                      2},
    {"EnTemporalMVP",               0,              1,                      1},
    {"ScalingList",                 0,              2,                      0}, // 20
    {"IndeSliceMode",               0,              1,                      0},
    {"IndeSliceArg",                0,              UI16_MAX,               0},
    {"DeSliceMode",                 0,              2,                      0},
    {"DeSliceArg",                  0,              UI16_MAX,               0},
    {"EnDBK",                       0,              1,                      1},
    {"EnSAO",                       0,              1,                      1},
    {"LFCrossSliceBoundaryFlag",    0,              1,                      1},
    {"BetaOffsetDiv2",             -6,              6,                      0},
    {"TcOffsetDiv2",               -6,              6,                      0},
    {"WaveFrontSynchro",            0,              1,                      0}, // 30
    {"LosslessCoding",              0,              1,                      0},
    {"UsePresetEncTools",           0,              3,                      0},
    {"GopPreset",                   0,             16,                      0},
    {"RateControl",                 0,              1,                      0},
    {"EncBitrate",                  0,              700000000,              0},
    {"InitialDelay",                10,             3000,                   0},
    {"EnHvsQp",                     0,              1,                      1},
    {"CULevelRateControl",          0,              1,                      1},
    {"ConfWindSizeTop",             0,              W5_MAX_ENC_PIC_HEIGHT,  0},
    {"ConfWindSizeBot",             0,              W5_MAX_ENC_PIC_HEIGHT,  0}, //40
    {"ConfWindSizeRight",           0,              W5_MAX_ENC_PIC_WIDTH,   0},
    {"ConfWindSizeLeft",            0,              W5_MAX_ENC_PIC_WIDTH,   0},
    {"HvsQpScaleDiv2",              0,              4,                      2},
    {"MinQp",                       0,              63,                     8},
    {"MaxQp",                       0,              63,                    51},
    {"MaxDeltaQp",                  0,              12,                    10},
    {"QP",                          0,              63,                    30},
    {"BitAllocMode",                0,              2,                      0},
    {"FixedBitRatio%d",             1,              255,                    1},
    {"InternalBitDepth",            0,              10,                     0}, //50
    {"EnUserDataSei",               0,              1,                      0},
    {"UserDataEncTiming",           0,              1,                      0},
    {"UserDataSize",                0,              (1<<24) - 1,            1},
    {"UserDataPos",                 0,              1,                      0},
    {"EnRoi",                       0,              1,                      0},
    {"NumUnitsInTick",              0,              INT_MAX,                0},
    {"TimeScale",                   0,              INT_MAX,                0},
    {"NumTicksPocDiffOne",          0,              INT_MAX,                0},
    {"EncAUD",                      0,              1,                      0},
    {"EncEOS",                      0,              1,                      0}, //60
    {"EncEOB",                      0,              1,                      0},
    {"CbQpOffset",                  -12,            12,                     0},
    {"CrQpOffset",                  -12,            12,                     0},
    {"RcInitialQp",                 -1,              63,                   63},
    {"EnNoiseReductionY",           0,              1,                      0},
    {"EnNoiseReductionCb",          0,              1,                      0},
    {"EnNoiseReductionCr",          0,              1,                      0},
    {"EnNoiseEst",                  0,              1,                      1},
    {"NoiseSigmaY",                 0,              255,                    0},
    {"NoiseSigmaCb",                0,              255,                    0}, //70
    {"NoiseSigmaCr",                0,              255,                    0},
    {"IntraNoiseWeightY",           0,              31,                     7},
    {"IntraNoiseWeightCb",          0,              31,                     7},
    {"IntraNoiseWeightCr",          0,              31,                     7},
    {"InterNoiseWeightY",           0,              31,                     4},
    {"InterNoiseWeightCb",          0,              31,                     4},
    {"InterNoiseWeightCr",          0,              31,                     4},
    {"UseAsLongTermRefPeriod",      0,              INT_MAX,                0},
    {"RefLongTermPeriod",           0,              INT_MAX,                0},
    {"CropXPos",                    0,              W5_MAX_ENC_PIC_WIDTH,   0}, //80
    {"CropYPos",                    0,              W5_MAX_ENC_PIC_HEIGHT,  0},
    {"CropXSize",                   0,              W5_MAX_ENC_PIC_WIDTH,   0},
    {"CropYSize",                   0,              W5_MAX_ENC_PIC_HEIGHT,  0},
    {"BitstreamFile",               0,              0,                      0},
    {"EnCustomVpsHeader",           0,              1,                      0},
    {"EnCustomSpsHeader",           0,              1,                      0},
    {"EnCustomPpsHeader",           0,              1,                      0},
    {"CustomVpsPsId",               0,              15,                     0},
    {"CustomSpsPsId",               0,              15,                     0},
    {"CustomSpsActiveVpsId",        0,              15,                     0}, //90
    {"CustomPpsActiveSpsId",        0,              15,                     0},
    {"CustomVpsIntFlag",            0,              1,                      1},
    {"CustomVpsAvailFlag",          0,              1,                      1},
    {"CustomVpsMaxLayerMinus1",     0,              62,                     0},
    {"CustomVpsMaxSubLayerMinus1",  0,              6,                      0},
    {"CustomVpsTempIdNestFlag",     0,              1,                      0},
    {"CustomVpsMaxLayerId",         0,              31,                     0},
    {"CustomVpsNumLayerSetMinus1",  0,              2,                      0},
    {"CustomVpsExtFlag",            0,              1,                      0},
    {"CustomVpsExtDataFlag",        0,              1,                      0}, //100
    {"CustomVpsSubOrderInfoFlag",   0,              1,                      0},
    {"CustomSpsSubOrderInfoFlag",   0,              1,                      0},
    {"CustomVpsLayerId0",           0,              INT_MAX,                0},
    {"CustomVpsLayerId1",           0,              INT_MAX,                0},
    {"CustomSpsLog2MaxPocMinus4",   0,              12,                     4},
// newly added for WAVE ENCODER
    {"EncMonochrome",               0,              1,                      0},
    {"StrongIntraSmoothing",        0,              1,                      1},
    {"RoiAvgQP",                    0,              63,                     0},
    {"WeightedPred",                0,              3,                      0},
    {"EnBgDetect",                  0,              1,                      0}, // 110
    {"BgThDiff",                    0,              255,                    8},
    {"BgThMeanDiff",                0,              255,                    1},
    {"BgLambdaQp",                  0,              63,                    32},
    {"BgDeltaQp",                   -16,            15,                     3},
    {"TileNumColumns",              1,              6,                      1},
    {"TileNumRows",                 1,              6,                      1},
    {"TileUniformSpace",            0,              1,                      1},
    {"EnLambdaMap",                 0,              1,                      0},
    {"EnCustomLambda",              0,              1,                      0},
    {"EnCustomMD",                  0,              1,                      0}, //120
    {"PU04DeltaRate",               0,              255,                    0},
    {"PU08DeltaRate",               0,              255,                    0},
    {"PU16DeltaRate",               0,              255,                    0},
    {"PU32DeltaRate",               0,              255,                    0},
    {"PU04IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU04IntraDcDeltaRate",        0,              255,                    0},
    {"PU04IntraAngleDeltaRate",     0,              255,                    0},
    {"PU08IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU08IntraDcDeltaRate",        0,              255,                    0},
    {"PU08IntraAngleDeltaRate",     0,              255,                    0}, //130
    {"PU16IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU16IntraDcDeltaRate",        0,              255,                    0},
    {"PU16IntraAngleDeltaRate",     0,              255,                    0},
    {"PU32IntraPlanarDeltaRate",    0,              255,                    0},
    {"PU32IntraDcDeltaRate",        0,              255,                    0},
    {"PU32IntraAngleDeltaRate",     0,              255,                    0},
    {"CU08IntraDeltaRate",          0,              255,                    0},
    {"CU08InterDeltaRate",          0,              255,                    0},
    {"CU08MergeDeltaRate",          0,              255,                    0},
    {"CU16IntraDeltaRate",          0,              255,                    0}, //140
    {"CU16InterDeltaRate",          0,              255,                    0},
    {"CU16MergeDeltaRate",          0,              255,                    0},
    {"CU32IntraDeltaRate",          0,              255,                    0},
    {"CU32InterDeltaRate",          0,              255,                    0},
    {"CU32MergeDeltaRate",          0,              255,                    0},
    {"DisableCoefClear",            0,              1,                      0},
    {"EnModeMap",                   0,              3,                      0},
    {"ForcePicSkipStart",          -1,              INT_MAX,               -1},
    {"ForcePicSkipEnd",            -1,              INT_MAX,               -1},
    {"ForceCoefDropStart",         -1,              INT_MAX,               -1}, //150
    {"ForceCoefDropEnd",           -1,              INT_MAX,               -1},
    {"EnUserFilterLevel",           0,              1,                      0},
    {"LfFilterLevel",              -63,            63,                      0},
    {"SharpnessLevel",              0,              7,                      0},
    {"LfRefDeltaIntra",            -63,            63,                      1},
    {"LfRefDeltaRef0",             -63,            63,                      0},
    {"LfRefDeltaRef1",             -63,            63,                     -1},
    {"LfModeDelta",                -63,            63,                      0},
    {"EnSVC",                       0,              1,                      0},
    {"YDcQpOffset",                -3,              3,                      0}, // 160
    {"CbCrDcQpOffset",             -3,              3,                      0},
    {"CbCrAcQpOffset",             -3,              3,                      0},
    {"StillPictureProfile",         0,              1,                      0},
    {"SvcMode",                     0,              1,                      1},
    {"VbvBufferSize",              10,             3000,                   3000},
    {"EncBitrateBL",                0,              700000000,              0},
    // newly added for H.264 on WAVE5
    {"IdrPeriod",                   0,              2047,                   0},
    {"RdoSkip",                     0,              1,                      1},
    {"LambdaScaling",               0,              1,                      1},
    {"Transform8x8",                0,              1,                      1},
    {"SliceMode",                   0,              1,                      0}, //170
    {"SliceArg",                    0,              INT_MAX,                0},
    {"IntraMbRefreshMode",          0,              3,                      0},
    {"IntraMbRefreshArg",           1,              INT_MAX,                1},
    {"MBLevelRateControl",          0,              1,                      0},
    {"CABAC",                       0,              1,                      1},
    {"RoiQpMapFile",                0,              1,                      1},
    {"S2fmeOff",                    0,              1,                      0},
    {"RcWeightParaCtrl",            1,              31,                     16},
    {"RcWeightBufCtrl",             1,              255,                    128}, //179, total 180
    // HLS
    {"EnPrefixSeiData",             0,              1,                      0},
    {"PrefixSeiDataSize",           0,              1024,                   1},
    {"EnSuffixSeiData",             0,              1,                      0},
    {"SuffixSeiDataSize",           0,              1024,                   1},
    {"EncodeRbspVui",               0,              1,                      0},
    {"RbspVuiSize",                 0,              16383,                  1},
    {"EncodeRbspHrdInVps",          0,              1,                      0},
    {"RbspHrdSize",                 0,              16383,                  1},//191  total : 192
    {"EnForcedIDRHeader",           0,                2,                    0},
    {"ForceIdrPicIdx",             -1,          INT_MAX,                   -1},
    {"Profile",                     0, H264_PROFILE_HIGH444,                0},
};

//------------------------------------------------------------------------------
// ENCODE PARAMETER PARSE FUNCSIONS
//------------------------------------------------------------------------------
static int WAVE_GetStringValue(
    osal_file_t fp,
    char* para,
    char* value
    )
{
    int pos = 0;
    char* token = NULL;
    char lineStr[256] = {0, };
    char valueStr[256] = {0, };
    osal_fseek(fp, 0, SEEK_SET);

    while (1) {
        if ( fgets(lineStr, 256, fp) == NULL ) {
            return 0;//not exist para in cfg file
        }

        if( (lineStr[0] == '#') || (lineStr[0] == ';') || (lineStr[0] == ':') ) { // check comment
            continue;
        }

        token = strtok(lineStr, ": "); // parameter name is separated by ' ' or ':'
        if( token != NULL ) {
            if ( strcasecmp(para, token) == 0) { // check parameter name
                token = strtok(NULL, ":\r\n");
                if ( token && strlen(token) == 1 && strncmp(token, " ", 1) == 0) //check space - ex) Frame1  : P  1  0  0
                    token = strtok(NULL, ":\r\n");
                if ( token == NULL )
                    return -1;
                osal_memcpy( valueStr, token, strlen(token) );
                while( valueStr[pos] == ' ' ) { // check space
                    pos++;
                }
                if ( valueStr[pos] == 0 )
                    return -1;//no value
                strcpy(value, &valueStr[pos]);
                return 1;
            }
            else {
                continue;
            }
        }
        else {
            continue;
        }
    }
}

// Parameter parsing helper
static int WAVE_GetValue(
    osal_file_t fp,
    char* cfgName,
    int* value
    )
{
    int i;
    int iValue;
    int ret;
    char sValue[256] = {0, };
    int maxCfg = sizeof(waveCfgInfo) / sizeof(WaveCfgInfo);

    for (i=0; i < maxCfg ;i++) {
        if ( strcmp(waveCfgInfo[i].name, cfgName) == 0)
            break;
    }
    if ( i == maxCfg ) {
        VLOG(ERR, "CFG param error : %s\n", cfgName);
        return 0;
    }

    ret = WAVE_GetStringValue(fp, cfgName, sValue);
    if(ret == 1) {
        iValue = atoi(sValue);
        if( (iValue >= waveCfgInfo[i].min) && (iValue <= waveCfgInfo[i].max) ) { // Check min, max
            *value = iValue;
            return 1;
        }
        else {
            VLOG(ERR, "CFG file error : %s value is not available. ( min = %d, max = %d)\n", waveCfgInfo[i].name, waveCfgInfo[i].min, waveCfgInfo[i].max);
            return 0;
        }
    }
    else if ( ret == -1 ) {
            VLOG(ERR, "CFG file error : %s value is not available. ( min = %d, max = %d)\n", waveCfgInfo[i].name, waveCfgInfo[i].min, waveCfgInfo[i].max);
            return 0;
    }
    else {
        *value = waveCfgInfo[i].def;
        return 1;
    }
}


static int WAVE_SetGOPInfo(
    char* lineStr,
    CustomGopPicParam* gopPicParam,
    int useDeriveLambdaWeight,
    int intraQp
    )
{
    int numParsed;
    char sliceType;

    osal_memset(gopPicParam, 0, sizeof(CustomGopPicParam));

    numParsed = sscanf(lineStr, "%c %d %d %d %d %d",
        &sliceType, &gopPicParam->pocOffset, &gopPicParam->picQp,
        &gopPicParam->temporalId, &gopPicParam->refPocL0, &gopPicParam->refPocL1);

    if (sliceType=='I') {
        gopPicParam->picType = PIC_TYPE_I;
    }
    else if (sliceType=='P') {
        gopPicParam->picType = PIC_TYPE_P;
        gopPicParam->useMultiRefP = (numParsed == 6) ? 1 : 0;
    }
    else if (sliceType=='B') {
        gopPicParam->picType = PIC_TYPE_B;
    }
    else {
        return 0;
    }
    if (sliceType=='B' && numParsed != 6) {
        return 0;
    }
    if (gopPicParam->temporalId < 0) {
        return 0;
    }

    gopPicParam->picQp = MIN(63, gopPicParam->picQp + intraQp);

    return 1;
}

static int WAVE_AVCSetGOPInfo(
    char* lineStr,
    CustomGopPicParam* gopPicParam,
    int useDeriveLambdaWeight,
    int intraQp
    )
{
    int numParsed;
    char sliceType;

    osal_memset(gopPicParam, 0, sizeof(CustomGopPicParam));

    numParsed = sscanf(lineStr, "%c %d %d %d %d %d",
        &sliceType, &gopPicParam->pocOffset, &gopPicParam->picQp,
        &gopPicParam->temporalId, &gopPicParam->refPocL0, &gopPicParam->refPocL1);

    if (sliceType=='I') {
        gopPicParam->picType = PIC_TYPE_I;
    }
    else if (sliceType=='P') {
        gopPicParam->picType = PIC_TYPE_P;
        gopPicParam->useMultiRefP = (numParsed == 6) ? 1 : 0;
    }
    else if (sliceType=='B') {
        gopPicParam->picType = PIC_TYPE_B;
    }
    else {
        return 0;
    }
    if (sliceType=='B' && numParsed != 6) {
        return 0;
    }

    gopPicParam->picQp = gopPicParam->picQp + intraQp;

    return 1;
}

int parseRoiCtuModeParam(
    char* lineStr,
    VpuRect* roiRegion,
    int* roiQp,
    int picX,
    int picY
    )
{
    int numParsed;

    osal_memset(roiRegion, 0, sizeof(VpuRect));
    *roiQp = 0;

    numParsed = sscanf(lineStr, "%d %d %d %d %d",
        &roiRegion->left, &roiRegion->right, &roiRegion->top, &roiRegion->bottom, roiQp);

    if (numParsed != 5) {
        return 0;
    }
    if (*roiQp < 0 || *roiQp > 51) {
        return 0;
    }
    if ((Int32)roiRegion->left < 0 || (Int32)roiRegion->top < 0) {
        return 0;
    }
    if (roiRegion->left > (Uint32)((picX + CTB_SIZE - 1) >> LOG2_CTB_SIZE) || \
        roiRegion->top > (Uint32)((picY + CTB_SIZE - 1) >> LOG2_CTB_SIZE)) {
        return 0;
    }
    if (roiRegion->right > (Uint32)((picX + CTB_SIZE - 1) >> LOG2_CTB_SIZE) || \
        roiRegion->bottom > (Uint32)((picY + CTB_SIZE - 1) >> LOG2_CTB_SIZE)) {
        return 0;
    }
    if (roiRegion->left > roiRegion->right) {
        return 0;
    }
    if (roiRegion->top > roiRegion->bottom) {
        return 0;
    }

    return 1;
}

int parseWaveEncCfgFile(
    ENC_CFG *pEncCfg,
    char *FileName,
    int bitFormat
    )
{
    osal_file_t fp;
    char sValue[256] = {0, };
    char tempStr[256] = {0, };
    int iValue = 0, ret = 0, i = 0;
    int intra8=0, intra16=0, intra32=0, frameSkip=0; // temp value
    UNREFERENCED_PARAMETER(frameSkip);

    fp = osal_fopen(FileName, "r");
    if (fp == NULL) {
        VLOG(ERR, "file open err : %s, errno(%d)\n", FileName, errno);
        return ret;
    }

    if (WAVE_GetStringValue(fp, "BitstreamFile", sValue) == 1)
        strcpy(pEncCfg->BitStreamFileName, sValue);

    if (WAVE_GetStringValue(fp, "InputFile", sValue) == 1)
        strcpy(pEncCfg->SrcFileName, sValue);
    else
        goto __end_parse;

    if (WAVE_GetValue(fp, "SourceWidth", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.picX = iValue;
    if (WAVE_GetValue(fp, "SourceHeight", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.picY = iValue;
    if (WAVE_GetValue(fp, "FramesToBeEncoded", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->NumFrame = iValue;
    if (WAVE_GetValue(fp, "InputBitDepth", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->SrcBitDepth = iValue; // BitDepth == 8 ? HEVC_PROFILE_MAIN : HEVC_PROFILE_MAIN10

    if (WAVE_GetValue(fp, "InternalBitDepth", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.internalBitDepth   = iValue;

    if (WAVE_GetValue(fp, "LosslessCoding", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.losslessEnable = iValue;
    if (WAVE_GetValue(fp, "ConstrainedIntraPred", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.constIntraPredFlag = iValue;
    if (WAVE_GetValue(fp, "DecodingRefreshType", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.decodingRefreshType = iValue;

    if (WAVE_GetValue(fp, "StillPictureProfile", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.enStillPicture = iValue;

    // BitAllocMode
    if (WAVE_GetValue(fp, "BitAllocMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bitAllocMode = iValue;

    // FixedBitRatio 0 ~ 7
#define FIXED_BIT_RATIO 49
    for (i=0; i<MAX_GOP_NUM; i++) {
        sprintf(tempStr, "FixedBitRatio%d", i);
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            iValue = atoi(sValue);
            if ( iValue >= waveCfgInfo[FIXED_BIT_RATIO].min && iValue <= waveCfgInfo[FIXED_BIT_RATIO].max )
                pEncCfg->waveCfg.fixedBitRatio[i] = iValue;
            else
                pEncCfg->waveCfg.fixedBitRatio[i] = waveCfgInfo[FIXED_BIT_RATIO].def;
        }
        else
            pEncCfg->waveCfg.fixedBitRatio[i] = waveCfgInfo[FIXED_BIT_RATIO].def;

    }

    if (WAVE_GetValue(fp, "QP", &iValue) == 0) //INTRA_QP
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraQP = iValue;

    if (WAVE_GetValue(fp, "IntraPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraPeriod = iValue;

    if (WAVE_GetValue(fp, "ConfWindSizeTop", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.confWinTop = iValue;

    if (WAVE_GetValue(fp, "ConfWindSizeBot", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.confWinBot = iValue;

    if (WAVE_GetValue(fp, "ConfWindSizeRight", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.confWinRight = iValue;

    if (WAVE_GetValue(fp, "ConfWindSizeLeft", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.confWinLeft = iValue;

    if (WAVE_GetValue(fp, "FrameRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.frameRate = iValue;

    if (WAVE_GetValue(fp, "IndeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.independSliceMode = iValue;

    if (WAVE_GetValue(fp, "IndeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.independSliceModeArg = iValue;

    if (WAVE_GetValue(fp, "DeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.dependSliceMode = iValue;

    if (WAVE_GetValue(fp, "DeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.dependSliceModeArg = iValue;


    if (WAVE_GetValue(fp, "IntraCtuRefreshMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraRefreshMode = iValue;


    if (WAVE_GetValue(fp, "IntraCtuRefreshArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraRefreshArg = iValue;

    if (WAVE_GetValue(fp, "UsePresetEncTools", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.useRecommendEncParam = iValue;

    if (WAVE_GetValue(fp, "ScalingList", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.scalingListEnable = iValue;

    if (WAVE_GetValue(fp, "EnCu8x8", &iValue) == 0)
        goto __end_parse;
    else
        intra8 = iValue;

    if (WAVE_GetValue(fp, "EnCu16x16", &iValue) == 0)
        goto __end_parse;
    else
        intra16 = iValue;

    if (WAVE_GetValue(fp, "EnCu32x32", &iValue) == 0)
        goto __end_parse;
    else
        intra32 = iValue;

    pEncCfg->waveCfg.cuSizeMode = (intra8&0x01) | (intra16&0x01)<<1 | (intra32&0x01)<<2;

    if (WAVE_GetValue(fp, "EnTemporalMVP", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.tmvpEnable = iValue;

    if (WAVE_GetValue(fp, "WaveFrontSynchro", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.wppenable = iValue;

    if (WAVE_GetValue(fp, "MaxNumMerge", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxNumMerge = iValue;

    if (WAVE_GetValue(fp, "EnDBK", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.disableDeblk = !(iValue);

    if (WAVE_GetValue(fp, "LFCrossSliceBoundaryFlag", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.lfCrossSliceBoundaryEnable = iValue;

    if (WAVE_GetValue(fp, "BetaOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.betaOffsetDiv2 = iValue;

    if (WAVE_GetValue(fp, "TcOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.tcOffsetDiv2 = iValue;

    if (WAVE_GetValue(fp, "IntraTransSkip", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.skipIntraTrans = iValue;

    if (WAVE_GetValue(fp, "EnSAO", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.saoEnable = iValue;

    if (WAVE_GetValue(fp, "IntraNxN", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraNxNEnable = iValue;

    if (WAVE_GetValue(fp, "RateControl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcEnable = iValue;

    if (WAVE_GetValue(fp, "EncBitrate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcBitRate = iValue;

    if (WAVE_GetValue(fp, "EncBitrateBL", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcBitRateBL = iValue;

    if (WAVE_GetValue(fp, "CULevelRateControl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cuLevelRCEnable = iValue;

    if (WAVE_GetValue(fp, "EnHvsQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hvsQPEnable = iValue;

    if (WAVE_GetValue(fp, "HvsQpScaleDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hvsQpScale = iValue;

    if (WAVE_GetValue(fp, "InitialDelay", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->VbvBufferSize = iValue;

    if (pEncCfg->VbvBufferSize == 0) {
        if (WAVE_GetValue(fp, "VbvBufferSize", &iValue) == 0)
            goto __end_parse;
        else
            pEncCfg->VbvBufferSize = iValue;
    }

    if (WAVE_GetValue(fp, "MinQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.minQp = iValue;

    if (WAVE_GetValue(fp, "MaxQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxQp = iValue;

    if (WAVE_GetValue(fp, "MaxDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxDeltaQp = iValue;

    if (WAVE_GetValue(fp, "GOPSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.gopParam.customGopSize = iValue;

    if (WAVE_GetValue(fp, "EnRoi", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.roiEnable = iValue;

    if (WAVE_GetValue(fp, "GopPreset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.gopPresetIdx = iValue;

    if (WAVE_GetValue(fp, "FrameSkip", &iValue) == 0)
        goto __end_parse;
    else
        frameSkip = iValue;

    if (WAVE_GetValue(fp, "NumUnitsInTick", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.numUnitsInTick = iValue;

    if (WAVE_GetValue(fp, "TimeScale", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.timeScale = iValue;

    if (WAVE_GetValue(fp, "NumTicksPocDiffOne", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.numTicksPocDiffOne = iValue;

    if (WAVE_GetValue(fp, "EncAUD", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.encAUD = iValue;

    if (WAVE_GetValue(fp, "EncEOS", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.encEOS = iValue;

    if (WAVE_GetValue(fp, "EncEOB", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.encEOB = iValue;

    if (WAVE_GetValue(fp, "CbQpOffset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.chromaCbQpOffset = iValue;

    if (WAVE_GetValue(fp, "CrQpOffset", &iValue) == 0)
            goto __end_parse;
    else
        pEncCfg->waveCfg.chromaCrQpOffset = iValue;

    if (WAVE_GetValue(fp, "RcInitialQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.initialRcQp = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrYEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrCbEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrCrEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseEst", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseEstEnable = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaY = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaCb = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaCr = iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightY = iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightCb= iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightCr = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightY = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightCb = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightCr = iValue;

    if (WAVE_GetValue(fp, "UseAsLongTermRefPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.useAsLongtermPeriod = iValue;

    if (WAVE_GetValue(fp, "RefLongTermPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.refLongtermPeriod = iValue;




    //GOP
    if (pEncCfg->waveCfg.intraPeriod == 1) {
        pEncCfg->waveCfg.gopParam.picParam[0].picType = PIC_TYPE_I;
        pEncCfg->waveCfg.gopParam.picParam[0].picQp = pEncCfg->waveCfg.intraQP;
        if (pEncCfg->waveCfg.gopParam.customGopSize > 1) {
            VLOG(ERR, "CFG file error : gop size should be smaller than 2 for all intra case\n");
            goto __end_parse;
        }
    }
    else {
        for (i = 0; pEncCfg->waveCfg.gopPresetIdx == PRESET_IDX_CUSTOM_GOP && i < pEncCfg->waveCfg.gopParam.customGopSize; i++) {

            sprintf(tempStr, "Frame%d", i+1);
            if (WAVE_GetStringValue(fp, tempStr, sValue) != 1) {
                VLOG(ERR, "CFG file error : %s value is not available. \n", tempStr);
                goto __end_parse;
            }

            if ( bitFormat == STD_AVC ) {
                if ( WAVE_AVCSetGOPInfo(sValue, &pEncCfg->waveCfg.gopParam.picParam[i], 0, pEncCfg->waveCfg.intraQP) != 1) {
                    VLOG(ERR, "CFG file error : %s value is not available. \n", tempStr);
                    goto __end_parse;
                }
            }
            else {
                if ( WAVE_SetGOPInfo(sValue, &pEncCfg->waveCfg.gopParam.picParam[i], 0, pEncCfg->waveCfg.intraQP) != 1) {
                    VLOG(ERR, "CFG file error : %s value is not available. \n", tempStr);
                    goto __end_parse;
                }
#if TEMP_SCALABLE_RC
                if ( (pEncCfg->waveCfg.gopParam.picParam[i].temporalId + 1) > MAX_NUM_TEMPORAL_LAYER) {
                    VLOG(ERR, "CFG file error : %s MaxTempLayer %d exceeds MAX_TEMP_LAYER(7). \n", tempStr, pEncCfg->waveCfg.gopParam.picParam[i].temporalId + 1);
                    goto __end_parse;
                }
#endif
            }
        }
    }

    //ROI
    if (pEncCfg->waveCfg.roiEnable) {
        sprintf(tempStr, "RoiFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.roiFileName);
        }
    }

    if (pEncCfg->waveCfg.losslessEnable) {
        pEncCfg->waveCfg.disableDeblk = 1;
        pEncCfg->waveCfg.saoEnable = 0;
        pEncCfg->RcEnable = 0;
    }

    if (pEncCfg->waveCfg.roiEnable) {
        sprintf(tempStr, "RoiQpMapFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.roiQpMapFile);
        }
    }

    /*======================================================*/
    /*          ONLY for H.264                              */
    /*======================================================*/
    if (WAVE_GetValue(fp, "IdrPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.idrPeriod = iValue;
    if (WAVE_GetValue(fp, "RdoSkip", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.rdoSkip = iValue;

    if (WAVE_GetValue(fp, "LambdaScaling", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.lambdaScalingEnable = iValue;

    if (WAVE_GetValue(fp, "Transform8x8", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.transform8x8 = iValue;

    if (WAVE_GetValue(fp, "SliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.avcSliceMode = iValue;

    if (WAVE_GetValue(fp, "SliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.avcSliceArg = iValue;

    if (WAVE_GetValue(fp, "IntraMbRefreshMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraMbRefreshMode = iValue;

    if (WAVE_GetValue(fp, "IntraMbRefreshArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraMbRefreshArg = iValue;

    if (WAVE_GetValue(fp, "MBLevelRateControl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.mbLevelRc = iValue;

    if (WAVE_GetValue(fp, "CABAC", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.entropyCodingMode = iValue;

    // H.264 END

    if (WAVE_GetValue(fp, "EncMonochrome", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.monochromeEnable = iValue;

    if (WAVE_GetValue(fp, "StrongIntraSmoothing", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.strongIntraSmoothEnable = iValue;

    if (WAVE_GetValue(fp, "RoiAvgQP", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.roiAvgQp = iValue;

    if (WAVE_GetValue(fp, "WeightedPred", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.weightPredEnable = iValue & 1;

    if (WAVE_GetValue(fp, "EnBgDetect", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgDetectEnable = iValue;

    if (WAVE_GetValue(fp, "BgThDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgThrDiff = iValue;

    if (WAVE_GetValue(fp, "S2fmeOff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.s2fmeDisable = iValue;

    if (WAVE_GetValue(fp, "BgThMeanDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgThrMeanDiff = iValue;

    if (WAVE_GetValue(fp, "BgLambdaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgLambdaQp = iValue;

    if (WAVE_GetValue(fp, "BgDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgDeltaQp = iValue;

    if (WAVE_GetValue(fp, "EnLambdaMap", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customLambdaMapEnable = iValue;

    if (WAVE_GetValue(fp, "EnCustomLambda", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customLambdaEnable = iValue;

    if (WAVE_GetValue(fp, "EnCustomMD", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customMDEnable = iValue;

    if (WAVE_GetValue(fp, "PU04DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08MergeDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16MergeDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32MergeDeltaRate = iValue;


    if (WAVE_GetValue(fp, "DisableCoefClear", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.coefClearDisable = iValue;


    if (WAVE_GetValue(fp, "EnModeMap", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customModeMapFlag = iValue;


    if (WAVE_GetValue(fp, "ForcePicSkipStart", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forcePicSkipStart = iValue;

    if (WAVE_GetValue(fp, "ForcePicSkipEnd", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forcePicSkipEnd = iValue;

    if (WAVE_GetValue(fp, "ForceCoefDropStart", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forceCoefDropStart = iValue;

    if (WAVE_GetValue(fp, "ForceCoefDropEnd", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forceCoefDropEnd = iValue;

    if (WAVE_GetValue(fp, "RcWeightParaCtrl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.rcWeightParam = iValue;

    if (WAVE_GetValue(fp, "RcWeightBufCtrl", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.rcWeightBuf = iValue;


    if (WAVE_GetValue(fp, "EnForcedIDRHeader", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forcedIdrHeaderEnable = iValue;

    if (WAVE_GetValue(fp, "ForceIdrPicIdx", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forceIdrPicIdx = iValue;

    // Scaling list
    if (pEncCfg->waveCfg.scalingListEnable) {
        sprintf(tempStr, "ScalingListFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.scalingListFileName);
        }
    }
    // Custom Lambda
    if (pEncCfg->waveCfg.customLambdaEnable) {
        sprintf(tempStr, "CustomLambdaFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.customLambdaFileName);
        }
    }


    // custom Lambda Map
    if (pEncCfg->waveCfg.customLambdaMapEnable) {
        sprintf(tempStr, "LambdaMapFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.customLambdaMapFileName);
        }
    }

    // custom Lambda Map
    if (pEncCfg->waveCfg.customModeMapFlag) {
        sprintf(tempStr, "ModeMapFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.customModeMapFileName);
        }
    }

    if (pEncCfg->waveCfg.weightPredEnable & 0x1) {
        sprintf(tempStr, "WpParamFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.WpParamFileName);
        }
    }

#define NUM_MAX_PARAM_CHANGE    10
    for (i=0; i<NUM_MAX_PARAM_CHANGE; i++) {
        sprintf(tempStr, "SPCh%d", i+1);
        if (WAVE_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%d %x %s\n", &(pEncCfg->changeParam[i].setParaChgFrmNum), &(pEncCfg->changeParam[i].enableOption), pEncCfg->changeParam[i].cfgName);

        }
        else {
            pEncCfg->numChangeParam = i;
            break;
        }
    }


    if (WAVE_GetValue(fp, "EnPrefixSeiData", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.prefixSeiEnable = iValue;

    if (WAVE_GetValue(fp, "PrefixSeiDataSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.prefixSeiDataSize = iValue;


    if (WAVE_GetValue(fp, "EnSuffixSeiData", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.suffixSeiEnable = iValue;

    if (WAVE_GetValue(fp, "SuffixSeiDataSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.suffixSeiDataSize = iValue;


    if (pEncCfg->waveCfg.prefixSeiEnable) {
        sprintf(tempStr, "PrefixSeiDataFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.prefixSeiDataFileName);
        }
    }

    if (pEncCfg->waveCfg.suffixSeiEnable) {
        sprintf(tempStr, "SuffixSeiDataFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.suffixSeiDataFileName);
        }
    }

    if (WAVE_GetValue(fp, "EncodeRbspVui", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.vuiDataEnable = iValue;

    if (WAVE_GetValue(fp, "RbspVuiSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.vuiDataSize = iValue;

    if (WAVE_GetValue(fp, "EncodeRbspHrdInVps", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hrdInVPS = iValue;

    if (WAVE_GetValue(fp, "RbspHrdSize", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hrdDataSize = iValue;

    if (pEncCfg->waveCfg.hrdInVPS) {
        sprintf(tempStr, "RbspHrdFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.hrdDataFileName);
        }
    }

    if (pEncCfg->waveCfg.vuiDataEnable) {
        sprintf(tempStr, "RbspVuiFile");
        if (WAVE_GetStringValue(fp, tempStr, sValue) != 0) {
            sscanf(sValue, "%s\n", pEncCfg->waveCfg.vuiDataFileName);
        }
    }

    if (WAVE_GetValue(fp, "Profile", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->Profile = iValue;

    ret = 1; /* Success */

__end_parse:
    osal_fclose(fp);
    return ret;
}

int parseWaveChangeParamCfgFile(
    ENC_CFG *pEncCfg,
    char *FileName
    )
{
    osal_file_t fp;
    char sValue[256] = {0, };
    char tempStr[256] = {0, };
    int iValue = 0, ret = 0, i = 0;

    fp = osal_fopen(FileName, "r");
    if (fp == NULL) {
        VLOG(ERR, "file open err : %s, errno(%d)\n", FileName, errno);
        return ret;
    }

    if (WAVE_GetValue(fp, "ConstrainedIntraPred", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.constIntraPredFlag = iValue;

    // FixedBitRatio 0 ~ 7
    for (i=0; i<MAX_GOP_NUM; i++) {
        sprintf(tempStr, "FixedBitRatio%d", i);
        if (WAVE_GetStringValue(fp, tempStr, sValue) == 1) {
            iValue = atoi(sValue);
            if ( iValue >= waveCfgInfo[FIXED_BIT_RATIO].min && iValue <= waveCfgInfo[FIXED_BIT_RATIO].max )
                pEncCfg->waveCfg.fixedBitRatio[i] = iValue;
            else
                pEncCfg->waveCfg.fixedBitRatio[i] = waveCfgInfo[FIXED_BIT_RATIO].def;
        }
        else
            pEncCfg->waveCfg.fixedBitRatio[i] = waveCfgInfo[FIXED_BIT_RATIO].def;

    }

    if (WAVE_GetValue(fp, "IndeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.independSliceMode = iValue;

    if (WAVE_GetValue(fp, "IndeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.independSliceModeArg = iValue;

    if (WAVE_GetValue(fp, "DeSliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.dependSliceMode = iValue;

    if (WAVE_GetValue(fp, "DeSliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.dependSliceModeArg = iValue;

    if (WAVE_GetValue(fp, "MaxNumMerge", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxNumMerge = iValue;

    if (WAVE_GetValue(fp, "EnDBK", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.disableDeblk = !(iValue);

    if (WAVE_GetValue(fp, "LFCrossSliceBoundaryFlag", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.lfCrossSliceBoundaryEnable = iValue;

    if (WAVE_GetValue(fp, "DecodingRefreshType", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.decodingRefreshType = iValue;

    if (WAVE_GetValue(fp, "BetaOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.betaOffsetDiv2 = iValue;

    if (WAVE_GetValue(fp, "TcOffsetDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.tcOffsetDiv2 = iValue;

    if (WAVE_GetValue(fp, "IntraNxN", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraNxNEnable = iValue;

    if (WAVE_GetValue(fp, "QP", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraQP = iValue;

    if (WAVE_GetValue(fp, "IntraPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.intraPeriod = iValue;

    if (WAVE_GetValue(fp, "EnForcedIDRHeader", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.forcedIdrHeaderEnable = iValue;

    if (WAVE_GetValue(fp, "FrameRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.frameRate = iValue;

    if (WAVE_GetValue(fp, "EncBitrate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->RcBitRate = iValue;

    if (WAVE_GetValue(fp, "EnHvsQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hvsQPEnable = iValue;

    if (WAVE_GetValue(fp, "HvsQpScaleDiv2", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.hvsQpScale = iValue;

    if (WAVE_GetValue(fp, "InitialDelay", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->VbvBufferSize = iValue;

    if (pEncCfg->VbvBufferSize == 0) {
        if (WAVE_GetValue(fp, "VbvBufferSize", &iValue) == 0)
            goto __end_parse;
        else
            pEncCfg->VbvBufferSize = iValue;
    }

    if (WAVE_GetValue(fp, "MinQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.minQp = iValue;

    if (WAVE_GetValue(fp, "MaxQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxQp = iValue;

    if (WAVE_GetValue(fp, "MaxDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.maxDeltaQp = iValue;


    if (WAVE_GetValue(fp, "CbQpOffset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.chromaCbQpOffset = iValue;

    if (WAVE_GetValue(fp, "CrQpOffset", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.chromaCrQpOffset = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrYEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrCbEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseReductionCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrCrEnable = iValue;

    if (WAVE_GetValue(fp, "EnNoiseEst", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseEstEnable = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaY = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaCb = iValue;

    if (WAVE_GetValue(fp, "NoiseSigmaCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrNoiseSigmaCr = iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightY = iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightCb= iValue;

    if (WAVE_GetValue(fp, "IntraNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrIntraWeightCr = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightY", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightY = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightCb", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightCb = iValue;

    if (WAVE_GetValue(fp, "InterNoiseWeightCr", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.nrInterWeightCr = iValue;


    /*======================================================*/
    /*          newly added for WAVE Encoder                */
    /*======================================================*/

    if (WAVE_GetValue(fp, "WeightedPred", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.weightPredEnable = iValue & 1;

    if (WAVE_GetValue(fp, "EnBgDetect", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgDetectEnable = iValue;

    if (WAVE_GetValue(fp, "BgThDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgThrDiff = iValue;

    if (WAVE_GetValue(fp, "S2fmeOff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.s2fmeDisable = iValue;

    if (WAVE_GetValue(fp, "BgThMeanDiff", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgThrMeanDiff = iValue;

    if (WAVE_GetValue(fp, "BgLambdaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgLambdaQp = iValue;

    if (WAVE_GetValue(fp, "BgDeltaQp", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.bgDeltaQp = iValue;


    if (WAVE_GetValue(fp, "EnCustomLambda", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customLambdaEnable = iValue;

    if (WAVE_GetValue(fp, "EnCustomMD", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.customMDEnable = iValue;

    if (WAVE_GetValue(fp, "DisableCoefClear", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.coefClearDisable = iValue;

    if (WAVE_GetValue(fp, "PU04DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32DeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32DeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU04IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu04IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU08IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu08IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU16IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu16IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraPlanarDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraPlanarDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraDcDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraDcDeltaRate = iValue;

    if (WAVE_GetValue(fp, "PU32IntraAngleDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.pu32IntraAngleDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU08MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu08MergeDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU16MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu16MergeDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32IntraDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32IntraDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32InterDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32InterDeltaRate = iValue;

    if (WAVE_GetValue(fp, "CU32MergeDeltaRate", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.cu32MergeDeltaRate = iValue;

    /*======================================================*/
    /*          only for H.264 encoder                      */
    /*======================================================*/
    if (WAVE_GetValue(fp, "IdrPeriod", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.idrPeriod = iValue;

    if (WAVE_GetValue(fp, "Transform8x8", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.transform8x8 = iValue;

    if (WAVE_GetValue(fp, "SliceMode", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.avcSliceMode = iValue;

    if (WAVE_GetValue(fp, "SliceArg", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.avcSliceArg = iValue;

    if (WAVE_GetValue(fp, "CABAC", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.entropyCodingMode = iValue;

    if (WAVE_GetValue(fp, "RdoSkip", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.rdoSkip = iValue;

    if (WAVE_GetValue(fp, "LambdaScaling", &iValue) == 0)
        goto __end_parse;
    else
        pEncCfg->waveCfg.lambdaScalingEnable = iValue;


    ret = 1; /* Success */

__end_parse:
    osal_fclose(fp);
    return ret;
}
