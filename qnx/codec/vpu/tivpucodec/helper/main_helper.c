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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "vpuapifunc.h"
#include "wave5_regdefine.h"
#include "vpuerror.h"
#include "main_helper.h"
#include "misc/debug.h"
#if defined(PLATFORM_NON_OS) || defined (PLATFORM_LINUX)
#include <getopt.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/slog2.h>


#define BIT_DUMMY_READ_GEN          0x06000000
#define BIT_READ_LATENCY            0x06000004
#define W5_SET_READ_DELAY           0x01000000
#define W5_SET_WRITE_DELAY          0x01000004
#define MAX_CODE_BUF_SIZE           (512*1024)

// JB: Some defaults
#define MIN_IN_BUFFERS         3

char* EncPicTypeStringH264[] = {
    "IDR/I",
    "P",
};

char* EncPicTypeStringMPEG4[] = {
    "I",
    "P",
};

/* return TRUE  - Go next step
 *        FALSE - Retry
 */
BOOL ParamReturnTest(ComponentParamRet ret, BOOL* success)
{
    BOOL next_step = TRUE;

    switch (ret) {
    case COMPONENT_PARAM_FAILURE:    next_step = FALSE; *success = FALSE; break;
    case COMPONENT_PARAM_SUCCESS:    next_step = TRUE;  *success = TRUE;  break;
    case COMPONENT_PARAM_NOT_READY:  next_step = FALSE; *success = TRUE;  break;
    case COMPONENT_PARAM_NOT_FOUND:  next_step = FALSE; *success = FALSE; break;
    case COMPONENT_PARAM_TERMINATED: next_step = FALSE; *success = TRUE;  break;
    default:                         next_step = FALSE; *success = FALSE; break;
    }

    return next_step;
}


// Most of the code can be removed for the codec, since we do not use any of this.
// commenting this out for now. Will be cleaned if we do not use it for the
// decoder too.
#if 0
void SetDefaultDecTestConfig(TestDecConfig* testConfig)
{
    osal_memset(testConfig, 0, sizeof(TestDecConfig));

    testConfig->bitstreamMode   = BS_MODE_INTERRUPT;
    testConfig->feedingMode     = FEEDING_METHOD_FIXED_SIZE;
    testConfig->streamEndian    = VPU_STREAM_ENDIAN;
    testConfig->frameEndian     = VPU_FRAME_ENDIAN;
    testConfig->cbcrInterleave  = FALSE;
    testConfig->nv21            = FALSE;
    testConfig->bitFormat       = STD_HEVC;
    testConfig->renderType      = RENDER_DEVICE_NULL;
    testConfig->mapType         = COMPRESSED_FRAME_MAP;
    testConfig->enableWTL       = TRUE;
    testConfig->wtlMode         = FF_FRAME;
    testConfig->wtlFormat       = FORMAT_420;           //!<< 8 bit YUV
    testConfig->wave.av1Format  = 0;
    testConfig->fps             = 30;
}

Uint32 randomSeed;
static BOOL initializedRandom;
Uint32 GetRandom(
    Uint32   start,
    Uint32   end
    )
{
    Uint32   range = end-start+1;

    if (randomSeed == 0) {
        randomSeed = (Uint32)time(NULL);
        VLOG(INFO, "======= RANDOM SEED: %08x ======\n", randomSeed);
    }
    if (initializedRandom == FALSE) {
        srand(randomSeed);
        initializedRandom = TRUE;
    }

    if (range == 0) {
        VLOG(ERR, "%s:%d RANGE IS 0\n", __FUNCTION__, __LINE__);
        return 0;
    }
    else {
        return ((rand()%range) + start);
    }
}

Int32 LoadFirmware(
    Int32       productId,
    Uint8**     retFirmware,
    Uint32*     retSizeInWord,
    const char* path
    )
{
    Int32       nread;
    Uint32      totalRead, allocSize, readSize = WAVE5_MAX_CODE_BUF_SIZE;
    Uint8*      firmware = NULL;
    osal_file_t fp;

    if ((fp=osal_fopen(path, "rb")) == NULL)
    {
        VLOG(ERR, "Failed to open %s\n", path);
        return -1;
    }

    totalRead = 0;
    if (PRODUCT_ID_W_SERIES(productId)) {
        firmware = (Uint8*)osal_malloc(readSize);
        allocSize = readSize;
        nread = 0;
        while (TRUE) {
            if (allocSize < (totalRead+readSize)) {
                allocSize += 2*nread;
                firmware = (Uint8*)realloc(firmware, allocSize);
            }
            nread = osal_fread((void*)&firmware[totalRead], 1, readSize, fp);//lint !e613
            totalRead += nread;
            if (nread < (Int32)readSize)
                break;
        }
        *retSizeInWord = (totalRead+1)/2;
    }

    osal_fclose(fp);

    *retFirmware   = firmware;

    return 0;
}


void PrintVpuVersionInfo(
    Uint32   core_idx
    )
{
    Uint32 version;
    Uint32 revision;
    Uint32 productId;

    VPU_GetVersionInfo(core_idx, &version, &revision, &productId);

    VLOG(INFO, "VPU coreNum : [%d]\n", core_idx);
    VLOG(INFO, "Firmware : CustomerCode: %04x | version : %d.%d.%d rev.%d\n",
        (Uint32)(version>>16), (Uint32)((version>>(12))&0x0f), (Uint32)((version>>(8))&0x0f), (Uint32)((version)&0xff), revision);
    VLOG(INFO, "Hardware : %04x\n", productId);
    VLOG(INFO, "API      : %d.%d.%d\n\n", API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
}

BOOL OpenDisplayBufferFile(CodStd codec, char *outputPath, VpuRect rcDisplay, TiledMapType mapType, FILE *fp[])
{
    char strFile[MAX_FILE_PATH];
    int width;
    int height;

    width = rcDisplay.right - rcDisplay.left;
    height = rcDisplay.bottom - rcDisplay.top;

    if (mapType == LINEAR_FRAME_MAP) {
        if ((fp[0]=osal_fopen(outputPath, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, outputPath);
            goto ERR_OPEN_DISP_BUFFER_FILE;
        }
    }
    else {
        width  = (codec == STD_HEVC || codec == STD_AVS2) ? VPU_ALIGN16(width)  : VPU_ALIGN64(width);
        height = (codec == STD_HEVC || codec == STD_AVS2) ? VPU_ALIGN16(height) : VPU_ALIGN64(height);
        sprintf(strFile, "%s_%dx%d_fbc_data_y.bin", outputPath, width, height);
        if ((fp[0]=osal_fopen(strFile, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, strFile);
            goto ERR_OPEN_DISP_BUFFER_FILE;
        }
        sprintf(strFile, "%s_%dx%d_fbc_data_c.bin", outputPath, width, height);
        if ((fp[1]=osal_fopen(strFile, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, strFile);
            goto ERR_OPEN_DISP_BUFFER_FILE;
        }
        sprintf(strFile, "%s_%dx%d_fbc_table_y.bin", outputPath, width, height);
        if ((fp[2]=osal_fopen(strFile, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, strFile);
            goto ERR_OPEN_DISP_BUFFER_FILE;
        }
        sprintf(strFile, "%s_%dx%d_fbc_table_c.bin", outputPath, width, height);
        if ((fp[3]=osal_fopen(strFile, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, strFile);
            goto ERR_OPEN_DISP_BUFFER_FILE;
        }
    }
    return TRUE;
ERR_OPEN_DISP_BUFFER_FILE:
    CloseDisplayBufferFile(fp);
    return FALSE;
}

void CloseDisplayBufferFile(FILE *fp[])
{
    int i;
    for (i=0; i < OUTPUT_FP_NUMBER; i++) {
        if (fp[i] != NULL) {
            osal_fclose(fp[i]);
            fp[i] = NULL;
        }
    }
}
#endif

Int32 CalculateAuxBufferSize(AUX_BUF_TYPE type, CodStd codStd, Int32 width, Int32 height)
{
    Int32 size = 0;

    switch (type) {
    case AUX_BUF_TYPE_MVCOL:
        if (codStd == STD_AVC || codStd == STD_VC1 || codStd == STD_MPEG4 || codStd == STD_H263 || codStd == STD_RV || codStd == STD_AVS) {
            size = VPU_ALIGN32(width)*VPU_ALIGN32(height);
            size = (size * 3) / 2;
            size = (size + 4) / 5;
            size = ((size + 7) / 8) * 8;
        }
        else if (codStd == STD_HEVC) {
            size = WAVE5_DEC_HEVC_MVCOL_BUF_SIZE(width, height);
        }
        else if (codStd == STD_VP9) {
            size = WAVE5_DEC_VP9_MVCOL_BUF_SIZE(width, height);
        }
        else if (codStd == STD_AVS2) {
            size = WAVE5_DEC_AVS2_MVCOL_BUF_SIZE(width, height);
        }
        else if (codStd == STD_AV1) {
            size = WAVE5_DEC_AV1_MVCOL_BUF_SIZE(width, height);
        }
        else {
            size = 0;
        }
        break;
    case AUX_BUF_TYPE_FBC_Y_OFFSET:
        size = WAVE5_FBC_LUMA_TABLE_SIZE(width, height);
        break;
    case AUX_BUF_TYPE_FBC_C_OFFSET:
        size = WAVE5_FBC_CHROMA_TABLE_SIZE(width, height);
        break;
    }

    return size;
}

RetCode GetFBCOffsetTableSize(CodStd codStd, int width, int height, int* ysize, int* csize)
{
    if (ysize == NULL || csize == NULL)
        return RETCODE_INVALID_PARAM;

    *ysize = CalculateAuxBufferSize(AUX_BUF_TYPE_FBC_Y_OFFSET, codStd, width, height);
    *csize = CalculateAuxBufferSize(AUX_BUF_TYPE_FBC_C_OFFSET, codStd, width, height);

    return RETCODE_SUCCESS;
}

RetCode GetPVRICRealSize(CodStd codStd, TiledMapType mapType, FrameBufferFormat format, Uint32 width, Uint32 height, int* ysize, int* csize, Uint32* offset_luma, Uint32* offset_chroma, Uint32 frameWidth, Uint32 frameHeight)
{
    Uint32 total_tile_num, hdr_size, data_size;
    Uint32 frame_total_tile_num, frame_hdr_size;

    if (ysize == NULL || csize == NULL)
        return RETCODE_INVALID_PARAM;

    switch (format) {
    case FORMAT_420:
        total_tile_num = (VPU_ALIGN64(width) >> 6) *(VPU_ALIGN4(height) >> 2);//width
        frame_total_tile_num = (VPU_ALIGN64(frameWidth) >> 6) *(VPU_ALIGN4(frameHeight) >> 2);//width
        break;
    default: /* 10bit */
        total_tile_num = (width+47)/48 * (VPU_ALIGN4(height) >> 2);
        frame_total_tile_num = (frameWidth+47)/48 * (VPU_ALIGN4(frameHeight) >> 2);
        break;
    }
    hdr_size = VPU_ALIGN256(total_tile_num);
    frame_hdr_size = VPU_ALIGN256(frame_total_tile_num);

    if (frame_hdr_size >= hdr_size) {
        *offset_luma = frame_hdr_size - hdr_size;
    }
    if( mapType == PVRIC_COMPRESSED_FRAME_LOSSLESS_MAP ) {
        data_size = (total_tile_num*256); // byte
    }
    else {
        data_size = (total_tile_num*128); // byte
    }
    *ysize = (hdr_size + data_size);

    VLOG(INFO, "<%s> Input WidthxHeight : %dx%d \t luma h=%d(%x), d=%d(%x)\n", __FUNCTION__, width, height, hdr_size, hdr_size, data_size, data_size);

    switch (format) {
    case FORMAT_420:
        total_tile_num = (VPU_ALIGN64(width) >> 6) *(VPU_ALIGN4((height+1)/2) >> 2);
        frame_total_tile_num = (VPU_ALIGN64(frameWidth) >> 6) *(VPU_ALIGN4((frameHeight+1)/2) >> 2);
        break;
    default: /* 10bit */
        total_tile_num = (width+47)/48 * (VPU_ALIGN4((height+1)/2) >> 2);
        frame_total_tile_num = (frameWidth+47)/48 * (VPU_ALIGN4((frameHeight+1)/2) >> 2);
        break;
    }
    hdr_size = VPU_ALIGN256(total_tile_num);
    frame_hdr_size = VPU_ALIGN256(frame_total_tile_num);
    if (frame_hdr_size >= hdr_size) {
        *offset_chroma = frame_hdr_size - hdr_size;
    }
    if( mapType == PVRIC_COMPRESSED_FRAME_LOSSLESS_MAP ) {
        data_size = (total_tile_num*256); // byte
    }
    else {
        data_size = (total_tile_num*128); // byte
    }
    *csize = (hdr_size + data_size);

    VLOG(INFO, "<%s> chroma h=%d(%x), d=%d(%x)\n", __FUNCTION__, hdr_size, hdr_size, data_size, data_size);
    return RETCODE_SUCCESS;
}

void ProcessVc1MultiResolution(
    Uint8* image,
    Int32  width,
    Int32  height,
    BOOL   horz_half,
    BOOL   vert_half
    )
{
    Int32 disX, disY;
    Int32 x, y;

    if (horz_half == 0 && vert_half == 0) {
        return;
    }

    disX = horz_half ? ((width+31)&~31)/2 : width;
    disY = vert_half ? ((height+31)&~31)/2 : height;

    for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {
            if (x >= disX || y >= disY)
                image[x+y*width] = 0;
        }
    }

    /* Cb */
    image += width * height;

    width  /= 2;
    height /= 2;
    disX /= 2;
    disY /= 2;

    for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {
            if (x >= disX || y >= disY) image[x+y*width] = 0;
        }
    }

    /* Cr */
    image += width * height;

    for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {
            if (x >= disX || y >= disY) image[x+y*width] = 0;
        }
    }
}

void PrintDecSeqWarningMessages(
    Uint32          productId,
    DecInitialInfo* seqInfo
    )
{
    if (PRODUCT_ID_W_SERIES(productId))
    {
        if (seqInfo->seqInitErrReason&0x00000001) VLOG(WARN, "sps_max_sub_layer_minus1 shall be 0 to 6\n");
        if (seqInfo->seqInitErrReason&0x00000002) VLOG(WARN, "general_reserved_zero_44bits shall be 0.\n");
        if (seqInfo->seqInitErrReason&0x00000004) VLOG(WARN, "reserved_zero_2bits shall be 0\n");
        if (seqInfo->seqInitErrReason&0x00000008) VLOG(WARN, "sub_layer_reserved_zero_44bits shall be 0");
        if (seqInfo->seqInitErrReason&0x00000010) VLOG(WARN, "general_level_idc shall have one of level of Table A.1\n");
        if (seqInfo->seqInitErrReason&0x00000020) VLOG(WARN, "sps_max_dec_pic_buffering[i] <= MaxDpbSize\n");
        if (seqInfo->seqInitErrReason&0x00000040) VLOG(WARN, "trailing bits shall be 1000... pattern, 7.3.2.1\n");
        if (seqInfo->seqInitErrReason&0x00100000) VLOG(WARN, "Not supported or undefined profile: %d\n", seqInfo->profile);
        if (seqInfo->seqInitErrReason&0x00200000) VLOG(WARN, "Spec over level(%d)\n", seqInfo->level);
    }
}

void DisplayDecodedInformationForHevc(
    DecHandle      handle,
    Uint32         frameNo,
    BOOL           performance,
    DecOutputInfo* decodedInfo
    )
{
    Int32 logLevel = TRACE;
    QueueStatusInfo queueStatus;
    PhysicalAddress frameSize = 0, frameStAddr = 0, frameEdAddr = 0;

    VPU_DecGiveCommand(handle, DEC_GET_QUEUE_STATUS, &queueStatus);

    if (decodedInfo == NULL) {
        if ( performance == TRUE ) {
            VLOG(logLevel, "                                                                                                                    | FRAME  |  HOST | SEEK_S SEEK_E    SEEK  | PARSE_S PARSE_E  PARSE  | DEC_S  DEC_E   DEC   |\n");
            VLOG(logLevel, "I    NO  T     POC     NAL DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  TEMP | CYCLE  |  TICK |  TICK   TICK     CYCLE |  TICK    TICK    CYCLE  |  TICK   TICK   CYCLE | RQ IQ VCore\n");
            VLOG(logLevel, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        }
        else {
            VLOG(logLevel, "I    NO  T     POC     NAL DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  TEMP  CYCLE  (   Seek,   Parse,    Dec)    RQ IQ VCore\n");
            VLOG(logLevel, "---------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        }
    }
    else {
        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? ERR : TRACE;
        frameStAddr = decodedInfo->bytePosFrameStart;
        frameEdAddr = decodedInfo->bytePosFrameEnd;
        frameSize = (frameEdAddr > frameStAddr) ? (frameEdAddr - frameStAddr) : (frameStAddr - frameEdAddr);
        // Print informations
        if ( performance == TRUE ) {
            VLOG(logLevel, "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %4d  %8d %8d (%6d,%6d,%8d) (%6d,%6d,%8d) (%6d,%6d,%8d) %d %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->decodedPOC, decodedInfo->displayPOC, decodedInfo->nalType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->temporalId,
                decodedInfo->frameCycle, decodedInfo->decHostCmdTick,
                decodedInfo->decSeekStartTick, decodedInfo->decSeekEndTick, decodedInfo->seekCycle,
                decodedInfo->decParseStartTick, decodedInfo->decParseEndTick, decodedInfo->parseCycle,
                decodedInfo->decDecodeStartTick, decodedInfo->decDecodeEndTick, decodedInfo->DecodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount,
                0
                );
        }
        else {
            VLOG(logLevel, "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %4d  %8d(%8d,%8d,%8d) %d %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->decodedPOC, decodedInfo->displayPOC, decodedInfo->nalType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->temporalId,
                decodedInfo->frameCycle, decodedInfo->seekCycle, decodedInfo->parseCycle, decodedInfo->DecodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount,
                0
                );
        }
        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
    }
}

void DisplayDecodedInformationForAVS2(
    DecHandle      handle,
    Uint32         frameNo,
    BOOL           performance,
    DecOutputInfo* decodedInfo
    )
{
    Int32 logLevel;
    QueueStatusInfo queueStatus;
    PhysicalAddress frameSize = 0, frameStAddr = 0, frameEdAddr = 0;

    VPU_DecGiveCommand(handle, DEC_GET_QUEUE_STATUS, &queueStatus);

    if (decodedInfo == NULL) {
        if ( performance == TRUE ) {
            VLOG(INFO, "                                                                                                                    | FRAME  |  HOST | SEEK_S SEEK_E    SEEK  | PARSE_S PARSE_E  PARSE  | DEC_S  DEC_E   DEC   |\n");
            VLOG(INFO, "I    NO  T     POC     NAL DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  TEMP | CYCLE  |  TICK |  TICK   TICK     CYCLE |  TICK    TICK    CYCLE  |  TICK   TICK   CYCLE | RQ IQ\n");
        } else {
            VLOG(INFO, "I    NO  T     POC     NAL DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  TEMP  CYCLE (Seek, Parse, Dec) RQ IQ\n");
        }
        VLOG(INFO, "------------------------------------------------------------------------------------------------------------\n");
    }
    else {
        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? ERR : TRACE;
        frameStAddr = decodedInfo->bytePosFrameStart;
        frameEdAddr = decodedInfo->bytePosFrameEnd;
        frameSize = (frameEdAddr > frameStAddr) ? (frameEdAddr - frameStAddr) : (frameStAddr - frameEdAddr);
        // Print informations
        if (performance == TRUE) {
            VLOG(logLevel, "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %4d  %8d %8d (%6d,%6d,%8d) (%6d,%6d,%8d) (%6d,%6d,%8d) %d %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->avs2Info.decodedPOI, decodedInfo->avs2Info.displayPOI, decodedInfo->nalType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->temporalId,
                decodedInfo->frameCycle, decodedInfo->decHostCmdTick,
                decodedInfo->decSeekStartTick, decodedInfo->decSeekEndTick, decodedInfo->seekCycle,
                decodedInfo->decParseStartTick, decodedInfo->decParseEndTick, decodedInfo->parseCycle,
                decodedInfo->decDecodeStartTick, decodedInfo->decDecodeEndTick, decodedInfo->DecodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount,
                0
                );
        }
        else {
            VLOG(logLevel, "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %4d  %8d(%8d,%8d,%8d) %d %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->avs2Info.decodedPOI, decodedInfo->avs2Info.displayPOI, decodedInfo->nalType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->temporalId,
                decodedInfo->frameCycle, decodedInfo->seekCycle, decodedInfo->parseCycle, decodedInfo->DecodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount,
               0
               );
        }
        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
    }
}

void DisplayDecodedInformationForVP9(
    DecHandle handle,
    Uint32 frameNo,
    BOOL           performance,
    DecOutputInfo* decodedInfo)
{
    Int32 logLevel;
    QueueStatusInfo queueStatus;
    PhysicalAddress frameSize = 0, frameStAddr = 0, frameEdAddr = 0;

    VPU_DecGiveCommand(handle, DEC_GET_QUEUE_STATUS, &queueStatus);

    if (decodedInfo == NULL) {
        // Print header
        if ( performance == TRUE ) {
            VLOG(INFO, "                                                                                                | FRAME  |  HOST | SEEK_S SEEK_E    SEEK  | PARSE_S PARSE_E  PARSE  | DEC_S  DEC_E   DEC   |\n");
            VLOG(INFO, "I    NO  T  DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  | CYCLE  |  TICK |  TICK   TICK     CYCLE |  TICK    TICK    CYCLE  |  TICK   TICK   CYCLE | RQ IQ\n");
        }
        else {
            VLOG(INFO, "I    NO  T  DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  CYCLE (Seek, Parse, Dec)  RQ IQ\n");
        }
        VLOG(INFO, "--------------------------------------------------------------------------------------------\n");
    }
    else {
        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? ERR : TRACE;
        frameStAddr = decodedInfo->bytePosFrameStart;
        frameEdAddr = decodedInfo->bytePosFrameEnd;
        frameSize = (frameEdAddr > frameStAddr) ? (frameEdAddr - frameStAddr) : (frameStAddr - frameEdAddr);
        // Print informations
        if (performance == TRUE) {
            VLOG(logLevel, "%02d %5d %d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d %8d %8d (%6d,%6d,%8d) (%6d,%6d,%8d) (%6d,%6d,%8d) %d %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->frameCycle, decodedInfo->decHostCmdTick,
                decodedInfo->decSeekStartTick, decodedInfo->decSeekEndTick, decodedInfo->seekCycle,
                decodedInfo->decParseStartTick, decodedInfo->decParseEndTick, decodedInfo->parseCycle,
                decodedInfo->decDecodeStartTick, decodedInfo->decDecodeEndTick, decodedInfo->DecodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount,
                0
                );
        }
        else {
            VLOG(logLevel, "%02d %5d %d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %8d (%8d,%8d,%8d) %d %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->frameCycle, decodedInfo->seekCycle, decodedInfo->parseCycle, decodedInfo->DecodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount,
                0
                );
        }
        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
    }
}

void DisplayDecodedInformationForAVC(
    DecHandle      handle,
    Uint32         frameNo,
    BOOL           performance,
    DecOutputInfo* decodedInfo)
{
    Int32 logLevel = TRACE;
    QueueStatusInfo queueStatus;
    PhysicalAddress frameSize = 0, frameStAddr = 0, frameEdAddr = 0;

    VPU_DecGiveCommand(handle, DEC_GET_QUEUE_STATUS, &queueStatus);

    if (decodedInfo == NULL) {
        // Print header
        if ( performance == TRUE ) {
            VLOG(logLevel, "                                                                                                                    | FRAME  |  HOST | SEEK_S SEEK_E    SEEK  | PARSE_S PARSE_E  PARSE  | DEC_S  DEC_E   DEC   |\n");
            VLOG(logLevel, "I    NO  T     POC     NAL DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  TEMP | CYCLE  |  TICK |  TICK   TICK     CYCLE |  TICK    TICK    CYCLE  |  TICK   TICK   CYCLE | RQ IQ VCore\n");
        }
        else {
            VLOG(logLevel, "I    NO  T     POC     NAL DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  TEMP  CYCLE  (   Seek,   Parse,    Dec)    RQ IQ VCore\n");
        }
        VLOG(logLevel, "-----------------------------------------------------------------------------------------------------------------------------------------------\n");
    }
    else {
        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? ERR : TRACE;
        frameStAddr = decodedInfo->bytePosFrameStart;
        frameEdAddr = decodedInfo->bytePosFrameEnd;
        frameSize = (frameEdAddr > frameStAddr) ? (frameEdAddr - frameStAddr) : (frameStAddr - frameEdAddr);
        // Print informations
        if (performance == TRUE) {
            VLOG(logLevel, "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %4d  %8d %8d (%6d,%6d,%8d) (%6d,%6d,%8d) (%6d,%6d,%8d) %d %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType, decodedInfo->decodedPOC, decodedInfo->displayPOC, decodedInfo->nalType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo, decodedInfo->temporalId,
                decodedInfo->frameCycle, decodedInfo->decHostCmdTick,
                decodedInfo->decSeekStartTick, decodedInfo->decSeekEndTick, decodedInfo->seekCycle,
                decodedInfo->decParseStartTick, decodedInfo->decParseEndTick, decodedInfo->parseCycle,
                decodedInfo->decDecodeStartTick, decodedInfo->decDecodeEndTick, decodedInfo->DecodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount,
                0
                );
        }
        else {
            VLOG(logLevel, "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %4d  %8d(%8d,%8d,%8d) %d %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType, decodedInfo->decodedPOC, decodedInfo->displayPOC, decodedInfo->nalType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo, decodedInfo->temporalId,
                decodedInfo->frameCycle, decodedInfo->seekCycle, decodedInfo->parseCycle, decodedInfo->DecodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount,
                0
                );
        }

        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
    }
}

void DisplayDecodedInformationForAV1(
    DecHandle handle,
    Uint32 frameNo,
    BOOL           performance,
    DecOutputInfo* decodedInfo)
{
    Int32 logLevel;
    QueueStatusInfo queueStatus;
    PhysicalAddress frameSize = 0, frameStAddr = 0, frameEdAddr = 0;

    VPU_DecGiveCommand(handle, DEC_GET_QUEUE_STATUS, &queueStatus);

    if (decodedInfo == NULL) {
        // Print header
        if ( performance == TRUE ) {
            VLOG(INFO, "                                                                                                | FRAME  |  HOST | SEEK_S SEEK_E    SEEK  | PARSE_S PARSE_E  PARSE  | DEC_S  DEC_E   DEC   |\n");
            VLOG(INFO, "I    NO  T  DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  | CYCLE  |  TICK |  TICK   TICK     CYCLE |  TICK    TICK    CYCLE  |  TICK   TICK   CYCLE | RQ IQ\n");
        }
        else {
            VLOG(INFO, "I    NO  T  DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  CYCLE (Seek, Parse, Dec)  RQ IQ\n");
        }
        VLOG(INFO, "--------------------------------------------------------------------------------------------\n");
    }
    else {
        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? ERR : TRACE;
        frameStAddr = decodedInfo->bytePosFrameStart;
        frameEdAddr = decodedInfo->bytePosFrameEnd;
        frameSize = (frameEdAddr > frameStAddr) ? (frameEdAddr - frameStAddr) : (frameStAddr - frameEdAddr);
        // Print informations
        if (performance == TRUE) {
            VLOG(logLevel, "%02d %5d %d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d %8d %8d (%6d,%6d,%8d) (%6d,%6d,%8d) (%6d,%6d,%8d) %d %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->frameCycle, decodedInfo->decHostCmdTick,
                decodedInfo->decSeekStartTick, decodedInfo->decSeekEndTick, decodedInfo->seekCycle,
                decodedInfo->decParseStartTick, decodedInfo->decParseEndTick, decodedInfo->parseCycle,
                decodedInfo->decDecodeStartTick, decodedInfo->decDecodeEndTick, decodedInfo->DecodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount,
                0
                );
        }
        else {
            VLOG(logLevel, "%02d %5d %d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %8d (%8d,%8d,%8d) %d %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->frameCycle, decodedInfo->seekCycle, decodedInfo->parseCycle, decodedInfo->DecodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount,
                0
                );
        }
        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
    }
}

void DisplayDecodedInformationCommon(
    DecHandle      handle,
    Uint32         frameNo,
    DecOutputInfo* decodedInfo)
{
    Int32 logLevel = TRACE;
    PhysicalAddress frameSize = 0, frameStAddr = 0, frameEdAddr = 0;

    if (decodedInfo == NULL) {
        // Print header
        VLOG(TRACE, "I    NO  T  DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE WxH  \n");
        VLOG(TRACE, "---------------------------------------------------------------------------\n");
    }
    else {
        VpuRect rc    = decodedInfo->rcDisplay;
        Uint32 width  = rc.right - rc.left;
        Uint32 height = rc.bottom - rc.top;
        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? ERR : TRACE;
        frameStAddr = decodedInfo->bytePosFrameStart;
        frameEdAddr = decodedInfo->bytePosFrameEnd;
        frameSize = (frameEdAddr > frameStAddr) ? (frameEdAddr - frameStAddr) : (frameStAddr - frameEdAddr);
        // Print informations
        VLOG(logLevel, "%02d %5d %d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %dx%d\n",
            handle->instIndex, frameNo, decodedInfo->picType,
            decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
            decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
            decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
            decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, frameSize,
            width, height);
        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
    }
}

/**
* \brief                   Print out decoded information such like RD_PTR, WR_PTR, PIC_TYPE, ..
* \param   decodedInfo     If this parameter is not NULL then print out decoded informations
*                          otherwise print out header.
*/
void
    DisplayDecodedInformation(
    DecHandle      handle,
    CodStd         codec,
    Uint32         frameNo,
    DecOutputInfo* decodedInfo,
    ...
    )
{
    int performance = FALSE;
    va_list         ap;

    va_start(ap, decodedInfo);
    performance = va_arg(ap, Uint32);
    va_end(ap);
    switch (codec)
    {
    case STD_HEVC:
        DisplayDecodedInformationForHevc(handle, frameNo, performance, decodedInfo);
        break;
    case STD_VP9:
        DisplayDecodedInformationForVP9(handle, frameNo, performance, decodedInfo);
        break;
    case STD_AVS2:
        DisplayDecodedInformationForAVS2(handle, frameNo, performance, decodedInfo);
        break;
    case STD_AVC:
        DisplayDecodedInformationForAVC(handle, frameNo, performance, decodedInfo);
        break;
    case STD_AV1:
        DisplayDecodedInformationForAV1(handle, frameNo, performance, decodedInfo);
        break;
    default:
        DisplayDecodedInformationCommon(handle, frameNo, decodedInfo);
        break;
    }

    return;
}

void SetDefaultEncTestConfig(TestEncConfig* testConfig) {
    osal_memset(testConfig, 0, sizeof(TestEncConfig));

    testConfig->stdMode        = STD_AVC;
    testConfig->frame_endian   = VPU_FRAME_ENDIAN;
    testConfig->stream_endian  = VPU_STREAM_ENDIAN;
    testConfig->source_endian  = VPU_SOURCE_ENDIAN;
    testConfig->mapType        = COMPRESSED_FRAME_MAP;
    testConfig->lineBufIntEn   = TRUE;
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
    testConfig->srcReleaseIntEnable     = FALSE;
#endif
    testConfig->ringBufferEnable        = FALSE;
    testConfig->ringBufferWrapEnable    = FALSE;

}

static void Wave5DisplayEncodedInformation(
    EncHandle       handle,
    CodStd          codec,
    Uint32          frameNo,
    EncOutputInfo*  encodedInfo,
    Int32           srcEndFlag,
    Int32           srcFrameIdx,
    Int32           performance
    )
{
    QueueStatusInfo queueStatus;
    Int32           logLevel = TRACE;

    VPU_EncGiveCommand(handle, ENC_GET_QUEUE_STATUS, &queueStatus);

    if (encodedInfo == NULL) {
        if (performance == TRUE ) {
            VLOG(logLevel, "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
            VLOG(logLevel, "                                                           USEDSRC            | FRAME  |  HOST  |  PREP_S   PREP_E    PREP   |  PROCE_S   PROCE_E  PROCE  |  ENC_S    ENC_E     ENC    |\n");
            VLOG(logLevel, "I     NO     T   RECON  RD_PTR   WR_PTR     BYTES  SRCIDX  IDX IDC      Vcore | CYCLE  |  TICK  |   TICK     TICK     CYCLE  |   TICK      TICK    CYCLE  |   TICK     TICK     CYCLE  | RQ IQ\n");
            VLOG(logLevel, "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        }
        else {
            VLOG(logLevel, "---------------------------------------------------------------------------------------------------------------------------\n");
            VLOG(logLevel, "                                                              USEDSRC         |                CYCLE\n");
            VLOG(logLevel, "I     NO     T   RECON   RD_PTR    WR_PTR     BYTES  SRCIDX   IDX IDC   Vcore | FRAME PREPARING PROCESSING ENCODING | RQ IQ\n");
            VLOG(logLevel, "---------------------------------------------------------------------------------------------------------------------------\n");
        }
    } else {
        if (performance == TRUE) {
            VLOG(logLevel, "%02d %5d %5d %5d   %08x %08x %8x    %2d     %2d %08x    %2d  %8u %8u (%8u,%8u,%8u) (%8u,%8u,%8u) (%8u,%8u,%8u)   %d  %d\n",
                handle->instIndex, encodedInfo->encPicCnt, encodedInfo->picType, encodedInfo->reconFrameIndex, encodedInfo->rdPtr, encodedInfo->wrPtr,
                encodedInfo->bitstreamSize, (srcEndFlag == 1 ? -1 : srcFrameIdx), encodedInfo->encSrcIdx,
                encodedInfo->releaseSrcFlag,
                0,
                encodedInfo->frameCycle, encodedInfo->encHostCmdTick,
                encodedInfo->encPrepareStartTick, encodedInfo->encPrepareEndTick, encodedInfo->prepareCycle,
                encodedInfo->encProcessingStartTick, encodedInfo->encProcessingEndTick, encodedInfo->processing,
                encodedInfo->encEncodeStartTick, encodedInfo->encEncodeEndTick, encodedInfo->EncodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount);
        }
        else {
            VLOG(logLevel, "%02d %5d %5d %5d    %08x  %08x %8x     %2d     %2d %04x    %d  %8d %8d %8d %8d      %d %d\n",
                handle->instIndex, encodedInfo->encPicCnt, encodedInfo->picType, encodedInfo->reconFrameIndex, encodedInfo->rdPtr, encodedInfo->wrPtr,
                encodedInfo->bitstreamSize, (srcEndFlag == 1 ? -1 : srcFrameIdx), encodedInfo->encSrcIdx,
                encodedInfo->releaseSrcFlag,
                0,
                encodedInfo->frameCycle, encodedInfo->prepareCycle, encodedInfo->processing, encodedInfo->EncodedCycle,
                queueStatus.reportQueueCount, queueStatus.instanceQueueCount);
        }
    }
}

/*lint -esym(438, ap) */
void
    DisplayEncodedInformation(
    EncHandle      handle,
    CodStd         codec,
    Uint32         frameNo,
    EncOutputInfo* encodedInfo,
    ...
    )
{
    int srcEndFlag;
    int srcFrameIdx;
    int performance = FALSE;
    va_list         ap;

    switch (codec) {
    case STD_HEVC:
        va_start(ap, encodedInfo);
        srcEndFlag = va_arg(ap, Uint32);
        srcFrameIdx = va_arg(ap, Uint32);
        performance = va_arg(ap, Uint32);
        va_end(ap);
        Wave5DisplayEncodedInformation(handle, codec, frameNo, encodedInfo, srcEndFlag , srcFrameIdx, performance);
        break;
    case STD_AVC:
        if(handle->productId == PRODUCT_ID_521) {
            va_start(ap, encodedInfo);
            srcEndFlag = va_arg(ap, Uint32);
            srcFrameIdx = va_arg(ap, Uint32);
            performance = va_arg(ap, Uint32);
            va_end(ap);
            Wave5DisplayEncodedInformation(handle, codec, frameNo, encodedInfo, srcEndFlag , srcFrameIdx, performance);
        }
        break;
    default:
        VLOG(INFO, "%s not implemented for codec %d\n", __FUNCTION__, codec);
        break;
    }

    return;
}
/*lint +esym(438, ap) */


void replace_character(char* str,
    char  c,
    char  r)
{
    int i=0;
    int len;
    len = strlen(str);

    for(i=0; i<len; i++)
    {
        if (str[i] == c)
            str[i] = r;
    }
}



void ChangePathStyle(
    char *str
    )
{
}

void ReleaseVideoMemory(
    DecHandle   handle,
    vpu_buffer_t*   memoryArr,
    Uint32          count
    )
{
    Int32       coreIndex = handle->coreIdx;
    Uint32      idx;

    for (idx=0; idx<count; idx++) {
        if (memoryArr[idx].size)
            vdi_free_dma_memory(coreIndex, &memoryArr[idx], DEC_FBC, handle->instIndex);
    }
}


BOOL AllocateDecFrameBuffer(
    DecoderContext_t* ctx,
    Uint32            nonLinearFbCnt,
    Uint32            linearFbCount,
    FrameBuffer*      retFbArray,
    vpu_buffer_t*     retFbAddrs,
    vpu_buffer_t*     retFbWtlAddrs,
    Uint32*           retStride
    )
{
    DecHandle               decHandle = ctx->handle;
    Uint32                  framebufSize;
    Uint32                  totalFbCount, linearFbStartIdx = 0;
    Uint32                  coreIndex;
    Uint32                  idx;
    FrameBufferFormat       format = ctx->wtlFormat;
    DecInitialInfo          seqInfo;
    FrameBufferAllocInfo    fbAllocInfo;
    TiledMapType            mapType = COMPRESSED_FRAME_MAP;
    RetCode                 ret;
    vpu_buffer_t*           pvb;
    size_t                  framebufStride;
    size_t                  framebufHeight;
    Uint32                  productId;
    DRAMConfig*             pDramCfg        = NULL;

    coreIndex = VPU_HANDLE_CORE_INDEX(decHandle);
    productId = VPU_HANDLE_PRODUCT_ID(decHandle);
    VPU_DecGiveCommand(decHandle, DEC_GET_SEQ_INFO, (void*)&seqInfo);

    totalFbCount    = nonLinearFbCnt + linearFbCount;

    codec_sloginfo("%s() -> Total_FBs: %d | decode_FBs: %d | WTL_FBs: %d\n", __FUNCTION__, totalFbCount, nonLinearFbCnt, linearFbCount);

    if (PRODUCT_ID_W_SERIES(productId)) {
        linearFbStartIdx = nonLinearFbCnt;
    }

    if (PRODUCT_ID_W_SERIES(productId)) {
        format = FORMAT_420;
    }

    *retStride     = VPU_ALIGN32(seqInfo.picWidth);
    framebufStride = CalcStride(seqInfo.picWidth, seqInfo.picHeight, format, ctx->decOpenParam.cbcrInterleave, mapType, FALSE);
    framebufHeight = VPU_ALIGN32(seqInfo.picHeight);
    framebufSize   = VPU_GetFrameBufSize(decHandle, decHandle->coreIdx, framebufStride, framebufHeight,
                                         mapType, format, ctx->decOpenParam.cbcrInterleave, pDramCfg);


    osal_memset((void*)&fbAllocInfo, 0x00, sizeof(fbAllocInfo));
    osal_memset((void*)retFbArray,   0x00, sizeof(FrameBuffer)*totalFbCount);
    fbAllocInfo.format          = format;
    fbAllocInfo.cbcrInterleave  = ctx->decOpenParam.cbcrInterleave;
    fbAllocInfo.mapType         = mapType;
    fbAllocInfo.stride          = framebufStride;
    fbAllocInfo.height          = framebufHeight;
    fbAllocInfo.size            = framebufSize;
    fbAllocInfo.lumaBitDepth    = seqInfo.lumaBitdepth;
    fbAllocInfo.chromaBitDepth  = seqInfo.chromaBitdepth;
    fbAllocInfo.num             = nonLinearFbCnt;
    fbAllocInfo.endian          = VDI_128BIT_LITTLE_ENDIAN;
    fbAllocInfo.type            = FB_TYPE_CODEC;
    osal_memset((void*)retFbAddrs,    0x00, sizeof(vpu_buffer_t)*nonLinearFbCnt);
    APIDPRINT("ALLOC MEM - FBC data\n");

    for (idx=0; idx<nonLinearFbCnt; idx++) {
        pvb = &retFbAddrs[idx];
        pvb->size = framebufSize;
        if (vdi_allocate_dma_memory(coreIndex, pvb, DEC_FBC, decHandle->instIndex) < 0) {
            VLOG(ERR, "%s:%d fail to allocate frame buffer\n", __FUNCTION__, __LINE__);
            ReleaseVideoMemory(decHandle, retFbAddrs, nonLinearFbCnt);
            return FALSE;
        }
        retFbArray[idx].bufY  = pvb->phys_addr;
        retFbArray[idx].bufCb = (PhysicalAddress)-1;
        retFbArray[idx].bufCr = (PhysicalAddress)-1;
        retFbArray[idx].updateFbInfo = TRUE;
        retFbArray[idx].size  = framebufSize;
        retFbArray[idx].width = seqInfo.picWidth;
    }

    if (nonLinearFbCnt != 0) {
        if ((ret=VPU_DecAllocateFrameBuffer(decHandle, fbAllocInfo, retFbArray)) != RETCODE_SUCCESS) {
            VLOG(ERR, "%s:%d failed to VPU_DecAllocateFrameBuffer(), ret(%d)\n",
                __FUNCTION__, __LINE__, ret);
            ReleaseVideoMemory(decHandle, retFbAddrs, nonLinearFbCnt);
            return FALSE;
        }
    }

    if (ctx->decOpenParam.wtlEnable || (linearFbCount != 0)) {
        size_t  linearStride;
        size_t  picWidth;
        size_t  picHeight;
        size_t  fbHeight;
        FrameBufferFormat outFormat = ctx->wtlFormat;
        picWidth  = seqInfo.picWidth;
        picHeight = seqInfo.picHeight;
        fbHeight  = picHeight;
        mapType = LINEAR_FRAME_MAP;

        linearStride = CalcStride(picWidth, picHeight, outFormat, ctx->decOpenParam.cbcrInterleave, (TiledMapType)mapType, FALSE);
        framebufSize = VPU_GetFrameBufSize(decHandle, coreIndex, linearStride, fbHeight, (TiledMapType)mapType, outFormat, ctx->decOpenParam.cbcrInterleave, pDramCfg);

        for (idx=0; idx<linearFbCount; idx++) {
            pvb = &retFbWtlAddrs[idx];
            pvb->size = framebufSize;
            if (vdi_attach_dma_memory(coreIndex, pvb) < 0) {
                VLOG(ERR, "%s:%d fail to attach to frame buffer\n", __FUNCTION__, __LINE__);
                return FALSE;
            }
            retFbArray[idx+linearFbStartIdx].bufY  = pvb->phys_addr;
            retFbArray[idx+linearFbStartIdx].bufCb = (PhysicalAddress)-1;
            retFbArray[idx+linearFbStartIdx].bufCr = (PhysicalAddress)-1;
            retFbArray[idx+linearFbStartIdx].updateFbInfo = TRUE;
            retFbArray[idx+linearFbStartIdx].size  = framebufSize;
            retFbArray[idx+linearFbStartIdx].width = picWidth;
        }

        fbAllocInfo.nv21    = ctx->decOpenParam.nv21;
        fbAllocInfo.format  = outFormat;
        fbAllocInfo.num     = linearFbCount;
        fbAllocInfo.mapType = (TiledMapType)mapType;
        fbAllocInfo.stride  = linearStride;
        fbAllocInfo.height  = fbHeight;

        ret = VPU_DecAllocateFrameBuffer(decHandle, fbAllocInfo, &retFbArray[linearFbStartIdx]);
        if (ret != RETCODE_SUCCESS) {
            VLOG(ERR, "%s:%d failed to VPU_DecAllocateFrameBuffer() ret:%d\n",
                __FUNCTION__, __LINE__, ret);
            return FALSE;
        }
    }

    return TRUE;
}

BOOL AllocFBMemory(Uint32 coreIdx, vpu_buffer_t *pFbMem, FrameBuffer* pFb,Uint32 memSize, Uint32 memNum, Int32 memTypes, Int32 instIndex)
{
    Uint32 i =0;
    for (i = 0; i < memNum; i++) {
        pFbMem[i].size = memSize;
        if (vdi_allocate_dma_memory(coreIdx, &pFbMem[i], memTypes, instIndex) < 0) {
            VLOG(ERR, "fail to allocate src buffer\n");
            return FALSE;
        }
        pFb[i].bufY         = pFbMem[i].phys_addr;
        pFb[i].bufCb        = (PhysicalAddress) - 1;
        pFb[i].bufCr        = (PhysicalAddress) - 1;
        pFb[i].size         = memSize;
        pFb[i].updateFbInfo = TRUE;
    }
    return TRUE;
}

BOOL SetFbDetails(Uint32 coreIdx, vpu_buffer_t *inbuf, FrameBuffer* pFb, Uint32 memSize, Uint32 memNum)
{
    Uint32 i =0;
    for (i = 0; i < memNum; i++) {
        pFb[i].bufY         = inbuf[i].phys_addr;
        pFb[i].bufCb        = (PhysicalAddress) - 1;
        pFb[i].bufCr        = (PhysicalAddress) - 1;
        pFb[i].size         = memSize;
        pFb[i].updateFbInfo = TRUE;
    }
    return TRUE;
}

RetCode SetUpDecoderOpenParam(
    DecOpenParam*        param,
    tivpu_dec_config_t*  config
    )
{
    Int32   productId;
    RetCode ret = RETCODE_SUCCESS;

    if (NULL == param || NULL == config) {
        ret = RETCODE_INVALID_PARAM;
    } else {
        productId = VPU_GetProductId(config->coreIdx);

        param->bitstreamFormat = config->bitFormat;
        param->coreIdx         = config->coreIdx;
        param->bitstreamMode   = BS_MODE_INTERRUPT;
        param->wtlEnable       = 1;
        param->cbcrInterleave  = config->cbcrInterleave;
        param->nv21            = 0;
        param->cbcrOrder       = 0;
        param->streamEndian    = VPU_STREAM_ENDIAN;
        param->frameEndian     = VPU_FRAME_ENDIAN;

        if (PRODUCT_ID_W_SERIES(productId)) {
            //WAVE
            param->errorConcealMode  = (config->error_conceal == 1) ? 2 : 0;
            param->errorConcealUnit  = 3;
            param->av1Format         = 0;
            param->priExtAddr = vdi_get_axi_ext_addr();
            param->priAxProt  = 0;
            param->priAxCache = 0;
        }
    }

    return ret;
}

/* UNIX style */
#define IS_DIR_SEPARATOR(__c) (__c == '/')

char* GetDirname(
    const char* path
    )
{
    int length;
    int i;
    char* upper_dir;

    if (path == NULL) return NULL;

    length = strlen(path);
    for (i=length-1; i>=0; i--) {
        if (IS_DIR_SEPARATOR(path[i])) break;
    }

    if (i<0) {
        upper_dir = strdup(".");
    } else {
        upper_dir = strdup(path);
        if (upper_dir) {
            upper_dir[i] = 0;
        }
    }

    return upper_dir;
}

char* GetBasename(
    const char* pathname
    )
{
    const char* base = NULL;
    const char* p    = pathname;

    if (p == NULL) {
        return NULL;
    }

    for (base=p; *p; p++) {//lint !e443
        if (IS_DIR_SEPARATOR(*p)) {
            base = p+1;
        }
    }

    return (char*)base;
}

char* GetFileExtension(
    const char* filename
    )
{
    Int32      len;
    Int32      i;

    len = strlen(filename);
    for (i=len-1; i>=0; i--) {
        if (filename[i] == '.') {
            return (char*)&filename[i+1];
        }
    }

    return NULL;
}

BOOL IsEndOfFile(FILE* fp)
{
    BOOL  result = FALSE;
    Int32 idx  = 0;
    char  cTemp;

    // Check current fp pos
    if (osal_feof(fp) != 0) {
        result = TRUE;
    }

    // Check next fp pos
    // Ignore newline character
    do {
        cTemp = fgetc(fp);
        idx++;

        if (osal_feof(fp) != 0) {
            result = TRUE;
            break;
        }
    } while (cTemp == '\n' || cTemp == '\r');

    // Revert fp pos
    idx *= (-1);
    osal_fseek(fp, idx, SEEK_CUR);

    return result;
}

BOOL CalcYuvSize(
    Int32   format,
    Int32   picWidth,
    Int32   picHeight,
    Int32   cbcrInterleave,
    size_t  *lumaSize,
    size_t  *chromaSize,
    size_t  *frameSize,
    Int32   *bitDepth,
    Int32   *packedFormat,
    Int32   *yuv3p4b)
{
    Int32   temp_picWidth;
    Int32   chromaWidth = 0, chromaHeight = 0;

    if ( bitDepth != 0)
        *bitDepth = 0;
    if ( packedFormat != 0)
        *packedFormat = 0;
    if ( yuv3p4b != 0)
        *yuv3p4b = 0;

    if (!lumaSize || !chromaSize || !frameSize )
        return FALSE;

    switch (format)
    {
    case FORMAT_420:
        chromaWidth = (picWidth+1)/2;
        chromaHeight = (picHeight+1)/2;
        *lumaSize = picWidth * picHeight;
        *chromaSize = chromaWidth * chromaHeight / 2;
        *frameSize = picWidth * picHeight * 3 /2;
        break;
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
        if ( packedFormat != 0)
            *packedFormat = 1;
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_224:
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight;
        *frameSize = picWidth * picHeight * 4 / 2;
        break;
    case FORMAT_422:
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight;
        *frameSize = picWidth * picHeight * 4 / 2;
        break;
    case FORMAT_444:
        *lumaSize  = picWidth * picHeight;
        *chromaSize = picWidth * picHeight * 2;
        *frameSize = picWidth * picHeight * 3;
        break;
    case FORMAT_400:
        *lumaSize  = picWidth * picHeight;
        *chromaSize = 0;
        *frameSize = picWidth * picHeight;
        break;
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_422_P10_16BIT_LSB:
        if ( bitDepth != NULL) {
            *bitDepth = 10;
        }
        *lumaSize = picWidth * picHeight * 2;
        *chromaSize = *lumaSize;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_16BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        *lumaSize = picWidth * picHeight * 2;
        chromaWidth = picWidth;
        chromaHeight = picHeight;
        if (picWidth & 0x01) {
            chromaWidth = chromaWidth+1;
        }
        if (picHeight & 0x01) {
            chromaHeight = chromaHeight+1;
        }
        *chromaSize = chromaWidth * chromaHeight;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        if ( packedFormat != 0)
            *packedFormat = 1;
        *lumaSize = picWidth * picHeight * 2;
        *chromaSize = picWidth * picHeight * 2;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        if ( yuv3p4b != 0)
            *yuv3p4b = 1;
        temp_picWidth = VPU_ALIGN32(picWidth);
        chromaWidth = ((VPU_ALIGN16(temp_picWidth/2*(1<<cbcrInterleave))+2)/3*4);
        if ( cbcrInterleave == 1)
        {
            *lumaSize = (temp_picWidth+2)/3*4 * picHeight;
            *chromaSize = chromaWidth * picHeight/2;
        } else {
            *lumaSize = (temp_picWidth+2)/3*4 * picHeight;
            *chromaSize = chromaWidth * picHeight/2*2;
        }
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_422_P10_32BIT_MSB:
    case FORMAT_422_P10_32BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        if ( yuv3p4b != 0)
            *yuv3p4b = 1;
        temp_picWidth = VPU_ALIGN32(picWidth);
        *lumaSize = (VPU_ALIGN16(temp_picWidth)+2)/3*4 * picHeight;
        *chromaSize = *lumaSize;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        if ( packedFormat != 0)
            *packedFormat = 1;
        if ( yuv3p4b != 0)
            *yuv3p4b = 1;
        *frameSize = ((picWidth*2)+2)/3*4 * picHeight;
        *lumaSize = *frameSize/2;
        *chromaSize = *frameSize/2;
        break;
    default:
        *frameSize = picWidth * picHeight * 3 / 2;
        VLOG(ERR, "%s:%d Not supported format(%d)\n", __FILE__, __LINE__, format);
        return FALSE;
    }
    return TRUE;
}

FrameBufferFormat GetPackedFormat (
    int srcBitDepth,
    int packedType,
    int p10bits,
    int msb)
{
    FrameBufferFormat format = FORMAT_YUYV;

    // default pixel format = P10_16BIT_LSB (p10bits = 16, msb = 0)
    if (srcBitDepth == 8) {

        switch(packedType) {
        case PACKED_YUYV:
            format = FORMAT_YUYV;
            break;
        case PACKED_YVYU:
            format = FORMAT_YVYU;
            break;
        case PACKED_UYVY:
            format = FORMAT_UYVY;
            break;
        case PACKED_VYUY:
            format = FORMAT_VYUY;
            break;
        default:
            format = FORMAT_ERR;
        }
    }
    else if (srcBitDepth == 10) {
        switch(packedType) {
        case PACKED_YUYV:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_YUYV_P10_16BIT_LSB : FORMAT_YUYV_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_YUYV_P10_32BIT_LSB : FORMAT_YUYV_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        case PACKED_YVYU:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_YVYU_P10_16BIT_LSB : FORMAT_YVYU_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_YVYU_P10_32BIT_LSB : FORMAT_YVYU_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        case PACKED_UYVY:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_UYVY_P10_16BIT_LSB : FORMAT_UYVY_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_UYVY_P10_32BIT_LSB : FORMAT_UYVY_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        case PACKED_VYUY:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_VYUY_P10_16BIT_LSB : FORMAT_VYUY_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_VYUY_P10_32BIT_LSB : FORMAT_VYUY_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        default:
            format = FORMAT_ERR;
        }
    }
    else {
        format = FORMAT_ERR;
    }

    return format;
}


/******************************************************************************
EncOpenParam Initialization
******************************************************************************/
/**
* To init EncOpenParam by runtime evaluation
* IN
*   tivpu_enc_config_t *pVpuEncConfig
* OUT
*   EncOpenParam *pEncOP
*/
#define DEFAULT_ENC_OUTPUT_NUM      30
Int32 GetEncOpenParamDefault(EncOpenParam *pEncOP, tivpu_enc_config_t *pVpuEncConfig)
{
    int     bitFormat;
    EncWaveParam *param = &pEncOP->EncStdParam.waveParam;
    //pVpuEncConfig->outNum  = pVpuEncConfig->outNum == 0 ? DEFAULT_ENC_OUTPUT_NUM : pVpuEncConfig->outNum;
    Int32   rcBitrate   = pVpuEncConfig->bitrate;
    Int32   i           = 0;

    bitFormat = pVpuEncConfig->bitFormat;

    pEncOP->bitstreamFormat         = bitFormat;
    pEncOP->picWidth                = pVpuEncConfig->width;
    pEncOP->picHeight               = pVpuEncConfig->height;
    pEncOP->frameRateInfo           = (pVpuEncConfig->framerate > 0) ? pVpuEncConfig->framerate : 30;
    pEncOP->meBlkMode               = 0;        // for compare with C-model ( C-model = only 0 )
    //pVpuEncConfig->picQpY              = 23;

    // Standard specific
    if((bitFormat == STD_HEVC) || (bitFormat == STD_AVC)) {

        pEncOP->bitRate         = rcBitrate;

        if(pVpuEncConfig->rateControl > 0)
            pEncOP->rcEnable    = ((rcBitrate == 0) ? 0 : 1);
        else
            pEncOP->rcEnable    = 0;

        param->profile          = 0; /* VPU selects the appropriate profile */
        param->level            = 0; /* VPU selects the appropriate level */
        param->tier             = 0;
        param->internalBitDepth = 8;
        pEncOP->srcBitDepth     = 8;
        pEncOP->outputFormat    = FORMAT_420;
        param->losslessEnable   = 0;
        param->constIntraPredFlag = 0;
        param->useLongTerm  = 0;

        /* for CMD_ENC_SEQ_GOP_PARAM */
        param->gopPresetIdx     = PRESET_IDX_IPP_SINGLE; /**< Consecutive P, cyclic gopsize = 1, with single reference  */

        /* for CMD_ENC_SEQ_INTRA_PARAM */
        param->decodingRefreshType   = ((param->gopPresetIdx == 1) && (pEncOP->rcEnable == 1) && (bitFormat == STD_HEVC)) ? 2 : 1;
        param->intraPeriod           = ((param->gopPresetIdx == 1) && (pEncOP->rcEnable == 1) && (bitFormat == STD_HEVC)) ? 1 : 28;
        param->intraQP               = 30;
        param->forcedIdrHeaderEnable = 0;

        /* for CMD_ENC_SEQ_CONF_WIN_TOP_BOT/LEFT_RIGHT */
        param->confWinTop    = 0;
        param->confWinBot    = 0;
        param->confWinLeft   = 0;
        param->confWinRight  = 0;

        /* for CMD_ENC_SEQ_INDEPENDENT_SLICE */
        param->independSliceMode     = 0;
        param->independSliceModeArg  = 0;

        /* for CMD_ENC_SEQ_DEPENDENT_SLICE */
        param->dependSliceMode     = 0;
        param->dependSliceModeArg  = 0;

        /* for CMD_ENC_SEQ_INTRA_REFRESH_PARAM */
        param->intraRefreshMode     = 0;
        param->intraRefreshArg      = 0;
        param->useRecommendEncParam = 1;

        // pVpuEncConfig->roi_enable = 0;

        /* for CMD_ENC_PARAM */
        param->scalingListEnable    = 0;
        param->cuSizeMode           = 0x7; // always set cu8x8/16x16/32x32 enable to 1
        param->tmvpEnable           = 1;
        param->wppEnable            = 0;
        param->maxNumMerge          = 2;
        param->disableDeblk         = 0;
        param->lfCrossSliceBoundaryEnable = 1;
        param->betaOffsetDiv2       = 0;
        param->tcOffsetDiv2         = 0;
        param->skipIntraTrans       = 1;
        param->saoEnable            = 1;
        param->intraNxNEnable       = 1;

        /* for CMD_ENC_RC_PARAM */
        pEncOP->vbvBufferSize        = 3000;
        param->roiEnable    = 0;
        param->bitAllocMode          = 0;
        for (i = 0; i < MAX_GOP_NUM; i++) {
            param->fixedBitRatio[i] = 1;
        }
        param->cuLevelRCEnable       = 0;
        param->hvsQPEnable           = 1;
        param->hvsQpScale            = 2;

        /* for CMD_ENC_RC_MIN_MAX_QP */
        param->minQpI            = 8;
        param->maxQpI            = 51;
        param->minQpP            = 8;
        param->maxQpP            = 51;
        param->minQpB            = 8;
        param->maxQpB            = 51;

        param->hvsMaxDeltaQp     = 10;

        /* for CMD_ENC_CUSTOM_GOP_PARAM */
        param->gopParam.customGopSize     = 0;

        for (i= 0; i<param->gopParam.customGopSize; i++) {
            param->gopParam.picParam[i].picType      = PIC_TYPE_I;
            param->gopParam.picParam[i].pocOffset    = 1;
            param->gopParam.picParam[i].picQp        = 30;
            param->gopParam.picParam[i].refPocL0     = 0;
            param->gopParam.picParam[i].refPocL1     = 0;
            param->gopParam.picParam[i].temporalId   = 0;
        }

        // for VUI / time information.
        param->numTicksPocDiffOne   = 0;
        param->timeScale            = pEncOP->frameRateInfo * 1000;
        if (bitFormat == STD_AVC) {
            param->numUnitsInTick   = 1000/2; /* Must have 1/2 the value for H.264 */
        }
        else {
            param->numUnitsInTick   = 1000;
        }

        param->chromaCbQpOffset = 0;
        param->chromaCrQpOffset = 0;
        param->initialRcQp      = 63;       /* 63 indicates that the firmware will pick the value */
        param->nrYEnable        = 0;
        param->nrCbEnable       = 0;
        param->nrCrEnable       = 0;
        param->nrNoiseEstEnable = 0;

        //pVpuEncConfig->roi_avg_qp             = 0;
        //pVpuEncConfig->lambda_map_enable      = 0;

        param->monochromeEnable            = 0;
        param->strongIntraSmoothEnable     = 1;
        param->weightPredEnable            = 0;
        param->bgDetectEnable              = 0;
        param->bgThrDiff                   = 8;
        param->bgThrMeanDiff               = 1;
        param->bgLambdaQp                  = 32;
        param->bgDeltaQp                   = 3;

        param->customLambdaEnable          = 0;
        param->customMDEnable              = 0;
        param->pu04DeltaRate               = 0;
        param->pu08DeltaRate               = 0;
        param->pu16DeltaRate               = 0;
        param->pu32DeltaRate               = 0;
        param->pu04IntraPlanarDeltaRate    = 0;
        param->pu04IntraDcDeltaRate        = 0;
        param->pu04IntraAngleDeltaRate     = 0;
        param->pu08IntraPlanarDeltaRate    = 0;
        param->pu08IntraDcDeltaRate        = 0;
        param->pu08IntraAngleDeltaRate     = 0;
        param->pu16IntraPlanarDeltaRate    = 0;
        param->pu16IntraDcDeltaRate        = 0;
        param->pu16IntraAngleDeltaRate     = 0;
        param->pu32IntraPlanarDeltaRate    = 0;
        param->pu32IntraDcDeltaRate        = 0;
        param->pu32IntraAngleDeltaRate     = 0;
        param->cu08IntraDeltaRate          = 0;
        param->cu08InterDeltaRate          = 0;
        param->cu08MergeDeltaRate          = 0;
        param->cu16IntraDeltaRate          = 0;
        param->cu16InterDeltaRate          = 0;
        param->cu16MergeDeltaRate          = 0;
        param->cu32IntraDeltaRate          = 0;
        param->cu32InterDeltaRate          = 0;
        param->cu32MergeDeltaRate          = 0;
        param->coefClearDisable            = 0;

        param->rcWeightParam               = 2;
        param->rcWeightBuf                 = 128;
        // for H.264 encoder
        param->avcIdrPeriod                = ((param->gopPresetIdx == 1) && (pEncOP->rcEnable == 1) && (bitFormat == STD_AVC)) ? 1 : 0;
        param->rdoSkip                     = 1;
        param->lambdaScalingEnable         = 1;

        param->transform8x8Enable          = 1;
        param->avcSliceMode                = 0;
        param->avcSliceArg                 = 0;
        param->intraMbRefreshMode          = 0;
        param->intraMbRefreshArg           = 1;
        param->mbLevelRcEnable             = 0;
        param->entropyCodingMode           = 1;
    }
    else {
        VLOG(ERR, "Invalid codec standard mode: bitFormat(%d) \n", bitFormat);
        return 0;
    }

    return 1;
}

int setWaveEncOpenParam(EncOpenParam *pEncOP)
{
    //Int32   i = 0;
    EncWaveParam *param = &pEncOP->EncStdParam.waveParam;

    param->profile          = 0; /* VPU selects the appropriate profile */
    param->level            = 0; /* VPU selects the appropriate level */
    param->tier             = 0;

    param->internalBitDepth = 8;

#if 0
    if (pEncOP->bitstreamFormat == STD_HEVC) {
        if(pCfg->waveCfg.enStillPicture) {
            param->profile   = HEVC_PROFILE_STILLPICTURE;
            param->enStillPicture = 1;
        }
    } else {
        param->profile = pCfg->Profile;
    }

    param->losslessEnable   = pCfg->waveCfg.losslessEnable;
    param->constIntraPredFlag = pCfg->waveCfg.constIntraPredFlag;

    if (pCfg->waveCfg.useAsLongtermPeriod > 0 || pCfg->waveCfg.refLongtermPeriod > 0)
        param->useLongTerm = 1;
    else
        param->useLongTerm = 0;

    /* for CMD_ENC_SEQ_GOP_PARAM */
    param->gopPresetIdx     = pCfg->waveCfg.gopPresetIdx;

    /* for CMD_ENC_SEQ_INTRA_PARAM */
    param->decodingRefreshType   = ((pCfg->waveCfg.gopPresetIdx == 1) && (pCfg->RcEnable == 1) && (pEncOP->bitstreamFormat == STD_HEVC)) ? 2 : pCfg->waveCfg.decodingRefreshType;
    param->intraPeriod           = ((pCfg->waveCfg.gopPresetIdx == 1) && (pCfg->RcEnable == 1) && (pEncOP->bitstreamFormat == STD_HEVC)) ? 1 : pCfg->waveCfg.intraPeriod;
    param->intraQP               = pCfg->waveCfg.intraQP;
	param->forcedIdrHeaderEnable = pCfg->waveCfg.forcedIdrHeaderEnable;

    /* for CMD_ENC_SEQ_CONF_WIN_TOP_BOT/LEFT_RIGHT */
    param->confWinTop    = pCfg->waveCfg.confWinTop;
    param->confWinBot    = pCfg->waveCfg.confWinBot;
    param->confWinLeft   = pCfg->waveCfg.confWinLeft;
    param->confWinRight  = pCfg->waveCfg.confWinRight;

    /* for CMD_ENC_SEQ_INDEPENDENT_SLICE */
    param->independSliceMode     = pCfg->waveCfg.independSliceMode;
    param->independSliceModeArg  = pCfg->waveCfg.independSliceModeArg;

    /* for CMD_ENC_SEQ_DEPENDENT_SLICE */
    param->dependSliceMode     = pCfg->waveCfg.dependSliceMode;
    param->dependSliceModeArg  = pCfg->waveCfg.dependSliceModeArg;

    /* for CMD_ENC_SEQ_INTRA_REFRESH_PARAM */
    param->intraRefreshMode     = pCfg->waveCfg.intraRefreshMode;
    param->intraRefreshArg      = pCfg->waveCfg.intraRefreshArg;
    param->useRecommendEncParam = pCfg->waveCfg.useRecommendEncParam;

    /* for CMD_ENC_PARAM */
    param->scalingListEnable        = pCfg->waveCfg.scalingListEnable;
    param->cuSizeMode               = 0x7; // always set cu8x8/16x16/32x32 enable to 1.
    param->tmvpEnable               = pCfg->waveCfg.tmvpEnable;
    param->wppEnable                = pCfg->waveCfg.wppenable;
    param->maxNumMerge              = pCfg->waveCfg.maxNumMerge;

    param->disableDeblk             = pCfg->waveCfg.disableDeblk;

    param->lfCrossSliceBoundaryEnable   = pCfg->waveCfg.lfCrossSliceBoundaryEnable;
    param->betaOffsetDiv2           = pCfg->waveCfg.betaOffsetDiv2;
    param->tcOffsetDiv2             = pCfg->waveCfg.tcOffsetDiv2;
    param->skipIntraTrans           = pCfg->waveCfg.skipIntraTrans;
    param->saoEnable                = pCfg->waveCfg.saoEnable;
    param->intraNxNEnable           = pCfg->waveCfg.intraNxNEnable;

    /* for CMD_ENC_RC_PARAM */
    param->cuLevelRCEnable       = pCfg->waveCfg.cuLevelRCEnable;
    param->hvsQPEnable           = pCfg->waveCfg.hvsQPEnable;
    param->hvsQpScale            = pCfg->waveCfg.hvsQpScale;

    param->bitAllocMode          = pCfg->waveCfg.bitAllocMode;
    for (i = 0; i < MAX_GOP_NUM; i++) {
        param->fixedBitRatio[i] = pCfg->waveCfg.fixedBitRatio[i];
    }

    param->minQpI           = pCfg->waveCfg.minQp;
    param->minQpP           = pCfg->waveCfg.minQp;
    param->minQpB           = pCfg->waveCfg.minQp;
    param->maxQpI           = pCfg->waveCfg.maxQp;
    param->maxQpP           = pCfg->waveCfg.maxQp;
    param->maxQpB           = pCfg->waveCfg.maxQp;

    param->hvsMaxDeltaQp    = pCfg->waveCfg.maxDeltaQp;

    /* for CMD_ENC_CUSTOM_GOP_PARAM */
    param->gopParam.customGopSize     = pCfg->waveCfg.gopParam.customGopSize;

    for (i= 0; i<param->gopParam.customGopSize; i++) {
        param->gopParam.picParam[i].picType      = pCfg->waveCfg.gopParam.picParam[i].picType;
        param->gopParam.picParam[i].pocOffset    = pCfg->waveCfg.gopParam.picParam[i].pocOffset;
        param->gopParam.picParam[i].picQp        = pCfg->waveCfg.gopParam.picParam[i].picQp;
        param->gopParam.picParam[i].refPocL0     = pCfg->waveCfg.gopParam.picParam[i].refPocL0;
        param->gopParam.picParam[i].refPocL1     = pCfg->waveCfg.gopParam.picParam[i].refPocL1;
        param->gopParam.picParam[i].temporalId   = pCfg->waveCfg.gopParam.picParam[i].temporalId;
        param->gopParam.picParam[i].useMultiRefP = pCfg->waveCfg.gopParam.picParam[i].useMultiRefP;
    }

    param->roiEnable = pCfg->waveCfg.roiEnable;

    // VPS & VUI
    param->numTicksPocDiffOne   = pCfg->waveCfg.numTicksPocDiffOne;
    pEncOP->encodeVuiRbsp       = pCfg->waveCfg.vuiDataEnable;
    pEncOP->vuiRbspDataSize     = pCfg->waveCfg.vuiDataSize;
    pEncOP->encodeHrdRbspInVPS  = pCfg->waveCfg.hrdInVPS;
    pEncOP->hrdRbspDataSize     = pCfg->waveCfg.hrdDataSize;
    param->chromaCbQpOffset = pCfg->waveCfg.chromaCbQpOffset;
    param->chromaCrQpOffset = pCfg->waveCfg.chromaCrQpOffset;
    param->initialRcQp      = pCfg->waveCfg.initialRcQp;

    param->nrYEnable        = pCfg->waveCfg.nrYEnable;
    param->nrCbEnable       = pCfg->waveCfg.nrCbEnable;
    param->nrCrEnable       = pCfg->waveCfg.nrCrEnable;
    param->nrNoiseEstEnable = pCfg->waveCfg.nrNoiseEstEnable;
    param->nrNoiseSigmaY    = pCfg->waveCfg.nrNoiseSigmaY;
    param->nrNoiseSigmaCb   = pCfg->waveCfg.nrNoiseSigmaCb;
    param->nrNoiseSigmaCr   = pCfg->waveCfg.nrNoiseSigmaCr;
    param->nrIntraWeightY   = pCfg->waveCfg.nrIntraWeightY;
    param->nrIntraWeightCb  = pCfg->waveCfg.nrIntraWeightCb;
    param->nrIntraWeightCr  = pCfg->waveCfg.nrIntraWeightCr;
    param->nrInterWeightY   = pCfg->waveCfg.nrInterWeightY;
    param->nrInterWeightCb  = pCfg->waveCfg.nrInterWeightCb;
    param->nrInterWeightCr  = pCfg->waveCfg.nrInterWeightCr;

    param->monochromeEnable            = pCfg->waveCfg.monochromeEnable;
    param->strongIntraSmoothEnable     = pCfg->waveCfg.strongIntraSmoothEnable;
    param->weightPredEnable            = pCfg->waveCfg.weightPredEnable;
    param->bgDetectEnable              = pCfg->waveCfg.bgDetectEnable;
    param->bgThrDiff                   = pCfg->waveCfg.bgThrDiff;
    param->bgThrMeanDiff               = pCfg->waveCfg.bgThrMeanDiff;
    param->bgLambdaQp                  = pCfg->waveCfg.bgLambdaQp;
    param->bgDeltaQp                   = pCfg->waveCfg.bgDeltaQp;
    param->customLambdaEnable          = pCfg->waveCfg.customLambdaEnable;
    param->customMDEnable              = pCfg->waveCfg.customMDEnable;
    param->pu04DeltaRate               = pCfg->waveCfg.pu04DeltaRate;
    param->pu08DeltaRate               = pCfg->waveCfg.pu08DeltaRate;
    param->pu16DeltaRate               = pCfg->waveCfg.pu16DeltaRate;
    param->pu32DeltaRate               = pCfg->waveCfg.pu32DeltaRate;
    param->pu04IntraPlanarDeltaRate    = pCfg->waveCfg.pu04IntraPlanarDeltaRate;
    param->pu04IntraDcDeltaRate        = pCfg->waveCfg.pu04IntraDcDeltaRate;
    param->pu04IntraAngleDeltaRate     = pCfg->waveCfg.pu04IntraAngleDeltaRate;
    param->pu08IntraPlanarDeltaRate    = pCfg->waveCfg.pu08IntraPlanarDeltaRate;
    param->pu08IntraDcDeltaRate        = pCfg->waveCfg.pu08IntraDcDeltaRate;
    param->pu08IntraAngleDeltaRate     = pCfg->waveCfg.pu08IntraAngleDeltaRate;
    param->pu16IntraPlanarDeltaRate    = pCfg->waveCfg.pu16IntraPlanarDeltaRate;
    param->pu16IntraDcDeltaRate        = pCfg->waveCfg.pu16IntraDcDeltaRate;
    param->pu16IntraAngleDeltaRate     = pCfg->waveCfg.pu16IntraAngleDeltaRate;
    param->pu32IntraPlanarDeltaRate    = pCfg->waveCfg.pu32IntraPlanarDeltaRate;
    param->pu32IntraDcDeltaRate        = pCfg->waveCfg.pu32IntraDcDeltaRate;
    param->pu32IntraAngleDeltaRate     = pCfg->waveCfg.pu32IntraAngleDeltaRate;
    param->cu08IntraDeltaRate          = pCfg->waveCfg.cu08IntraDeltaRate;
    param->cu08InterDeltaRate          = pCfg->waveCfg.cu08InterDeltaRate;
    param->cu08MergeDeltaRate          = pCfg->waveCfg.cu08MergeDeltaRate;
    param->cu16IntraDeltaRate          = pCfg->waveCfg.cu16IntraDeltaRate;
    param->cu16InterDeltaRate          = pCfg->waveCfg.cu16InterDeltaRate;
    param->cu16MergeDeltaRate          = pCfg->waveCfg.cu16MergeDeltaRate;
    param->cu32IntraDeltaRate          = pCfg->waveCfg.cu32IntraDeltaRate;
    param->cu32InterDeltaRate          = pCfg->waveCfg.cu32InterDeltaRate;
    param->cu32MergeDeltaRate          = pCfg->waveCfg.cu32MergeDeltaRate;
    param->coefClearDisable            = pCfg->waveCfg.coefClearDisable;

    param->rcWeightParam               = pCfg->waveCfg.rcWeightParam;
    param->rcWeightBuf                 = pCfg->waveCfg.rcWeightBuf;

    param->s2fmeDisable                = pCfg->waveCfg.s2fmeDisable;
    // for H.264 on WAVE
    param->avcIdrPeriod         = ((pCfg->waveCfg.gopPresetIdx == 1) && (pCfg->RcEnable == 1) && (pEncOP->bitstreamFormat == STD_AVC)) ? 1 : pCfg->waveCfg.idrPeriod;
    param->rdoSkip              = pCfg->waveCfg.rdoSkip;
    param->lambdaScalingEnable  = pCfg->waveCfg.lambdaScalingEnable;
    param->transform8x8Enable   = pCfg->waveCfg.transform8x8;
    param->avcSliceMode         = pCfg->waveCfg.avcSliceMode;
    param->avcSliceArg          = pCfg->waveCfg.avcSliceArg;
    param->intraMbRefreshMode   = pCfg->waveCfg.intraMbRefreshMode;
    param->intraMbRefreshArg    = pCfg->waveCfg.intraMbRefreshArg;
    param->mbLevelRcEnable      = pCfg->waveCfg.mbLevelRc;
    param->entropyCodingMode    = pCfg->waveCfg.entropyCodingMode;
#endif

    return 1;
}


BOOL SetupEncoderOpenParam(
    EncOpenParam*        param,
    tivpu_enc_config_t*  config
    )
{
    EncWaveParam *waveparam = &param->EncStdParam.waveParam;
    FrameBufferFormat packedFormat;
    Int32             productId;
    Int32             framesize;
    productId = VPU_GetProductId(config->coreIdx);

    if (GetEncOpenParamDefault(param, config) == FALSE) {
        VLOG(ERR, "VPU: [ERROR] Failed to parse CFG params (GetEncOpenParamDefault)\n");
        return FALSE;
    }

    if ((config->setGOP)) {
        waveparam->gopPresetIdx = config->setGOP;
    }

    // JB: Setting the out buffer size
    framesize = (Int32) (config->width * config->height * 1);

    param->streamBufCount = config->streamBufCount ? config->streamBufCount : ENC_STREAM_BUF_COUNT;
    param->sourceBufCount = config->sourceBufCount ? config->sourceBufCount : MIN_IN_BUFFERS;

    if (config->streamBufSize != 0) {
        param->streamBufSize  = config->streamBufSize;
    }
    else {
        param->streamBufSize  = framesize; //ENC_STREAM_BUF_SIZE;
        config->streamBufSize = framesize; //ENC_STREAM_BUF_SIZE;
    }

    /* YUV420 is the default color format */
    param->srcFormat = FORMAT_420;
    param->nv21      = 0; /* only NV12 is supported currently */

    param->cbcrInterleave = config->cbcrInterleave;
    param->packedFormat   = config->packedFormat;

    if (param->packedFormat >= PACKED_YUYV) {
        packedFormat = GetPackedFormat(param->srcBitDepth, param->packedFormat, 0, 1);
        if (packedFormat == -1) {
            VLOG(ERR, "VPU: [ERROR] Failed to GetPackedFormat\n");
            return FALSE;
        }
        param->srcFormat      = packedFormat;
        param->cbcrInterleave = 0;
        param->packedFormat   = 0;
    }

    param->frameEndian    = VPU_FRAME_ENDIAN;
    param->streamEndian   = VPU_STREAM_ENDIAN;
    param->sourceEndian   = VPU_SOURCE_ENDIAN;
    param->lineBufIntEn   = TRUE;
    param->coreIdx        = config->coreIdx;
    param->cbcrOrder      = CBCR_ORDER_NORMAL;

    param->EncStdParam.waveParam.useLongTerm = 0;
    param->priExtAddr = vdi_get_axi_ext_addr();
    param->priAxProt  = 0;
    param->priAxCache = 0;
    if (PRODUCT_ID_W_SERIES(productId)) {
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
        param->srcReleaseIntEnable    = FALSE;
#endif
        param->ringBufferEnable       = FALSE;
        param->ringBufferWrapEnable   = FALSE;
        if (param->ringBufferEnable == TRUE) {
            param->streamBufCount = 1;
            param->lineBufIntEn = FALSE;
        }
    }

    param->subFrameSyncEnable     = FALSE;
    param->subFrameSyncMode       = REGISTER_BASE_SUB_FRAME_SYNC;// default mode = register based for Ref.SW

    /** Bit flags for encoding features */
    param->EncStdParam.waveParam.entropyCodingMode = 1; /* CABAC = 1 */

    /** Rate Control */
    if ((config->rateControl > 0) && (config->bitrate > 0)) {
        param->rcEnable = 1;
        param->EncStdParam.waveParam.cuLevelRCEnable = 1; /* applicable to HEVC encode */
        param->EncStdParam.waveParam.mbLevelRcEnable = 1; /* applicable to AVC encode */
        if (config->rateControl == 2) /* CBR mode */
            param->vbvBufferSize = 1000; /* Change vbvBufferSize away from the default(VBR) of 3000 */
    }
    else {
        param->rcEnable = 0;
    }

    /** IDR-period */
    if (param->bitstreamFormat == STD_AVC) {
        param->EncStdParam.waveParam.avcIdrPeriod = config->keyFrameInterval ? config->keyFrameInterval : 0;
    }
    else {
        param->EncStdParam.waveParam.avcIdrPeriod = 0;
    }
    /** I-period */
    param->EncStdParam.waveParam.intraPeriod = config->keyFrameInterval ? config->keyFrameInterval : 0;
    /** Bitrate */
    param->bitRate = config->bitrate; /* Allow both non-zero bitrates and zero bitrate (in case of RC disabled) */

    /* Push down any additional waveParam level params not already covered above */
    if (setWaveEncOpenParam(param) == 0) {
        VLOG(ERR, "VPU: [ERROR] Failed to setWaveEncOpenParam\n");
        return FALSE;
    }

    if ((config->setLossless)) {
        param->rcEnable = 0;
        param->EncStdParam.waveParam.losslessEnable = 1;
        param->EncStdParam.waveParam.saoEnable = 0;
        param->EncStdParam.waveParam.bgDetectEnable = 0;
        param->EncStdParam.waveParam.disableDeblk = 1;
    }
    return TRUE;
}

void GenRegionToMap(
    VpuRect *region,        /**< The size of the ROI region for H.265 (start X/Y in CTU, end X/Y int CTU)  */
    int *roiQp,
    int num,
    Uint32 mapWidth,
    Uint32 mapHeight,
    Uint8 *roiCtuMap)
{
    Int32 roi_id, blk_addr;
    Uint32 roi_map_size      = mapWidth * mapHeight;

    //init roi map
    for (blk_addr=0; blk_addr<(Int32)roi_map_size; blk_addr++)
        roiCtuMap[blk_addr] = 0;

    //set roi map. roi_entry[i] has higher priority than roi_entry[i+1]
    for (roi_id=(Int32)num-1; roi_id>=0; roi_id--)
    {
        Uint32 x, y;
        VpuRect *roi = region + roi_id;

        for (y=roi->top; y<=roi->bottom; y++)
        {
            for (x=roi->left; x<=roi->right; x++)
            {
                roiCtuMap[y*mapWidth + x] = *(roiQp + roi_id);
            }
        }
    }
}


void GenRegionToQpMap(
    VpuRect *region,        /**< The size of the ROI region for H.265 (start X/Y in CTU, end X/Y int CTU)  */
    int *roiQp,
    int num,
    int initQp,
    Uint32 mapWidth,
    Uint32 mapHeight,
    Uint8 *roiCtuMap)
{
    Int32 roi_id, blk_addr;
    Uint32 roi_map_size      = mapWidth * mapHeight;

    //init roi map
    for (blk_addr=0; blk_addr<(Int32)roi_map_size; blk_addr++)
        roiCtuMap[blk_addr] = initQp;

    //set roi map. roi_entry[i] has higher priority than roi_entry[i+1]
    for (roi_id=(Int32)num-1; roi_id>=0; roi_id--)
    {
        Uint32 x, y;
        VpuRect *roi = region + roi_id;

        for (y=roi->top; y<=roi->bottom; y++)
        {
            for (x=roi->left; x<=roi->right; x++)
            {
                roiCtuMap[y*mapWidth + x] = *(roiQp + roi_id);
            }
        }
    }
}

// These helper functions have created build issues in SDP 8.0. While these functions have been updated
// to compile in SDP 8.0, they were not used so pending verification that these functions are unused
// via code coverage utilities, they will be removed.
#if 0
Int32 writeVuiRbsp(int coreIdx, TestEncConfig *encConfig, EncOpenParam *encOP, vpu_buffer_t *vbVuiRbsp)
{
    if (encOP->encodeVuiRbsp == TRUE) {
        vbVuiRbsp->size = VUI_HRD_RBSP_BUF_SIZE;

        if (vdi_allocate_dma_memory(coreIdx, vbVuiRbsp, ENC_ETC, 0) < 0) {//I don't know instIndex before Calling VpuEncOpen
            VLOG(ERR, "fail to allocate VUI rbsp buffer\n" );
            return FALSE;
        }
        encOP->vuiRbspDataAddr = vbVuiRbsp->phys_addr;

        Uint8   *pVuiRbspBuf;
        Int32   rbspSizeInByte = (encOP->vuiRbspDataSize+7)>>3;
        ChangePathStyle(encConfig->vui_rbsp_file_name);
        if ((encConfig->vui_rbsp_fp = osal_fopen(encConfig->vui_rbsp_file_name, "r")) == NULL) {
            VLOG(ERR, "fail to open VUI rbsp Data file, %s\n", encConfig->vui_rbsp_file_name);
            return FALSE;
        }

        if (rbspSizeInByte > (Uint32)VUI_HRD_RBSP_BUF_SIZE)
            VLOG(ERR, "VUI Rbsp size is bigger than buffer size\n");

        pVuiRbspBuf = (Uint8*)osal_malloc(VUI_HRD_RBSP_BUF_SIZE);
        osal_memset(pVuiRbspBuf, 0, VUI_HRD_RBSP_BUF_SIZE);
        osal_fread(pVuiRbspBuf, 1, rbspSizeInByte, encConfig->vui_rbsp_fp);
        vdi_write_memory(coreIdx, encOP->vuiRbspDataAddr, pVuiRbspBuf,  rbspSizeInByte, encOP->streamEndian);
        osal_free(pVuiRbspBuf);
    }
    return TRUE;
}

Int32 writeHrdRbsp(int coreIdx, TestEncConfig *encConfig, EncOpenParam *encOP, vpu_buffer_t *vbHrdRbsp)
{
    if (encOP->encodeHrdRbspInVPS)
    {
        vbHrdRbsp->size    = VUI_HRD_RBSP_BUF_SIZE;
        if (vdi_allocate_dma_memory(coreIdx, vbHrdRbsp, ENC_ETC, 0) < 0) {//I don't know instIndex before Calling VpuEncOpen
            VLOG(ERR, "fail to allocate HRD rbsp buffer\n" );
            return FALSE;
        }

        encOP->hrdRbspDataAddr = vbHrdRbsp->phys_addr;

        Uint8   *pHrdRbspBuf;
        Int32   rbspSizeInByte = (encOP->hrdRbspDataSize+7)>>3;
        ChangePathStyle(encConfig->hrd_rbsp_file_name);
        if ((encConfig->hrd_rbsp_fp = osal_fopen(encConfig->hrd_rbsp_file_name, "r")) == NULL) {
            VLOG(ERR, "fail to open HRD rbsp Data file, %s\n", encConfig->hrd_rbsp_file_name);
            return FALSE;
        }

        if (rbspSizeInByte > (Uint32)VUI_HRD_RBSP_BUF_SIZE)
            VLOG(ERR, "HRD Rbsp size is bigger than buffer size\n");

        if ((pHrdRbspBuf = (Uint8*)osal_malloc(VUI_HRD_RBSP_BUF_SIZE)) == NULL) {
            VLOG(ERR, "failed to allocate space for pHrdRbspBuf, size:%d\n", VUI_HRD_RBSP_BUF_SIZE);
            return FALSE;
        }
        osal_memset(pHrdRbspBuf, 0, VUI_HRD_RBSP_BUF_SIZE);
        osal_fread(pHrdRbspBuf, 1, rbspSizeInByte, encConfig->hrd_rbsp_fp);
        vdi_write_memory(coreIdx, encOP->hrdRbspDataAddr, pHrdRbspBuf,  rbspSizeInByte, encOP->streamEndian);
        osal_free(pHrdRbspBuf);
    }
    return TRUE;
}

int openRoiMapFile(TestEncConfig *encConfig)
{
    if (encConfig->roi_enable) {
        ChangePathStyle(encConfig->roi_file_name);
        if ((encConfig->roi_file = osal_fopen(encConfig->roi_file_name, "r")) == NULL) {
            VLOG(ERR, "fail to open ROI file, %s\n", encConfig->roi_file_name);
            return FALSE;
        }
    }
    return TRUE;
}
#endif

int allocateRoiMapBuf(EncHandle handle, TestEncConfig encConfig, vpu_buffer_t *vbRoi, int srcFbNum, int ctuNum)
{
    int i;
    Int32 coreIdx = handle->coreIdx;
    if (encConfig.roi_enable) {
        //number of roi buffer should be the same as source buffer num.
        for (i = 0; i < srcFbNum ; i++) {
            vbRoi[i].size = ctuNum;
            if (vdi_allocate_dma_memory(coreIdx, &vbRoi[i], ENC_ETC, handle->instIndex) < 0) {
                VLOG(ERR, "fail to allocate ROI buffer\n" );
                return FALSE;
            }
        }
    }
    return TRUE;
}

// Define tokens for parsing scaling list file
const char* MatrixType[SCALING_LIST_SIZE_NUM][SL_NUM_MATRIX] =
{
    {"INTRA4X4_LUMA", "INTRA4X4_CHROMAU", "INTRA4X4_CHROMAV", "INTER4X4_LUMA", "INTER4X4_CHROMAU", "INTER4X4_CHROMAV"},
    {"INTRA8X8_LUMA", "INTRA8X8_CHROMAU", "INTRA8X8_CHROMAV", "INTER8X8_LUMA", "INTER8X8_CHROMAU", "INTER8X8_CHROMAV"},
    {"INTRA16X16_LUMA", "INTRA16X16_CHROMAU", "INTRA16X16_CHROMAV", "INTER16X16_LUMA", "INTER16X16_CHROMAU", "INTER16X16_CHROMAV"},
    {"INTRA32X32_LUMA", "INTRA32X32_CHROMAU_FROM16x16_CHROMAU", "INTRA32X32_CHROMAV_FROM16x16_CHROMAV","INTER32X32_LUMA", "INTER32X32_CHROMAU_FROM16x16_CHROMAU", "INTER32X32_CHROMAV_FROM16x16_CHROMAV"}
};

const char* MatrixType_DC[SCALING_LIST_SIZE_NUM - 2][SL_NUM_MATRIX] =
{
    {"INTRA16X16_LUMA_DC", "INTRA16X16_CHROMAU_DC", "INTRA16X16_CHROMAV_DC", "INTER16X16_LUMA_DC", "INTER16X16_CHROMAU_DC", "INTER16X16_CHROMAV_DC"},
    {"INTRA32X32_LUMA_DC", "INTRA32X32_CHROMAU_DC_FROM16x16_CHROMAU", "INTRA32X32_CHROMAV_DC_FROM16x16_CHROMAV", "INTER32X32_LUMA_DC","INTER32X32_CHROMAU_DC_FROM16x16_CHROMAU","INTER32X32_CHROMAV_DC_FROM16x16_CHROMAV"},
};

static Uint8* get_sl_addr(UserScalingList* sl, Uint32 size_id, Uint32 mat_id)
{
    Uint8* addr = NULL;

    switch(size_id)
    {
    case SCALING_LIST_4x4:
        addr = sl->s4[mat_id];
        break;
    case SCALING_LIST_8x8:
        addr = sl->s8[mat_id];
        break;
    case SCALING_LIST_16x16:
        addr = sl->s16[mat_id];
        break;
    case SCALING_LIST_32x32:
        addr = sl->s32[mat_id];
        break;
    }
    return addr;
}

int parse_user_scaling_list(UserScalingList* sl, FILE* fp_sl, CodStd  stdMode)
{
#define LINE_SIZE (1024)
    const Uint32 scaling_list_size[SCALING_LIST_SIZE_NUM] = {16, 64, 64, 64};
    char line[LINE_SIZE];
    Uint32 i;
    Uint32 size_id, mat_id, data, num_coef = 0;
    Uint8* src = NULL;
    Uint8* ref = NULL;
    char* ret;
    const char* type_str;
    Uint32 maxScalingListSizeNum;

    if (fp_sl == NULL)
        return 0;

    // for HEVC : 4x4, 8x8, 16x16, 32x32
    // for AVC  : 4x4, 8x8
    maxScalingListSizeNum = (stdMode == STD_HEVC) ? SCALING_LIST_SIZE_NUM : SCALING_LIST_SIZE_NUM-2;

    for(size_id = 0; size_id < maxScalingListSizeNum; size_id++)  {
        num_coef = scaling_list_size[size_id];//lint !e644

        // for intra_y, intra_cb, intra_cr, inter_y, inter_cb, inter_cr
        for(mat_id = 0; mat_id < SL_NUM_MATRIX; mat_id++) {
            src = get_sl_addr(sl, size_id, mat_id);

            // for AVC  : ignore scaling list of chroma8x8
            if((stdMode == STD_AVC) && (size_id == SCALING_LIST_8x8 && (mat_id % 3)))
                continue;

            // for HEVC : derive scaling list of chroma32x32 from chroma16x16
            if((stdMode == STD_HEVC) && (size_id == SCALING_LIST_32x32 && (mat_id % 3))) {
                ref = get_sl_addr(sl, size_id - 1, mat_id);

                for(i = 0; i < num_coef; i++)
                    src[i] = ref[i];

                continue;
            }

            fseek(fp_sl,0,0);
            type_str = MatrixType[size_id][mat_id];

            do {
                ret = fgets(line, LINE_SIZE, fp_sl);
                if((ret == NULL) || (strstr(line, type_str) == NULL && feof(fp_sl))) {
                    VLOG(ERR,"Error: can't read a scaling list matrix(%s)\n", type_str);
                    return 0;
                }
            } while (strstr(line, type_str) == NULL);

            // get all coeff
            for(i = 0; i < num_coef; i++) {
                if(fscanf(fp_sl, "%u,", &data) != 1) {
                    VLOG(ERR,"Error: can't read a scaling list matrix(%s)\n", type_str);
                    return 0;
                }
                if (stdMode == STD_AVC && data < 4) {
                    VLOG(ERR,"Error: invald value in scaling list matrix(%s %d)\n", type_str, data);
                    return 0;
                }
                src[i] = data;
            }

            // get DC coeff for 16, 32
            if(size_id > SCALING_LIST_8x8) {
                fseek(fp_sl,0,0);
                type_str = MatrixType_DC[size_id - 2][mat_id];

                do {
                    ret = fgets(line, LINE_SIZE, fp_sl);
                    if((ret == NULL) || (strstr(line, type_str) == NULL && feof(fp_sl))) {
                        VLOG(ERR,"Error: can't read a scaling list matrix(%s)\n", type_str);
                        return 0;
                    }
                } while (strstr(line, type_str) == NULL);

                if(fscanf(fp_sl, "%u,", &data) != 1) {
                    VLOG(ERR,"Error: can't read a scaling list matrix(%s)\n", type_str);
                    return 0;
                }
                if (stdMode == STD_AVC && data < 4) {
                    VLOG(ERR,"Error: invald value in scaling list matrix(%s %d)\n", type_str, data);
                    return 0;
                }
                if(size_id == SCALING_LIST_16x16)
                    sl->s16dc[mat_id] = data;
                else // SCALING_LIST_32x32
                    sl->s32dc[mat_id/3] = data;
            }
        } // for matrix id
    } // for size id
    return 1;
}

int parse_custom_lambda(Uint32 buf[NUM_CUSTOM_LAMBDA], FILE* fp)
{
    int i, j = 0;
    char lineStr[256] = {0, };
    for(i = 0; i < 52; i++)
    {
        if( NULL == fgets(lineStr, 256, fp) )
        {
            VLOG(ERR,"Error: can't read custom_lambda\n");
            return 0;
        }
        else
        {
            sscanf(lineStr, "%u\n", &buf[j++]);
        }
    }
    for(i = 0; i < 52; i++)
    {
        if( NULL == fgets(lineStr, 256, fp) )
        {
            VLOG(ERR,"Error: can't read custom_lambda\n");
            return 0;
        }
        else
            sscanf(lineStr, "%u\n", &buf[j++]);
    }

    return 1;
}


#if defined(PLATFORM_NON_OS) || defined (PLATFORM_LINUX)
struct option* ConvertOptions(
    struct OptionExt*   cnmOpt,
    Uint32              nItems
    )
{
    struct option*  opt;
    Uint32          i;

    opt = (struct option*)osal_malloc(sizeof(struct option) * nItems);
    if (opt == NULL) {
        return NULL;
    }

    for (i=0; i<nItems; i++) {
        osal_memcpy((void*)&opt[i], (void*)&cnmOpt[i], sizeof(struct option));
    }

    return opt;
}
#endif

#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX) || defined(SUPPORT_READ_BITSTREAM_IN_ENCODER)
int mkdir_recursive(
    char *path,
    mode_t omode
    )
{
    struct stat sb;
    mode_t numask, oumask;
    int first, last, retval;
    char *p;

    p = path;
    oumask = 0;
    retval = 0;
    if (p[0] == '/')        /* Skip leading '/'. */
        ++p;
    for (first = 1, last = 0; !last ; ++p) {//lint !e441 !e443
        if (p[0] == '\0')
            last = 1;
        else if (p[0] != '/')
            continue;
        *p = '\0';
        if (p[1] == '\0')
            last = 1;
        if (first) {
            /*
            * POSIX 1003.2:
            * For each dir operand that does not name an existing
            * directory, effects equivalent to those cased by the
            * following command shall occcur:
            *
            * mkdir -p -m $(umask -S),u+wx $(dirname dir) &&
            *    mkdir [-m mode] dir
            *
            * We change the user's umask and then restore it,
            * instead of doing chmod's.
            */
            oumask = umask(0);
            numask = oumask & ~(S_IWUSR | S_IXUSR);
            (void)umask(numask);
            first = 0;
        }
        if (last)
            (void)umask(oumask);
        if (mkdir(path, last ? omode : S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
            if (errno == EEXIST || errno == EISDIR) {
                if (stat(path, &sb) < 0) {
                    VLOG(INFO, "%s", path);
                    retval = 1;
                    break;
                } else if (!S_ISDIR(sb.st_mode)) {
                    if (last)
                        errno = EEXIST;
                    else
                        errno = ENOTDIR;
                    VLOG(INFO, "%s", path);
                    retval = 1;
                    break;
                }
            } else {
                VLOG(INFO, "%s", path);
                retval = 1;
                break;
            }
        } else {
            VLOG(INFO, "%s", path);
            chmod(path, omode);
        }
        if (!last)
            *p = '/';
    }
    if (!first && !last)
        (void)umask(oumask);
    return (retval);
}
#endif

int file_exist(
    char* path
    )
{
#ifdef _MSC_VER
    DWORD   attributes;
    char    temp[4096];
    LPCTSTR lp_path = (LPCTSTR)temp;

    if (path == NULL) {
        return FALSE;
    }

    strcpy(temp, path);
    replace_character(temp, '/', '\\');
    attributes = GetFileAttributes(lp_path);
    return (attributes != (DWORD)-1);
#else
    return !access(path, F_OK);
#endif
}

BOOL MkDir(
    char* path
    )
{
#if defined(PLATFORM_NON_OS) || defined(PLATFORM_QNX)
    /* need to implement */
    return FALSE;
#else
#ifdef _MSC_VER
    char cmd[4096];
#endif
    if (file_exist(path))
        return TRUE;

#ifdef _MSC_VER
    sprintf(cmd, "mkdir %s", path);
    replace_character(cmd, '/', '\\');
    if (system(cmd)) {
        return FALSE;
    }
    return TRUE;
#else
    return mkdir_recursive(path, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
#endif
}

void GetUserData(Int32 coreIdx, Uint8* pBase, vpu_buffer_t vbUserData, DecOutputInfo outputInfo)
{
    int idx;
    user_data_entry_t* pEntry = (user_data_entry_t*)pBase;

    VpuReadMem(coreIdx, vbUserData.phys_addr, pBase, vbUserData.size, VPU_USER_DATA_ENDIAN);
    VLOG(INFO, "===== USER DATA(SEI OR VUI) : NUM(%d) =====\n", outputInfo.decOutputExtData.userDataNum);

    for (idx=0; idx<32; idx++) {
        if (outputInfo.decOutputExtData.userDataHeader&(1<<idx)) {
            VLOG(INFO, "\nUSERDATA INDEX: %02d offset: %8d size: %d\n", idx, pEntry[idx].offset, pEntry[idx].size);

            if (idx == H265_USERDATA_FLAG_MASTERING_COLOR_VOL) {
                h265_mastering_display_colour_volume_t* mastering;
                int i;

                mastering = (h265_mastering_display_colour_volume_t*)(pBase + pEntry[H265_USERDATA_FLAG_MASTERING_COLOR_VOL].offset);
                VLOG(INFO, " MASTERING DISPLAY COLOR VOLUME\n");
                for (i=0; i<3; i++) {
                    VLOG(INFO, " PRIMARIES_X%d : %10d PRIMARIES_Y%d : %10d\n", i, mastering->display_primaries_x[i], i, mastering->display_primaries_y[i]);
                }
                VLOG(INFO, " WHITE_POINT_X: %10d WHITE_POINT_Y: %10d\n", mastering->white_point_x, mastering->white_point_y);
                VLOG(INFO, " MIN_LUMINANCE: %10d MAX_LUMINANCE: %10d\n", mastering->min_display_mastering_luminance, mastering->max_display_mastering_luminance);
            }

            if(idx == H265_USERDATA_FLAG_VUI)
            {
                h265_vui_param_t* vui;

                vui = (h265_vui_param_t*)(pBase + pEntry[H265_USERDATA_FLAG_VUI].offset);
                VLOG(INFO, " VUI SAR(%d, %d)\n", vui->sar_width, vui->sar_height);
                VLOG(INFO, "     VIDEO FORMAT(%d)\n", vui->video_format);
                VLOG(INFO, "     COLOUR PRIMARIES(%d)\n", vui->colour_primaries);
                VLOG(INFO, "log2_max_mv_length_horizontal: %d\n", vui->log2_max_mv_length_horizontal);
                VLOG(INFO, "log2_max_mv_length_vertical  : %d\n", vui->log2_max_mv_length_vertical);
                VLOG(INFO, "video_full_range_flag  : %d\n", vui->video_full_range_flag);
                VLOG(INFO, "transfer_characteristics  : %d\n", vui->transfer_characteristics);
                VLOG(INFO, "matrix_coeffs  : %d\n", vui->matrix_coefficients);
            }
            if (idx == H265_USERDATA_FLAG_CHROMA_RESAMPLING_FILTER_HINT) {
                h265_chroma_resampling_filter_hint_t* c_resampleing_filter_hint;
                Uint32 i,j;

                c_resampleing_filter_hint = (h265_chroma_resampling_filter_hint_t*)(pBase + pEntry[H265_USERDATA_FLAG_CHROMA_RESAMPLING_FILTER_HINT].offset);
                VLOG(INFO, " CHROMA_RESAMPLING_FILTER_HINT\n");
                VLOG(INFO, " VER_CHROMA_FILTER_IDC: %10d HOR_CHROMA_FILTER_IDC: %10d\n", c_resampleing_filter_hint->ver_chroma_filter_idc, c_resampleing_filter_hint->hor_chroma_filter_idc);
                VLOG(INFO, " VER_FILTERING_FIELD_PROCESSING_FLAG: %d \n", c_resampleing_filter_hint->ver_filtering_field_processing_flag);
                if (c_resampleing_filter_hint->ver_chroma_filter_idc == 1 || c_resampleing_filter_hint->hor_chroma_filter_idc == 1) {
                    VLOG(INFO, " TARGET_FORMAT_IDC: %d \n", c_resampleing_filter_hint->target_format_idc);
                    if (c_resampleing_filter_hint->ver_chroma_filter_idc == 1) {
                        VLOG(INFO, " NUM_VERTICAL_FILTERS: %d \n", c_resampleing_filter_hint->num_vertical_filters);
                        for (i=0; i<c_resampleing_filter_hint->num_vertical_filters; i++) {
                            VLOG(INFO, " VER_TAP_LENGTH_M1[%d]: %d \n", i, c_resampleing_filter_hint->ver_tap_length_minus1[i]);
                            for (j=0; j<c_resampleing_filter_hint->ver_tap_length_minus1[i]; j++) {
                                VLOG(INFO, " VER_FILTER_COEFF[%d][%d]: %d \n", i, j, c_resampleing_filter_hint->ver_filter_coeff[i][j]);
                            }
                        }
                    }
                    if (c_resampleing_filter_hint->hor_chroma_filter_idc == 1) {
                        VLOG(INFO, " NUM_HORIZONTAL_FILTERS: %d \n", c_resampleing_filter_hint->num_horizontal_filters);
                        for (i=0; i<c_resampleing_filter_hint->num_horizontal_filters; i++) {
                            VLOG(INFO, " HOR_TAP_LENGTH_M1[%d]: %d \n", i, c_resampleing_filter_hint->hor_tap_length_minus1[i]);
                            for (j=0; j<c_resampleing_filter_hint->hor_tap_length_minus1[i]; j++) {
                                VLOG(INFO, " HOR_FILTER_COEFF[%d][%d]: %d \n", i, j, c_resampleing_filter_hint->hor_filter_coeff[i][j]);
                            }
                        }
                    }
                }
            }

            if (idx == H265_USERDATA_FLAG_KNEE_FUNCTION_INFO) {
                h265_knee_function_info_t* knee_function;

                knee_function = (h265_knee_function_info_t*)(pBase + pEntry[H265_USERDATA_FLAG_KNEE_FUNCTION_INFO].offset);
                VLOG(INFO, " FLAG_KNEE_FUNCTION_INFO\n");
                VLOG(INFO, " KNEE_FUNCTION_ID: %10d\n", knee_function->knee_function_id);
                VLOG(INFO, " KNEE_FUNCTION_CANCEL_FLAG: %d\n", knee_function->knee_function_cancel_flag);
                if (!knee_function->knee_function_cancel_flag) {
                    int i;

                    VLOG(INFO, " KNEE_FUNCTION_PERSISTENCE_FLAG: %10d\n", knee_function->knee_function_persistence_flag);
                    VLOG(INFO, " INPUT_D_RANGE: %d\n", knee_function->input_d_range);
                    VLOG(INFO, " INPUT_DISP_LUMINANCE: %d\n", knee_function->input_disp_luminance);
                    VLOG(INFO, " OUTPUT_D_RANGE: %d\n", knee_function->output_d_range);
                    VLOG(INFO, " OUTPUT_DISP_LUMINANCE: %d\n", knee_function->output_disp_luminance);
                    VLOG(INFO, " NUM_KNEE_POINTS_M1: %d\n", knee_function->num_knee_points_minus1);
                    for (i=0; i<knee_function->num_knee_points_minus1; i++) {
                        VLOG(INFO, " INPUT_KNEE_POINT: %10d OUTPUT_KNEE_POINT: %10d\n", knee_function->input_knee_point[i], knee_function->output_knee_point[i]);
                    }
                }
            }

            if (idx == H265_USERDATA_FLAG_ITU_T_T35_PRE)
            {
                char* itu_t_t35 = (char*)(pBase + pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE].offset);
                Uint32 i;

                VLOG(INFO, "ITU_T_T35_PRE = %d bytes, offset = %d\n",pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE].size, pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE].offset);
                for(i=0; i<pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE].size; i++)
                {
                    VLOG(INFO, "%02X ", (unsigned char)(itu_t_t35[i]));
                }
                VLOG(INFO, "\n");

            }

            if (idx == H265_USERDATA_FLAG_ITU_T_T35_PRE_1)
            {
                char* itu_t_t35 = (char*)(pBase + pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_1].offset);
                Uint32 i;

                VLOG(INFO, "ITU_T_T35_PRE = %d bytes, offset = %d\n",pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_1].size, pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_1].offset);
                for(i=0; i<pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_1].size; i++)
                {
                    VLOG(INFO, "%02X ", (unsigned char)(itu_t_t35[i]));
                }
                VLOG(INFO, "\n");
            }

            if (idx == H265_USERDATA_FLAG_ITU_T_T35_PRE_2)
            {
                char* itu_t_t35 = (char*)(pBase + pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].offset);
                Uint32 i;
                //user_data_entry_t* pEntry_prev;

                VLOG(INFO, "ITU_T_T35_PRE = %d bytes, offset = %d\n",pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].size, pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].offset);
                for(i=0; i<pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].size; i++)
                {
                    VLOG(INFO, "%02X ", (unsigned char)(itu_t_t35[i]));
                }
                VLOG(INFO, "\n");

                //pEntry_prev = (user_data_entry_t*)(itu_t_t35 + pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].size - sizeof(user_data_entry_t));
                //pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].size = pEntry_prev->size;
                //pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].offset = pEntry_prev->offset;
            }

            if (idx == H265_USERDATA_FLAG_COLOUR_REMAPPING_INFO) {
                int c, i;
                h265_colour_remapping_info_t* colour_remapping;
                colour_remapping = (h265_colour_remapping_info_t*)(pBase + pEntry[H265_USERDATA_FLAG_COLOUR_REMAPPING_INFO].offset);

                VLOG(INFO, " COLOUR_REMAPPING_INFO\n");
                VLOG(INFO, " COLOUR_REMAP_ID: %10d\n", colour_remapping->colour_remap_id);


                VLOG(INFO, " COLOUR_REMAP_CANCEL_FLAG: %d\n", colour_remapping->colour_remap_cancel_flag);
                if (!colour_remapping->colour_remap_cancel_flag) {
                    VLOG(INFO, " COLOUR_REMAP_PERSISTENCE_FLAG: %d\n", colour_remapping->colour_remap_persistence_flag);
                    VLOG(INFO, " COLOUR_REMAP_VIDEO_SIGNAL_INFO_PRESENT_FLAG: %d\n", colour_remapping->colour_remap_video_signal_info_present_flag);
                    if (colour_remapping->colour_remap_video_signal_info_present_flag) {
                        VLOG(INFO, " COLOUR_REMAP_FULL_RANGE_FLAG: %d\n", colour_remapping->colour_remap_full_range_flag);
                        VLOG(INFO, " COLOUR_REMAP_PRIMARIES: %d\n",	colour_remapping->colour_remap_primaries);
                        VLOG(INFO, " COLOUR_REMAP_TRANSFER_FUNCTION: %d\n", colour_remapping->colour_remap_transfer_function);
                        VLOG(INFO, " COLOUR_REMAP_MATRIX_COEFFICIENTS: %d\n", colour_remapping->colour_remap_matrix_coefficients);
                    }

                    VLOG(INFO, " COLOUR_REMAP_INPUT_BIT_DEPTH: %d\n", colour_remapping->colour_remap_input_bit_depth);
                    VLOG(INFO, " COLOUR_REMAP_OUTPUT_BIT_DEPTH: %d\n", colour_remapping->colour_remap_output_bit_depth);

                    for( c = 0; c < H265_MAX_LUT_NUM_VAL; c++ )
                    {
                        VLOG(INFO, " PRE_LUT_NUM_VAL_MINUS1[%d]: %d\n", c, colour_remapping->pre_lut_num_val_minus1[c]);
                        if(colour_remapping->pre_lut_num_val_minus1[c] > 0)
                        {
                            for( i = 0; i <= colour_remapping->pre_lut_num_val_minus1[c]; i++ )
                            {
                                VLOG(INFO, " PRE_LUT_CODED_VALUE[%d][%d]: %d\n", c, i, colour_remapping->pre_lut_coded_value[c][i]);
                                VLOG(INFO, " PRE_LUT_TARGET_VALUE[%d][%d]: %d\n", c, i, colour_remapping->pre_lut_target_value[c][i]);
                            }
                        }
                    }
                    VLOG(INFO, " COLOUR_REMAP_MATRIX_PRESENT_FLAG: %d\n", colour_remapping->colour_remap_matrix_present_flag);
                    if(colour_remapping->colour_remap_matrix_present_flag) {
                        VLOG(INFO, " LOG2_MATRIX_DENOM: %d\n", colour_remapping->log2_matrix_denom);
                        for( c = 0; c < H265_MAX_COLOUR_REMAP_COEFFS; c++ )
                            for( i = 0; i < H265_MAX_COLOUR_REMAP_COEFFS; i++ )
                                VLOG(INFO, " LOG2_MATRIX_DENOM[%d][%d]: %d\n", c, i, colour_remapping->colour_remap_coeffs[c][i]);
                    }

                    for( c = 0; c < H265_MAX_LUT_NUM_VAL; c++ )
                    {
                        VLOG(INFO, " POST_LUT_NUM_VAL_MINUS1[%d]: %d\n", c, colour_remapping->post_lut_num_val_minus1[c]);
                        if(colour_remapping->post_lut_num_val_minus1[c] > 0)
                        {
                            for( i = 0; i <= colour_remapping->post_lut_num_val_minus1[c]; i++)
                            {
                                VLOG(INFO, " POST_LUT_CODED_VALUE[%d][%d]: %d\n", c, i, colour_remapping->post_lut_coded_value[c][i]);
                                VLOG(INFO, " POST_LUT_TARGET_VALUE[%d][%d]: %d\n", c, i, colour_remapping->post_lut_target_value[c][i]);
                            }
                        }
                    }
                }
            }

            if (idx == H265_USERDATA_FLAG_TONE_MAPPING_INFO) {
                h265_tone_mapping_info_t* tone_mapping;

                tone_mapping = (h265_tone_mapping_info_t*)(pBase + pEntry[H265_USERDATA_FLAG_TONE_MAPPING_INFO].offset);
                VLOG(INFO, " FLAG_TONE_MAPPING_INFO\n");
                VLOG(INFO, " TONE_MAP_ID: %10d\n", tone_mapping->tone_map_id);
                VLOG(INFO, " TONE_MAP_CANCEL_FLAG: %d\n", tone_mapping->tone_map_cancel_flag);
                if (!tone_mapping->tone_map_cancel_flag) {
                    int i;

                    VLOG(INFO, " TONE_MAP_PERSISTENCE_FLAG: %10d\n", tone_mapping->tone_map_persistence_flag);
                    VLOG(INFO, " CODED_DATA_BIT_DEPTH : %d\n", tone_mapping->coded_data_bit_depth);
                    VLOG(INFO, " TARGET_BIT_DEPTH : %d\n", tone_mapping->target_bit_depth);
                    VLOG(INFO, " TONE_MAP_MODEL_ID : %d\n", tone_mapping->tone_map_model_id);
                    VLOG(INFO, " MIN_VALUE : %d\n", tone_mapping->min_value);
                    VLOG(INFO, " MAX_VALUE : %d\n", tone_mapping->max_value);
                    VLOG(INFO, " SIGMOID_MIDPOINT : %d\n", tone_mapping->sigmoid_midpoint);
                    VLOG(INFO, " SIGMOID_MIDPOINT : %d\n", tone_mapping->sigmoid_width);
                    for (i=0; i<(1<<tone_mapping->target_bit_depth); i++) {
                        VLOG(INFO, " START_OF_CODED_INTERVAL[%d] : %d\n", i, tone_mapping->start_of_coded_interval[i]); // [1 << target_bit_depth] // 10bits
                    }

                    VLOG(INFO, " NUM_PIVOTS : %d\n", tone_mapping->num_pivots); // [(1 << coded_data_bit_depth)?1][(1 << target_bit_depth)-1] // 10bits
                    for (i=0; i<tone_mapping->num_pivots; i++) {
                        VLOG(INFO, " CODED_PIVOT_VALUE[%d] : %d, TARGET_PIVOT_VALUE[%d] : %d\n", i, tone_mapping->coded_pivot_value[i]
                        , i, tone_mapping->target_pivot_value[i]);
                    }

                    VLOG(INFO, " CAMERA_ISO_SPEED_IDC : %d\n", tone_mapping->camera_iso_speed_idc);
                    VLOG(INFO, " CAMERA_ISO_SPEED_VALUE : %d\n", tone_mapping->camera_iso_speed_value);

                    VLOG(INFO, " EXPOSURE_INDEX_IDC : %d\n", tone_mapping->exposure_index_idc);
                    VLOG(INFO, " EXPOSURE_INDEX_VALUE : %d\n", tone_mapping->exposure_index_value);
                    VLOG(INFO, " EXPOSURE_INDEX_COMPESATION_VALUE_SIGN_FLAG : %d\n", tone_mapping->exposure_compensation_value_sign_flag);
                    VLOG(INFO, " EXPOSURE_INDEX_COMPESATION_VALUE_NUMERATOR : %d\n", tone_mapping->exposure_compensation_value_numerator);
                    VLOG(INFO, " EXPOSURE_INDEX_COMPESATION_VALUE_DENOM_IDC : %d\n", tone_mapping->exposure_compensation_value_denom_idc);

                    VLOG(INFO, " REF_SCREEN_LUMINANCE_WHITE : %d\n", tone_mapping->ref_screen_luminance_white);

                    VLOG(INFO, " EXTENDED_RANGE_WHITE_LEVEL : %d\n", tone_mapping->extended_range_white_level);
                    VLOG(INFO, " NOMINAL_BLACK_LEVEL_CODE_VALUE : %d\n", tone_mapping->nominal_black_level_code_value);
                    VLOG(INFO, " NOMINAL_WHITE_LEVEL_CODE_VALUE : %d\n", tone_mapping->nominal_white_level_code_value);
                    VLOG(INFO, " EXTENDED_WHITE_LEVEL_CODE_VALUE : %d\n", tone_mapping->extended_white_level_code_value);
                }
                VLOG(INFO, "\n");
            }

            if (idx == H265_USERDATA_FLAG_CONTENT_LIGHT_LEVEL_INFO) {
                h265_content_light_level_info_t* content_light_level;

                content_light_level = (h265_content_light_level_info_t*)(pBase + pEntry[H265_USERDATA_FLAG_CONTENT_LIGHT_LEVEL_INFO].offset);
                VLOG(INFO, " CONTNET_LIGHT_INFO\n");
                VLOG(INFO, " MAX_CONTENT_LIGHT_LEVEL : %d\n", content_light_level->max_content_light_level);
                VLOG(INFO, " MAX_PIC_AVERAGE_LIGHT_LEVEL : %d\n", content_light_level->max_pic_average_light_level);
                VLOG(INFO, "\n");
            }

            if (idx == H265_USERDATA_FLAG_FILM_GRAIN_CHARACTERISTICS_INFO) {
                h265_film_grain_characteristics_t* film_grain_characteristics;
                int i,j,c;

                film_grain_characteristics = (h265_film_grain_characteristics_t*)(pBase + pEntry[H265_USERDATA_FLAG_FILM_GRAIN_CHARACTERISTICS_INFO].offset);
                VLOG(INFO, " FILM_GRAIN_CHARACTERISTICS_INFO\n");
                VLOG(INFO, " FILM_GRAIN_CHARACTERISTICS_CANCEL_FLAG: %d\n", film_grain_characteristics->film_grain_characteristics_cancel_flag);
                if (!film_grain_characteristics->film_grain_characteristics_cancel_flag) {
                    VLOG(INFO, " FILM_GRAIN_MODEL_ID: %10d\n", film_grain_characteristics->film_grain_model_id);

                    VLOG(INFO, " SEPARATE_COLOUR_DESCRIPTION_PRESENT_FLAG: %d\n", film_grain_characteristics->separate_colour_description_present_flag);
                    if (film_grain_characteristics->separate_colour_description_present_flag)
                    {
                        VLOG(INFO, " FILM_GRAIN_BIT_DEPTH_LUMA_MINUS8: %d\n", film_grain_characteristics->film_grain_bit_depth_luma_minus8);
                        VLOG(INFO, " FILM_GRAIN_BIT_DEPTH_CHROMA_MINUS8: %d\n", film_grain_characteristics->film_grain_bit_depth_chroma_minus8);
                        VLOG(INFO, " FILM_GRAIN_FULL_RANGE_FLAG: %d\n", film_grain_characteristics->film_grain_full_range_flag);
                        VLOG(INFO, " FILM_GRAIN_COLOUR_PRIMARIES: %d\n", film_grain_characteristics->film_grain_colour_primaries);
                        VLOG(INFO, " FILM_GRAIN_TRANSFER_CHARACTERISTICS: %d\n", film_grain_characteristics->film_grain_transfer_characteristics);
                        VLOG(INFO, " FILM_GRAIN_MATRIX_COEFF: %d\n", film_grain_characteristics->film_grain_matrix_coeffs);
                    }
                }

                VLOG(INFO, " BLENDING_MODE_ID: %d\n", film_grain_characteristics->blending_mode_id);
                VLOG(INFO, " LOG2_SCALE_FACTOR: %d\n", film_grain_characteristics->log2_scale_factor);
                for(c=0; c < H265_MAX_NUM_FILM_GRAIN_COMPONENT; c++ )
                {
                    VLOG(INFO, " COMP_MODEL_PRESENT_FLAG[%d]: %d\n", c, film_grain_characteristics->comp_model_present_flag[c]);
                }

                for(c=0; c < H265_MAX_NUM_FILM_GRAIN_COMPONENT; c++ )
                {
                    if(film_grain_characteristics->comp_model_present_flag[c])
                    {

                        VLOG(INFO, " NUM_INTENSITY_INTERVALS_MINUS1[%d]: %d\n", c, film_grain_characteristics->num_intensity_intervals_minus1[c]);
                        VLOG(INFO, " NUM_MODEL_VALUES_MINUS1[%d]: %d\n", c, film_grain_characteristics->num_model_values_minus1[c]);
                        for(i=0; i <= film_grain_characteristics->num_intensity_intervals_minus1[c]; i++)
                        {
                            VLOG(INFO, " INTENSITY_INTERVAL_LOWER_BOUND[%d][%d]: %d\n", c, i, film_grain_characteristics->intensity_interval_lower_bound[c][i]);
                            VLOG(INFO, " INTENSITY_INTERVAL_UPPER_BOUND[%d][%d]: %d\n", c, i, film_grain_characteristics->intensity_interval_upper_bound[c][i]);
                            for(j=0; j <= film_grain_characteristics->num_model_values_minus1[c]; j++)
                            {
                                VLOG(INFO, " COMP_MODEL_VALUE[%d][%d][%d]: %d\n", c, i, j, film_grain_characteristics->comp_model_value[c][i][j]);
                            }
                        }
                    }
                }
                VLOG(INFO, " FILM_GRAIN_CHARACTERISTICS_PERSISTENCE_FLAG: %d\n", film_grain_characteristics->film_grain_characteristics_persistence_flag);
                VLOG(INFO, "\n");
            }
        }
    }
    VLOG(INFO, "===========================================\n");
}

#if 0
RetCode SetChangeParam(EncHandle handle, TestEncConfig encConfig, EncOpenParam encOP, Int32 changedCount)
{
    int i;
    RetCode ret;
    ENC_CFG ParaChagCfg;
    EncChangeParam changeParam;

    osal_memset(&ParaChagCfg, 0x00, sizeof(ENC_CFG));
    osal_memset(&changeParam, 0x00, sizeof(EncChangeParam));

    parseWaveChangeParamCfgFile(&ParaChagCfg, encConfig.changeParam[changedCount].cfgName);

    changeParam.enable_option = encConfig.changeParam[changedCount].enableOption;

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_PPS) {
        changeParam.constIntraPredFlag			= ParaChagCfg.waveCfg.constIntraPredFlag;
        changeParam.lfCrossSliceBoundaryEnable	= ParaChagCfg.waveCfg.lfCrossSliceBoundaryEnable;
        changeParam.weightPredEnable			= ParaChagCfg.waveCfg.weightPredEnable;
        changeParam.disableDeblk				= ParaChagCfg.waveCfg.disableDeblk;
        changeParam.betaOffsetDiv2				= ParaChagCfg.waveCfg.betaOffsetDiv2;
        changeParam.tcOffsetDiv2				= ParaChagCfg.waveCfg.tcOffsetDiv2;
        changeParam.chromaCbQpOffset			= ParaChagCfg.waveCfg.chromaCbQpOffset;
        changeParam.chromaCrQpOffset			= ParaChagCfg.waveCfg.chromaCrQpOffset;
        if (encConfig.stdMode == STD_AVC) {
            changeParam.transform8x8Enable      = ParaChagCfg.waveCfg.transform8x8;
            changeParam.entropyCodingMode       = ParaChagCfg.waveCfg.entropyCodingMode;
        }
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_INDEPEND_SLICE) {
        changeParam.independSliceMode			= ParaChagCfg.waveCfg.independSliceMode;
        changeParam.independSliceModeArg		= ParaChagCfg.waveCfg.independSliceModeArg;
        if (encConfig.stdMode == STD_AVC) {
            changeParam.avcSliceMode            = ParaChagCfg.waveCfg.avcSliceMode;
            changeParam.avcSliceArg             = ParaChagCfg.waveCfg.avcSliceArg;
        }
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_DEPEND_SLICE) {
        changeParam.dependSliceMode				= ParaChagCfg.waveCfg.dependSliceMode;
        changeParam.dependSliceModeArg			= ParaChagCfg.waveCfg.dependSliceModeArg;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RDO) {
        changeParam.coefClearDisable			= ParaChagCfg.waveCfg.coefClearDisable;
        changeParam.intraNxNEnable				= ParaChagCfg.waveCfg.intraNxNEnable;
        changeParam.maxNumMerge					= ParaChagCfg.waveCfg.maxNumMerge;
        changeParam.customLambdaEnable			= ParaChagCfg.waveCfg.customLambdaEnable;
        changeParam.customMDEnable				= ParaChagCfg.waveCfg.customMDEnable;
        if (encConfig.stdMode == STD_AVC) {
            changeParam.rdoSkip                 = ParaChagCfg.waveCfg.rdoSkip;
            changeParam.lambdaScalingEnable     = ParaChagCfg.waveCfg.lambdaScalingEnable;
        }
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC_FRAME_RATE) {
        changeParam.frameRate					= ParaChagCfg.waveCfg.frameRate;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC_TARGET_RATE) {
        changeParam.bitRate						= ParaChagCfg.RcBitRate;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC) {
        changeParam.hvsQPEnable					= ParaChagCfg.waveCfg.hvsQPEnable;
        changeParam.hvsQpScale					= ParaChagCfg.waveCfg.hvsQpScale;
        changeParam.vbvBufferSize				= ParaChagCfg.VbvBufferSize;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC_MIN_MAX_QP) {
        changeParam.minQpI						= ParaChagCfg.waveCfg.minQp;
        changeParam.maxQpI						= ParaChagCfg.waveCfg.maxQp;
        changeParam.hvsMaxDeltaQp   			= ParaChagCfg.waveCfg.maxDeltaQp;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC_INTER_MIN_MAX_QP) {
        changeParam.minQpP = ParaChagCfg.waveCfg.minQp;
        changeParam.minQpB = ParaChagCfg.waveCfg.minQp;
        changeParam.maxQpP = ParaChagCfg.waveCfg.maxQp;
        changeParam.maxQpB = ParaChagCfg.waveCfg.maxQp;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_RC_BIT_RATIO_LAYER) {
        for (i=0 ; i<MAX_GOP_NUM; i++)
            changeParam.fixedBitRatio[i]	= ParaChagCfg.waveCfg.fixedBitRatio[i];
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_BG) {
        changeParam.s2fmeDisable                = ParaChagCfg.waveCfg.s2fmeDisable;
        changeParam.bgThrDiff					= ParaChagCfg.waveCfg.bgThrDiff;
        changeParam.bgThrMeanDiff				= ParaChagCfg.waveCfg.bgThrMeanDiff;
        changeParam.bgLambdaQp					= ParaChagCfg.waveCfg.bgLambdaQp;
        changeParam.bgDeltaQp					= ParaChagCfg.waveCfg.bgDeltaQp;
        changeParam.s2fmeDisable                = ParaChagCfg.waveCfg.s2fmeDisable;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_NR) {
        changeParam.nrYEnable					= ParaChagCfg.waveCfg.nrYEnable;
        changeParam.nrCbEnable					= ParaChagCfg.waveCfg.nrCbEnable;
        changeParam.nrCrEnable					= ParaChagCfg.waveCfg.nrCrEnable;
        changeParam.nrNoiseEstEnable			= ParaChagCfg.waveCfg.nrNoiseEstEnable;
        changeParam.nrNoiseSigmaY				= ParaChagCfg.waveCfg.nrNoiseSigmaY;
        changeParam.nrNoiseSigmaCb				= ParaChagCfg.waveCfg.nrNoiseSigmaCb;
        changeParam.nrNoiseSigmaCr				= ParaChagCfg.waveCfg.nrNoiseSigmaCr;

        changeParam.nrIntraWeightY				= ParaChagCfg.waveCfg.nrIntraWeightY;
        changeParam.nrIntraWeightCb				= ParaChagCfg.waveCfg.nrIntraWeightCb;
        changeParam.nrIntraWeightCr				= ParaChagCfg.waveCfg.nrIntraWeightCr;
        changeParam.nrInterWeightY				= ParaChagCfg.waveCfg.nrInterWeightY;
        changeParam.nrInterWeightCb				= ParaChagCfg.waveCfg.nrInterWeightCb;
        changeParam.nrInterWeightCr				= ParaChagCfg.waveCfg.nrInterWeightCr;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_CUSTOM_MD) {
        changeParam.pu04DeltaRate               = ParaChagCfg.waveCfg.pu04DeltaRate;
        changeParam.pu08DeltaRate               = ParaChagCfg.waveCfg.pu08DeltaRate;
        changeParam.pu16DeltaRate               = ParaChagCfg.waveCfg.pu16DeltaRate;
        changeParam.pu32DeltaRate               = ParaChagCfg.waveCfg.pu32DeltaRate;
        changeParam.pu04IntraPlanarDeltaRate    = ParaChagCfg.waveCfg.pu04IntraPlanarDeltaRate;
        changeParam.pu04IntraDcDeltaRate        = ParaChagCfg.waveCfg.pu04IntraDcDeltaRate;
        changeParam.pu04IntraAngleDeltaRate     = ParaChagCfg.waveCfg.pu04IntraAngleDeltaRate;
        changeParam.pu08IntraPlanarDeltaRate    = ParaChagCfg.waveCfg.pu08IntraPlanarDeltaRate;
        changeParam.pu08IntraDcDeltaRate        = ParaChagCfg.waveCfg.pu08IntraDcDeltaRate;
        changeParam.pu08IntraAngleDeltaRate     = ParaChagCfg.waveCfg.pu08IntraAngleDeltaRate;
        changeParam.pu16IntraPlanarDeltaRate    = ParaChagCfg.waveCfg.pu16IntraPlanarDeltaRate;
        changeParam.pu16IntraDcDeltaRate        = ParaChagCfg.waveCfg.pu16IntraDcDeltaRate;
        changeParam.pu16IntraAngleDeltaRate     = ParaChagCfg.waveCfg.pu16IntraAngleDeltaRate;
        changeParam.pu32IntraPlanarDeltaRate    = ParaChagCfg.waveCfg.pu32IntraPlanarDeltaRate;
        changeParam.pu32IntraDcDeltaRate        = ParaChagCfg.waveCfg.pu32IntraDcDeltaRate;
        changeParam.pu32IntraAngleDeltaRate     = ParaChagCfg.waveCfg.pu32IntraAngleDeltaRate;
        changeParam.cu08IntraDeltaRate          = ParaChagCfg.waveCfg.cu08IntraDeltaRate;
        changeParam.cu08InterDeltaRate          = ParaChagCfg.waveCfg.cu08InterDeltaRate;
        changeParam.cu08MergeDeltaRate          = ParaChagCfg.waveCfg.cu08MergeDeltaRate;
        changeParam.cu16IntraDeltaRate          = ParaChagCfg.waveCfg.cu16IntraDeltaRate;
        changeParam.cu16InterDeltaRate          = ParaChagCfg.waveCfg.cu16InterDeltaRate;
        changeParam.cu16MergeDeltaRate          = ParaChagCfg.waveCfg.cu16MergeDeltaRate;
        changeParam.cu32IntraDeltaRate          = ParaChagCfg.waveCfg.cu32IntraDeltaRate;
        changeParam.cu32InterDeltaRate          = ParaChagCfg.waveCfg.cu32InterDeltaRate;
        changeParam.cu32MergeDeltaRate          = ParaChagCfg.waveCfg.cu32MergeDeltaRate;
    }

    if (changeParam.enable_option & ENC_SET_CHANGE_PARAM_INTRA_PARAM) {
        changeParam.intraQP                     = ParaChagCfg.waveCfg.intraQP;
        changeParam.intraPeriod                 = ParaChagCfg.waveCfg.intraPeriod;
        changeParam.avcIdrPeriod                = ParaChagCfg.waveCfg.idrPeriod;
        changeParam.forcedIdrHeaderEnable       = ParaChagCfg.waveCfg.forcedIdrHeaderEnable;
    }
    ret = VPU_EncGiveCommand(handle, ENC_SET_PARA_CHANGE, &changeParam);

    return ret;
}

BOOL GetBitstreamToBuffer(
    EncHandle handle,
    Uint8* pBuffer,
    PhysicalAddress rdAddr,
    PhysicalAddress wrAddr,
    PhysicalAddress streamBufStartAddr,
    PhysicalAddress streamBufEndAddr,
    Uint32 streamSize,
    EndianMode endian,
    BOOL ringbufferEnabled
    )
{
    Int32 coreIdx      = -1;
    Uint32 room         = 0;

    if (NULL == handle) {
        VLOG(ERR, "<%s:%d> NULL point exception\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    coreIdx = VPU_HANDLE_CORE_INDEX(handle);

    if (0 == streamBufStartAddr || 0 == streamBufEndAddr) {
        VLOG(ERR, "<%s:%d> Wrong Address, start or end Addr\n", __FUNCTION__, __LINE__);
        return FALSE;
    } else if (0 == rdAddr || 0 == wrAddr) {
        VLOG(ERR, "<%s:%d> Wrong Address, read or write Addr\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if (TRUE == ringbufferEnabled) {
        if ((rdAddr + streamSize) > streamBufEndAddr) {
            //wrap around on ringbuffer
            room = streamBufEndAddr - rdAddr;
            vdi_read_memory(coreIdx, rdAddr, pBuffer, room, endian);
            vdi_read_memory(coreIdx, streamBufStartAddr, pBuffer + room, (streamSize-room), endian);
        }
        else {
            vdi_read_memory(coreIdx, rdAddr, pBuffer, streamSize, endian);
        }
    } else { //Line buffer
        vdi_read_memory(coreIdx, rdAddr, pBuffer, streamSize, endian);
    }

    return TRUE;
}

#endif
