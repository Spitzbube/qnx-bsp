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

#include "vpuapifunc.h"
#include "product.h"
#include "wave5_regdefine.h"


#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif
#define MAX_LAVEL_IDX    16
static const int g_anLevel[MAX_LAVEL_IDX] =
{
    10, 11, 11, 12, 13,
    //10, 16, 11, 12, 13,
    20, 21, 22,
    30, 31, 32,
    40, 41, 42,
    50, 51
};

static const int g_anLevelMaxMBPS[MAX_LAVEL_IDX] =
{
    1485,   1485,   3000,   6000, 11880,
    11880,  19800,  20250,
    40500,  108000, 216000,
    245760, 245760, 522240,
    589824, 983040
};

static const int g_anLevelMaxFS[MAX_LAVEL_IDX] =
{
    99,    99,   396, 396, 396,
    396,   792,  1620,
    1620,  3600, 5120,
    8192,  8192, 8704,
    22080, 36864
};

static const int g_anLevelMaxBR[MAX_LAVEL_IDX] =
{
    64,     64,   192,  384, 768,
    2000,   4000,  4000,
    10000,  14000, 20000,
    20000,  50000, 50000,
    135000, 240000
};

static const int g_anLevelSliceRate[MAX_LAVEL_IDX] =
{
    0,  0,  0,  0,  0,
    0,  0,  0,
    22, 60, 60,
    60, 24, 24,
    24, 24
};

static const int g_anLevelMaxMbs[MAX_LAVEL_IDX] =
{
    28,   28,  56, 56, 56,
    56,   79, 113,
    113, 169, 202,
    256, 256, 263,
    420, 543
};

/******************************************************************************
    define value
******************************************************************************/

/******************************************************************************
    Codec Instance Slot Management
******************************************************************************/

RetCode InitCodecInstancePool(Uint32 coreIdx)
{
    int i;
    CodecInst * pCodecInst;
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return RETCODE_INSUFFICIENT_RESOURCE;

    if (vip->instance_pool_inited==0)
    {
        for( i = 0; i < MAX_NUM_INSTANCE; i++)
        {
            pCodecInst = (CodecInst *)vip->codecInstPool[i];
            pCodecInst->instIndex = i;
            pCodecInst->inUse = 0;
        }
        vip->instance_pool_inited = 1;
    }
    return RETCODE_SUCCESS;
}

/*
 * GetCodecInstance() obtains a instance.
 * It stores a pointer to the allocated instance in *ppInst
 * and returns RETCODE_SUCCESS on success.
 * Failure results in 0(null pointer) in *ppInst and RETCODE_FAILURE.
 */

RetCode GetCodecInstance(Uint32 coreIdx, CodecInst ** ppInst)
{
    int                     i;
    CodecInst*              pCodecInst = 0;
    vpu_instance_pool_t*    vip;
    Uint32                  handleSize;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return RETCODE_INSUFFICIENT_RESOURCE;

    for (i = 0; i < MAX_NUM_INSTANCE; i++) {
        pCodecInst = (CodecInst *)vip->codecInstPool[i];

        if (!pCodecInst) {
            return RETCODE_FAILURE;
        }

        if (!pCodecInst->inUse) {
            break;
        }
    }

    if (i == MAX_NUM_INSTANCE) {
        *ppInst = 0;
        return RETCODE_FAILURE;
    }

    pCodecInst->inUse         = 1;
    pCodecInst->coreIdx       = coreIdx;
    pCodecInst->codecMode     = -1;
    pCodecInst->codecModeAux  = -1;
    pCodecInst->loggingEnable = 0;
    pCodecInst->isDecoder     = TRUE;
    pCodecInst->productId     = ProductVpuGetId(coreIdx);
    osal_memset((void*)&pCodecInst->CodecInfo, 0x00, sizeof(pCodecInst->CodecInfo));

    handleSize = sizeof(DecInfo);
    if (handleSize < sizeof(EncInfo)) {
        handleSize = sizeof(EncInfo);
    }
    if ((pCodecInst->CodecInfo=(void*)osal_malloc(handleSize)) == NULL) {
        return RETCODE_INSUFFICIENT_RESOURCE;
    }
    osal_memset(pCodecInst->CodecInfo, 0x00, sizeof(handleSize));

    *ppInst = pCodecInst;


    return RETCODE_SUCCESS;
}

void FreeCodecInstance(CodecInst * pCodecInst)
{
    pCodecInst->codecMode    = -1;
    pCodecInst->codecModeAux = -1;

    vdi_close_instance(pCodecInst->coreIdx, pCodecInst->instIndex);

    osal_free(pCodecInst->CodecInfo);
    pCodecInst->CodecInfo = NULL;
    pCodecInst->inUse = 0;
}

RetCode CheckInstanceValidity(CodecInst * pCodecInst)
{
    int i;
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
    if (!vip)
        return RETCODE_INSUFFICIENT_RESOURCE;

    for (i = 0; i < MAX_NUM_INSTANCE; i++) {
        if ((CodecInst *)vip->codecInstPool[i] == pCodecInst)
            return RETCODE_SUCCESS;
    }

    return RETCODE_INVALID_HANDLE;
}

/******************************************************************************
    API Subroutines
******************************************************************************/

Uint64 GetTimestamp(
    EncHandle handle
    )
{
    CodecInst*  pCodecInst = (CodecInst*)handle;
    EncInfo*    pEncInfo   = NULL;
    Uint64      pts;
    Uint32      fps;

    if (pCodecInst == NULL) {
        return 0;
    }

    pEncInfo   = &pCodecInst->CodecInfo->encInfo;
    fps        = pEncInfo->openParam.frameRateInfo;
    if (fps == 0) {
        fps    = 30;        /* 30 fps */
    }

    pts        = pEncInfo->curPTS;
    pEncInfo->curPTS += 90000/fps; /* 90KHz/fps */

    return pts;
}

RetCode SetEncCropInfo(Int32 codecMode, EncWaveParam* param, int rotMode, int srcWidth, int srcHeight)
{
    int alignedWidth  = (codecMode == W_HEVC_ENC) ? VPU_ALIGN32(srcWidth)  : VPU_ALIGN16(srcWidth);
    int alignedHeight = (codecMode == W_HEVC_ENC) ? VPU_ALIGN32(srcHeight) : VPU_ALIGN16(srcHeight);
    int pad_right, pad_bot;
    int crop_right, crop_left, crop_top, crop_bot;
    int prp_mode = rotMode>>1;  // remove prp_enable bit

    if ((codecMode == W_HEVC_ENC) &&
        ((rotMode == 0) || (prp_mode == 14))) // prp_mode 14 : hor_mir && ver_mir && rot_180
    {
        return RETCODE_SUCCESS;
    }

    pad_right = alignedWidth  - srcWidth;
    pad_bot   = alignedHeight - srcHeight;

    if (param->confWinRight > 0)
        crop_right = param->confWinRight + pad_right;
    else
        crop_right = pad_right;

    if (param->confWinBot > 0)
        crop_bot = param->confWinBot + pad_bot;
    else
        crop_bot = pad_bot;

    crop_top  = param->confWinTop;
    crop_left = param->confWinLeft;

    param->confWinTop   = crop_top;
    param->confWinLeft  = crop_left;
    param->confWinBot   = crop_bot;
    param->confWinRight = crop_right;

    /* prp_mode :
    *          | hor_mir | ver_mir |   rot_angle
    *              [3]       [2]         [1:0] = {0= NONE, 1:90, 2:180, 3:270}
    */
    if(prp_mode == 1 || prp_mode ==15)
    {
        param->confWinTop   = crop_right;
        param->confWinLeft  = crop_top;
        param->confWinBot   = crop_left;
        param->confWinRight = crop_bot;
    }
    else if(prp_mode == 2 || prp_mode ==12)
    {
        param->confWinTop   = crop_bot;
        param->confWinLeft  = crop_right;
        param->confWinBot   = crop_top;
        param->confWinRight = crop_left;
    }
    else if(prp_mode == 3 || prp_mode ==13)
    {
        param->confWinTop   = crop_left;
        param->confWinLeft  = crop_bot;
        param->confWinBot   = crop_right;
        param->confWinRight = crop_top;
    }
    else if(prp_mode == 4 || prp_mode ==10)
    {
        param->confWinTop   = crop_bot;
        param->confWinBot   = crop_top;
    }
    else if(prp_mode == 8 || prp_mode ==6)
    {
        param->confWinLeft  = crop_right;
        param->confWinRight = crop_left;
    }
    else if(prp_mode == 5 || prp_mode ==11)
    {
        param->confWinTop   = crop_left;
        param->confWinLeft  = crop_top;
        param->confWinBot   = crop_right;
        param->confWinRight = crop_bot;
    }
    else if(prp_mode == 7 || prp_mode ==9)
    {
        param->confWinTop   = crop_right;
        param->confWinLeft  = crop_bot;
        param->confWinBot   = crop_left;
        param->confWinRight = crop_top;
    }

    return RETCODE_SUCCESS;
}

int DecBitstreamBufEmpty(DecInfo * pDecInfo)
{
    return (pDecInfo->streamRdPtr == pDecInfo->streamWrPtr);
}


RetCode SetParaSet(DecHandle handle, int paraSetType, DecParamSet * para)
{
    return RETCODE_SUCCESS;
}

void DecSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr, PhysicalAddress paraBuffer)
{
    BYTE tempBuf[8]={0,};                    // 64bit bus & endian
    Uint32 val;

    val = paraBuffer;
    tempBuf[0] = 0;
    tempBuf[1] = 0;
    tempBuf[2] = 0;
    tempBuf[3] = 0;
    tempBuf[4] = (val >> 24) & 0xff;
    tempBuf[5] = (val >> 16) & 0xff;
    tempBuf[6] = (val >> 8) & 0xff;
    tempBuf[7] = (val >> 0) & 0xff;
    VpuWriteMem(coreIdx, baseAddr, (BYTE *)tempBuf, 8, VDI_BIG_ENDIAN);
}

RetCode CheckEncInstanceValidity(EncHandle handle)
{
    CodecInst * pCodecInst;
    RetCode ret;

    if (handle == NULL)
        return RETCODE_INVALID_HANDLE;

    pCodecInst = handle;
    ret = CheckInstanceValidity(pCodecInst);
    if (ret != RETCODE_SUCCESS) {
        return RETCODE_INVALID_HANDLE;
    }
    if (!pCodecInst->inUse) {
        return RETCODE_INVALID_HANDLE;
    }

    if (pCodecInst->codecMode != MP4_ENC &&
        pCodecInst->codecMode != W_HEVC_ENC &&
        pCodecInst->codecMode != W_SVAC_ENC &&
        pCodecInst->codecMode != W_AVC_ENC  &&
        pCodecInst->codecMode != AVC_ENC) {
        return RETCODE_INVALID_HANDLE;
    }
    return RETCODE_SUCCESS;
}

RetCode CheckEncParam(EncHandle handle, EncParam * param)
{
    CodecInst *pCodecInst;
    EncInfo *pEncInfo;

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    if (param == 0) {
        return RETCODE_INVALID_PARAM;
    }

    if (param->skipPicture != 0 && param->skipPicture != 1) {
        return RETCODE_INVALID_PARAM;
    }
    if (param->skipPicture == 0) {
        if (param->sourceFrame == 0) {
            return RETCODE_INVALID_FRAME_BUFFER;
        }
    }
    if (pEncInfo->openParam.bitRate == 0) { // no rate control
        if (pCodecInst->codecMode == W_HEVC_ENC || pCodecInst->codecMode == W_SVAC_ENC) {
            if (param->forcePicQpEnable == 1) {
                if (param->forcePicQpI < 0 || param->forcePicQpI > 63)
                    return RETCODE_INVALID_PARAM;

                if (param->forcePicQpP < 0 || param->forcePicQpP > 63)
                    return RETCODE_INVALID_PARAM;

                if (param->forcePicQpB < 0 || param->forcePicQpB > 63)
                    return RETCODE_INVALID_PARAM;
            }
            if (pEncInfo->ringBufferEnable == 0) {
                if (param->picStreamBufferAddr % 16 || param->picStreamBufferSize == 0)
                    return RETCODE_INVALID_PARAM;
            }
        }
    }
    if (pEncInfo->ringBufferEnable == 0) {
        if (param->picStreamBufferAddr % 8 || param->picStreamBufferSize == 0) {
            return RETCODE_INVALID_PARAM;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode SetHecMode(EncHandle handle, int mode)
{
    return RETCODE_SUCCESS;
}

void EncSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr, PhysicalAddress paraAddr)
{
    BYTE tempBuf[8]={0,};                    // 64bit bus & endian
    Uint32 val;

    val =  paraAddr;
    tempBuf[0] = 0;
    tempBuf[1] = 0;
    tempBuf[2] = 0;
    tempBuf[3] = 0;
    tempBuf[4] = (val >> 24) & 0xff;
    tempBuf[5] = (val >> 16) & 0xff;
    tempBuf[6] = (val >> 8) & 0xff;
    tempBuf[7] = (val >> 0) & 0xff;
    VpuWriteMem(coreIdx, baseAddr, (BYTE *)tempBuf, 8, VDI_BIG_ENDIAN);
}


RetCode EnterDispFlagLock(Uint32 coreIdx)
{
    if (vdi_disp_lock(coreIdx) != 0)
        return RETCODE_FAILURE;
    return RETCODE_SUCCESS;
}

RetCode LeaveDispFlagLock(Uint32 coreIdx)
{
    vdi_disp_unlock(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode EnterLock(Uint32 coreIdx)
{
    if (vdi_lock(coreIdx) != 0)
        return RETCODE_FAILURE;
    SetClockGate(coreIdx, 1);
    return RETCODE_SUCCESS;
}

RetCode LeaveLock(Uint32 coreIdx)
{
    SetClockGate(coreIdx, 0);
    vdi_unlock(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode SetClockGate(Uint32 coreIdx, Uint32 on)
{
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip) {
        codec_slogerr("SetClockGate: RETCODE_INSUFFICIENT_RESOURCE\n");
        return RETCODE_INSUFFICIENT_RESOURCE;
    }

    vdi_set_clock_gate(coreIdx, on);

    return RETCODE_SUCCESS;
}

void SetPendingInst(Uint32 coreIdx, CodecInst *inst)
{
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return;

    vip->pendingInst = inst;
    if (inst)
        vip->pendingInstIdxPlus1 = (inst->instIndex+1);
    else
        vip->pendingInstIdxPlus1 = 0;
}

void ClearPendingInst(Uint32 coreIdx)
{
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return;

    if(vip->pendingInst) {
        vip->pendingInst = 0;
        vip->pendingInstIdxPlus1 = 0;
    }
}

CodecInst *GetPendingInst(Uint32 coreIdx)
{
    vpu_instance_pool_t *vip;
    int pendingInstIdx;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return NULL;

    if (!vip->pendingInst)
        return NULL;

    pendingInstIdx = vip->pendingInstIdxPlus1-1;
    if (pendingInstIdx < 0 || pendingInstIdx >= MAX_NUM_INSTANCE)
        return NULL;

    return  (CodecInst *)vip->codecInstPool[pendingInstIdx];
}

int GetPendingInstIdx(Uint32 coreIdx)
{
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
    if (!vip)
        return -1;

    return (vip->pendingInstIdxPlus1-1);
}

RetCode UpdateFrameBufferAddr(
    TiledMapType            mapType,
    FrameBuffer*            fbArr,
    Uint32                  numOfFrameBuffers,
    Uint32                  sizeLuma,
    Uint32                  sizeChroma
    )
{
    Uint32      i;
    BOOL        yuv422Interleave = FALSE;
    BOOL        fieldFrame       = 0;
    BOOL        cbcrInterleave   = (BOOL)(mapType >= COMPRESSED_FRAME_MAP || fbArr[0].cbcrInterleave == TRUE);
    BOOL        reuseFb          = FALSE;


    if (mapType < COMPRESSED_FRAME_MAP) {
        switch (fbArr[0].format) {
        case FORMAT_YUYV:
        case FORMAT_YUYV_P10_16BIT_MSB:
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_YVYU:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            yuv422Interleave = TRUE;
            break;
        default:
            yuv422Interleave = FALSE;
            break;
        }
    }

    for (i=0; i<numOfFrameBuffers; i++) {
        reuseFb = (fbArr[i].bufY != (PhysicalAddress)-1 && fbArr[i].bufCb != (PhysicalAddress)-1 && fbArr[i].bufCr != (PhysicalAddress)-1);
        if (reuseFb == FALSE) {
            if (yuv422Interleave == TRUE) {
                fbArr[i].bufCb = (PhysicalAddress)-1;
                fbArr[i].bufCr = (PhysicalAddress)-1;
            }
            else {
                if (fbArr[i].bufCb == (PhysicalAddress)-1) {
                    fbArr[i].bufCb = fbArr[i].bufY + (sizeLuma >> fieldFrame);
                }
                if (fbArr[i].bufCr == (PhysicalAddress)-1) {
                    if (cbcrInterleave == TRUE) {
                        fbArr[i].bufCr = (PhysicalAddress)-1;
                    }
                    else {
                        fbArr[i].bufCr = fbArr[i].bufCb + (sizeChroma >> fieldFrame);
                    }
                }
            }
        }
    }

    return RETCODE_SUCCESS;
}

static int GetXY2AXILogic(int map_val , int xpos, int ypos, int tb)
{
    int invert;
    int assign_zero;
    int tbxor;
    int xysel;
    int bitsel;

    int xypos,xybit,xybit_st1,xybit_st2,xybit_st3;

    invert      = map_val >> 7;
    assign_zero = (map_val & 0x78) >> 6;
    tbxor       = (map_val & 0x3C) >> 5;
    xysel       = (map_val & 0x1E) >> 4;
    bitsel      = map_val & 0x0f;

    xypos     = (xysel) ? ypos : xpos;
    xybit     = (xypos >> bitsel) & 0x01;
    xybit_st1 = (tbxor)       ? xybit^tb : xybit;
    xybit_st2 = (assign_zero) ? 0 : xybit_st1;
    xybit_st3 = (invert)      ? !xybit_st2 : xybit_st2;

    return xybit_st3;
}

static int GetXY2AXIAddr20(TiledMapConfig *pMapCfg, int ycbcr, int posY, int posX, int stride, FrameBuffer *fb)
{
    int tbSeparateMap;
    int use_linear_field;
    int ypos_field;
    int tb;
    int chr_flag;
    int ypos_mod;
    int i;
    int xy2axiLumMap;
    int xy2axiChrMap;
    int xy2axi_map_sel;
    int temp_bit;
    int axi_conv;

    int y_top_base;
    int cb_top_base;
    int cr_top_base;
    int y_bot_base;
    int cb_bot_base;
    int cr_bot_base;
    int top_base_addr;
    int bot_base_addr;
    int base_addr;
    int pix_addr;
    int mapType;
    mapType = fb->mapType;

    if (!pMapCfg)
        return -1;

    tbSeparateMap = pMapCfg->tbSeparateMap;
    use_linear_field = (mapType == 9);
    ypos_field = posY/2;
    tb = posY & 1;
    ypos_mod = (tbSeparateMap | use_linear_field) ? ypos_field : posY;
    chr_flag = (ycbcr >> 1) & 0x1;

    y_top_base = fb->bufY;
    cb_top_base = fb->bufCb;
    cr_top_base = fb->bufCr;
    y_bot_base = 0;
    cb_bot_base = 0;
    cr_bot_base = 0;

    if (mapType == LINEAR_FRAME_MAP)
    {
        base_addr = (ycbcr==0) ? y_top_base  : (ycbcr==2) ? cb_top_base : cr_top_base;
        pix_addr = ((posY * stride) + posX) + base_addr;
    }
    else
    {

        top_base_addr = (ycbcr==0) ? y_top_base  : (ycbcr==2) ? cb_top_base : cr_top_base;
        bot_base_addr = (ycbcr==0) ? y_bot_base  : (ycbcr==2) ? cb_bot_base : cr_bot_base;
        if (tbSeparateMap & tb)
            base_addr = bot_base_addr;
        else
            base_addr = top_base_addr;

        // axi_conv[31:0]
        axi_conv = 0;
        for (i=0 ; i<32; i++)
        {
            xy2axiLumMap = pMapCfg->xy2axiLumMap[i];
            xy2axiChrMap = pMapCfg->xy2axiChrMap[i];
            xy2axi_map_sel = (chr_flag) ? xy2axiChrMap : xy2axiLumMap;
            temp_bit = GetXY2AXILogic(xy2axi_map_sel,posX,ypos_mod,tb);
            axi_conv = axi_conv + (temp_bit << i);
        }

        pix_addr = axi_conv + base_addr;
    }

    return pix_addr;
}

int GetXY2AXIAddr(TiledMapConfig *pMapCfg, int ycbcr, int posY, int posX, int stride, FrameBuffer *fb)
{
    codec_slogtrace("%s", __func__);
    if (PRODUCT_ID_W_SERIES(pMapCfg->productId))
        return GetXY2AXIAddr20(pMapCfg, ycbcr, posY, posX, stride, fb);

    return 0;
}

Int32 CalcStride(
    Uint32              width,
    Uint32              height,
    FrameBufferFormat   format,
    BOOL                cbcrInterleave,
    TiledMapType        mapType,
    BOOL                isVP9
    )
{
    Uint32  lumaStride   = 0;
    Uint32  chromaStride = 0;

    lumaStride = VPU_ALIGN32(width);

    if (mapType == LINEAR_FRAME_MAP) {
        Uint32 twice = 0;

        twice = cbcrInterleave == TRUE ? 2 : 1;
        switch (format) {
        case FORMAT_420:
            /* nothing to do */
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_420_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_LSB:
            lumaStride = VPU_ALIGN32(width)*2;
            break;
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_420_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
            if (isVP9 == TRUE) {
                lumaStride   = VPU_ALIGN32(((width+11)/12)*16);
                chromaStride = (((width/2)+11)*twice/12)*16;
            }
            else {
                width = VPU_ALIGN32(width);
                lumaStride   = ((VPU_ALIGN16(width)+11)/12)*16;
                chromaStride = ((VPU_ALIGN16(width/2)+11)*twice/12)*16;
                if (cbcrInterleave == FALSE)
                {
                    if ( (chromaStride*2) > lumaStride)
                    {
                        lumaStride = chromaStride * 2;
                        codec_sloginfo("double chromaStride size is bigger than lumaStride\n");
                    }
                }
            }
            if (cbcrInterleave == TRUE) {
                lumaStride = VPU_ALIGN32(MAX(lumaStride, chromaStride));
            }
            break;
        case FORMAT_422:
            /* nothing to do */
            break;
        case FORMAT_YUYV:       // 4:2:2 8bit packed
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
            lumaStride = VPU_ALIGN32(width) * 2;
            break;
        case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
            lumaStride = VPU_ALIGN32(width) * 4;
            break;
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            lumaStride = VPU_ALIGN32(width*2)*2;
            break;
        default:
            break;
        }
    }
    else if (mapType == COMPRESSED_FRAME_MAP) {
        switch (format) {
        case FORMAT_420:
        case FORMAT_422:
        case FORMAT_YUYV:
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_420_P10_16BIT_MSB:
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_420_P10_32BIT_MSB:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_LSB:
        case FORMAT_422_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
        case FORMAT_YUYV_P10_16BIT_MSB:
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            lumaStride = VPU_ALIGN32(VPU_ALIGN16(width)*5)/4;
            lumaStride = VPU_ALIGN32(lumaStride);
            break;
        default:
            return -1;
        }
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_8BIT || mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_10BIT || mapType == COMPRESSED_FRAME_MAP_V50_LOSSY) {
        lumaStride = VPU_ALIGN32(width);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT || mapType == COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT) {
        Uint32 pad_x = 16;
        Uint32 valid_width = VPU_CEIL(width, 16);      // 16 align = BLK_WIDTH
        lumaStride  = VPU_CEIL(valid_width+pad_x, 16); // 16 align = BLK_WIDTH
    }
    else {
        width = (width < height) ? height : width;

        lumaStride = (width > 4096) ? 8192 :
                     (width > 2048) ? 4096 :
                     (width > 1024) ? 2048 :
                     (width >  512) ? 1024 : 512;
    }

    return lumaStride;
}

// 32 bit / 16 bit ==> 32-n bit remainder, n bit quotient
static int fixDivRq(int a, int b, int n)
{
    Int64 c;
    Int64 a_36bit;
    Int64 mask, signBit, signExt;
    int  i;

    // DIVS emulation for BPU accumulator size
    // For SunOS build
    mask = 0x0F; mask <<= 32; mask |= 0x00FFFFFFFF; // mask = 0x0FFFFFFFFF;
    signBit = 0x08; signBit <<= 32;                 // signBit = 0x0800000000;
    signExt = 0xFFFFFFF0; signExt <<= 32;           // signExt = 0xFFFFFFF000000000;

    a_36bit = (Int64) a;

    for (i=0; i<n; i++) {
        c =  a_36bit - (b << 15);
        if (c >= 0)
            a_36bit = (c << 1) + 1;
        else
            a_36bit = a_36bit << 1;

        a_36bit = a_36bit & mask;
        if (a_36bit & signBit)
            a_36bit |= signExt;
    }

    a = (int) a_36bit;
    return a;                           // R = [31:n], Q = [n-1:0]
}

static int math_div(int number, int denom)
{
    int  c;
    c = fixDivRq(number, denom, 17);             // R = [31:17], Q = [16:0]
    c = c & 0xFFFF;
    c = (c + 1) >> 1;                   // round
    return (c & 0xFFFF);
}

int LevelCalculation(int MbNumX, int MbNumY, int frameRateInfo, int interlaceFlag, int BitRate, int SliceNum)
{
    int mbps;
    int frameRateDiv, frameRateRes, frameRate;
    int mbPicNum = (MbNumX*MbNumY);
    int mbFrmNum;
    int MaxSliceNum;

    int LevelIdc = 0;
    int i, maxMbs;

    if (interlaceFlag)    {
        mbFrmNum = mbPicNum*2;
        MbNumY   *=2;
    }
    else                mbFrmNum = mbPicNum;

    frameRateDiv = (frameRateInfo >> 16) + 1;
    frameRateRes  = frameRateInfo & 0xFFFF;
    frameRate = math_div(frameRateRes, frameRateDiv);
    mbps = mbFrmNum*frameRate;

    for(i=0; i<MAX_LAVEL_IDX; i++)
    {
        maxMbs = g_anLevelMaxMbs[i];
        if ( mbps <= g_anLevelMaxMBPS[i]
        && mbFrmNum <= g_anLevelMaxFS[i]
        && MbNumX   <= maxMbs
            && MbNumY   <= maxMbs
            && BitRate  <= g_anLevelMaxBR[i] )
        {
            LevelIdc = g_anLevel[i];
            break;
        }
    }

    if (i==MAX_LAVEL_IDX)
        i = MAX_LAVEL_IDX-1;

    if (SliceNum)
    {
        SliceNum =  math_div(mbPicNum, SliceNum);

        if (g_anLevelSliceRate[i])
        {
            MaxSliceNum = math_div( MAX( mbPicNum, g_anLevelMaxMBPS[i]/( 172/( 1+interlaceFlag ) )), g_anLevelSliceRate[i] );

            if ( SliceNum> MaxSliceNum)
                return -1;
        }
    }

    return LevelIdc;
}

Int32 CalcLumaSize(
    CodecInst*        inst,
    Int32             productId,
    Int32             stride,
    Int32             height,
    FrameBufferFormat format,
    BOOL              cbcrIntl,
    TiledMapType      mapType,
    DRAMConfig        *pDramCfg
    )
{
    Uint32 unit_size_hor_lum, unit_size_ver_lum, size_dpb_lum, field_map;
    UNREFERENCED_PARAMETER(cbcrIntl);

    field_map = 0;

    if (mapType == LINEAR_FRAME_MAP) {
        size_dpb_lum = stride * height;
    }
    else if (mapType == COMPRESSED_FRAME_MAP) {
        size_dpb_lum = stride * height;
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_10BIT || mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_422_10BIT) {
        size_dpb_lum = WAVE5_ENC_FBC50_LOSSLESS_LUMA_10BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_8BIT || mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_422_8BIT) {
        size_dpb_lum = WAVE5_ENC_FBC50_LOSSLESS_LUMA_8BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSY || mapType == COMPRESSED_FRAME_MAP_V50_LOSSY_422) {
        if (pDramCfg == NULL)
            return 0;
        size_dpb_lum = WAVE5_ENC_FBC50_LOSSY_LUMA_FRAME_SIZE(stride, height, pDramCfg->tx16y);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT) {
        Uint32 pad_y = 4;
        Uint32 bgs_width;
        Uint32 bgs_height, valid_height, comp_frm_height, bgs_row_bytes, bgs_num_y;
        Uint32 size_dpb_lum_1024 = 0;
        //=====================================================================
        // BGS WIDTH calc for 8bit
        // if(h.264) bgs_width = 1024
        // else if (hevc_encoder) bgs_width = 512.
        // else if (hevc_decoder} MAX(comp_size for bgs_width=512, comp_size for bgs_width=1024)
        //=====================================================================
        if (inst->codecMode == W_AVC_DEC || inst->codecMode == W_AVC_ENC) {
            bgs_width = 1024;
        }
        else if (inst->codecMode == W_HEVC_ENC || inst->codecMode == W_HEVC_DEC) {
            bgs_width = 512;
        }
        else {
            codec_slogerr("CodecMode is not supported for COMPRESSED_FRAME_MAP_DUAL_CORE\n");
            return 0;
        }

        bgs_height = (1<<14) / bgs_width ;
        valid_height = VPU_CEIL(height, 4); // 4 = BLK_HEIGHT
        comp_frm_height = VPU_CEIL(valid_height + pad_y, bgs_height);
        bgs_row_bytes = VPU_CEIL(stride*bgs_height*8/8, 512); // 512 = BURST_SIZE
        bgs_num_y = comp_frm_height / bgs_height;
        size_dpb_lum = bgs_row_bytes * bgs_num_y;

        if (inst->codecMode == W_HEVC_DEC) {
            // In case of HEVC decoder, max comp_frame_size between bsg_width=512 and 1024 should be used.
            bgs_width = 1024;
            bgs_height = (1<<14) / bgs_width ;
            valid_height = VPU_CEIL(height, 4); // 4 = BLK_HEIGHT
            comp_frm_height = VPU_CEIL(valid_height + pad_y, bgs_height);
            bgs_row_bytes = VPU_CEIL(stride*bgs_height*8/8, 512); // 512 = BURST_SIZE
            bgs_num_y = comp_frm_height / bgs_height;
            size_dpb_lum_1024 = bgs_row_bytes * bgs_num_y;
            size_dpb_lum = MAX(size_dpb_lum, size_dpb_lum_1024);
        }
    }
    else if (mapType == COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT) {
        Uint32 pad_y = 4;
        Uint32 bgs_width;
        Uint32 bgs_height, valid_height, comp_frm_height, bgs_row_bytes, bgs_num_y;
        Uint32 size_dpb_lum_512 = 0;
        //=====================================================================
        // BGS WIDTH calc for 10bit
        // if(h.264) bgs_width = 512
        // else if (hevc_encoder) bgs_width = 256.
        // else if (hevc_decoder} MAX(comp_size for bgs_width=256, comp_size for bgs_width=512)
        //=====================================================================
        if (inst->codecMode == W_AVC_DEC || inst->codecMode == W_AVC_ENC) {
            bgs_width = 512;
        }
        else if (inst->codecMode == W_HEVC_ENC || inst->codecMode == W_HEVC_DEC) {
            bgs_width = 256;
        }
        else {
            codec_slogerr("CodecMode is not supported for COMPRESSED_FRAME_MAP_DUAL_CORE\n");
            return 0;
        }

        bgs_height = (1<<14) / bgs_width / 2 ;
        valid_height = VPU_CEIL(height, 4); // 4 = BLK_HEIGHT
        comp_frm_height = VPU_CEIL(valid_height + pad_y, bgs_height);
        bgs_row_bytes = VPU_CEIL(stride*bgs_height*10/8, 512); // 512 = BURST_SIZE
        bgs_num_y = comp_frm_height / bgs_height;
        size_dpb_lum = bgs_row_bytes * bgs_num_y;

        if (inst->codecMode == W_HEVC_DEC) {
            bgs_width = 512;
            bgs_height = (1<<14) / bgs_width / 2 ;
            valid_height = VPU_CEIL(height, 4); // 4 = BLK_HEIGHT
            comp_frm_height = VPU_CEIL(valid_height + pad_y, bgs_height);
            bgs_row_bytes = VPU_CEIL(stride*bgs_height*10/8, 512); // 512 = BURST_SIZE
            bgs_num_y = comp_frm_height / bgs_height;
            size_dpb_lum_512 = bgs_row_bytes * bgs_num_y;

            size_dpb_lum = MAX(size_dpb_lum, size_dpb_lum_512);
        }
    }
    else {
            unit_size_hor_lum = stride;
            unit_size_ver_lum = (((height>>field_map)+63)/64) * 64; // unit vertical size is 64 pixel (4MB)
            size_dpb_lum      = unit_size_hor_lum * (unit_size_ver_lum<<field_map);
    }

    return size_dpb_lum;
}

Int32 CalcChromaSize(
    CodecInst*          inst,
    Int32               productId,
    Int32               stride,
    Int32               height,
    FrameBufferFormat   format,
    BOOL                cbcrIntl,
    TiledMapType        mapType,
    DRAMConfig*         pDramCfg
    )
{
    Int32  chr_size_y, chr_size_x;
    Int32  chr_vscale, chr_hscale;
    Int32  unit_size_hor_chr, unit_size_ver_chr;
    Uint32 size_dpb_chr;
    Int32  field_map;

    unit_size_hor_chr = 0;
    unit_size_ver_chr = 0;

    chr_hscale = 1;
    chr_vscale = 1;

    switch (format) {
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_420:
        chr_hscale = 2;
        chr_vscale = 2;
        break;
    case FORMAT_224:
        chr_vscale = 2;
        break;
    case FORMAT_422:
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_422_P10_32BIT_MSB:
        chr_hscale = 2;
        break;
    case FORMAT_444:
    case FORMAT_400:
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
    case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        break;
    default:
        return 0;
    }

    field_map = 0;

    if (mapType == LINEAR_FRAME_MAP) {

        switch (format) {
        case FORMAT_420:
            unit_size_hor_chr = stride/2;
            unit_size_ver_chr = height/2;
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_420_P10_16BIT_MSB:
            // 1p2b stride = align32(W)*2;
            unit_size_hor_chr = stride/2;
            unit_size_ver_chr = height/2;
            break;
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_420_P10_32BIT_MSB:
            unit_size_hor_chr = VPU_ALIGN16(stride/2);
            unit_size_ver_chr = height/2;
            break;
        case FORMAT_422:
        case FORMAT_422_P10_16BIT_LSB:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
        case FORMAT_422_P10_32BIT_MSB:
            unit_size_hor_chr = VPU_ALIGN32(stride/2);
            unit_size_ver_chr = height;
            break;
        case FORMAT_YUYV:
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
        case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
        case FORMAT_YUYV_P10_16BIT_LSB:
        case FORMAT_YVYU_P10_16BIT_MSB:
        case FORMAT_YVYU_P10_16BIT_LSB:
        case FORMAT_UYVY_P10_16BIT_MSB:
        case FORMAT_UYVY_P10_16BIT_LSB:
        case FORMAT_VYUY_P10_16BIT_MSB:
        case FORMAT_VYUY_P10_16BIT_LSB:
        case FORMAT_YUYV_P10_32BIT_MSB:
        case FORMAT_YUYV_P10_32BIT_LSB:
        case FORMAT_YVYU_P10_32BIT_MSB:
        case FORMAT_YVYU_P10_32BIT_LSB:
        case FORMAT_UYVY_P10_32BIT_MSB:
        case FORMAT_UYVY_P10_32BIT_LSB:
        case FORMAT_VYUY_P10_32BIT_MSB:
        case FORMAT_VYUY_P10_32BIT_LSB:
            unit_size_hor_chr = 0;
            unit_size_ver_chr = 0;
            break;
        default:
            break;
        }
        size_dpb_chr = (format == FORMAT_400) ? 0 : unit_size_ver_chr * unit_size_hor_chr;
    }
    else if (mapType == COMPRESSED_FRAME_MAP) {
        switch (format) {
        case FORMAT_420:
        case FORMAT_YUYV:       // 4:2:2 8bit packed
        case FORMAT_YVYU:
        case FORMAT_UYVY:
        case FORMAT_VYUY:
            size_dpb_chr = VPU_ALIGN16(stride/2)*height;
            break;
        default:
            /* 10bit */
            stride = VPU_ALIGN64(stride/2)+12; /* FIXME: need width information */
            size_dpb_chr = VPU_ALIGN32(stride)*VPU_ALIGN4(height);
            break;
        }
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_10BIT) {
        size_dpb_chr = WAVE5_ENC_FBC50_LOSSLESS_CHROMA_10BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_8BIT) {
        size_dpb_chr = WAVE5_ENC_FBC50_LOSSLESS_CHROMA_8BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSY) {
        if (pDramCfg == NULL)
            return 0;
        size_dpb_chr = WAVE5_ENC_FBC50_LOSSY_CHROMA_FRAME_SIZE(stride, height, pDramCfg->tx16c);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_422_10BIT) {
        size_dpb_chr = WAVE5_ENC_FBC50_LOSSLESS_422_CHROMA_10BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSLESS_422_8BIT) {
        size_dpb_chr = WAVE5_ENC_FBC50_LOSSLESS_422_CHROMA_8BIT_FRAME_SIZE(stride, height);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_V50_LOSSY_422) {
        if (pDramCfg == NULL)
            return 0;
        size_dpb_chr = WAVE5_ENC_FBC50_LOSSY_422_CHROMA_FRAME_SIZE(stride, height, pDramCfg->tx16c);
    }
    else if (mapType == COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT) {
        Uint32 pad_y = 4;
        Uint32 pad_x = 16;
        Uint32 bgs_width;
        Uint32 bgs_height, valid_width, comp_frm_width, valid_height, comp_frm_height, bgs_row_bytes, bgs_num_y;
        Uint32 size_dpb_chr_1024 = 0;
        Uint32 width = stride;
        //=====================================================================
        // BGS WIDTH calc for 8bit
        // if(h.264) bgs_width = 1024
        // else if (hevc_encoder) bgs_width = 512.
        // else if (hevc_decoder} MAX(comp_size for bgs_width=512, comp_size for bgs_width=1024)
        //=====================================================================
        if (inst->codecMode == W_AVC_DEC || inst->codecMode == W_AVC_ENC) {
            bgs_width = 1024;
        }
        else if (inst->codecMode == W_HEVC_ENC || inst->codecMode == W_HEVC_DEC) {
            bgs_width = 512;
        }
        else {
            codec_slogerr("CodecMode is not supported for COMPRESSED_FRAME_MAP_DUAL_CORE\n");
            return 0;
        }

        if (IS_WAVE_DECODER_HANDLE(inst) == TRUE)
            width = inst->CodecInfo->decInfo.initialInfo.picWidth;
        else
            width = inst->CodecInfo->encInfo.openParam.picWidth;

        bgs_height = (1<<14) / bgs_width ;
        valid_width = VPU_CEIL(width/2, 16);      // 16 align = BLK_WIDTH
        comp_frm_width  = VPU_CEIL(valid_width+pad_x, 16); // 16 align = BLK_WIDTH
        valid_height = VPU_CEIL(height, 4); // 4 = BLK_HEIGHT
        comp_frm_height = VPU_CEIL(valid_height + pad_y, bgs_height);
        bgs_row_bytes = VPU_CEIL(comp_frm_width*bgs_height*8/8, 512); // 512 = BURST_SIZE
        bgs_num_y = comp_frm_height / bgs_height;
        size_dpb_chr = bgs_row_bytes * bgs_num_y;

        if (inst->codecMode == W_HEVC_DEC) {
            bgs_width = 1024;
            bgs_height = (1<<14) / bgs_width ;
            valid_width = VPU_CEIL(width/2, 16);      // 16 align = BLK_WIDTH
            comp_frm_width  = VPU_CEIL(valid_width+pad_x, 16); // 16 align = BLK_WIDTH
            valid_height = VPU_CEIL(height, 4); // 4 = BLK_HEIGHT
            comp_frm_height = VPU_CEIL(valid_height + pad_y, bgs_height);
            bgs_row_bytes = VPU_CEIL(comp_frm_width*bgs_height*8/8, 512); // 512 = BURST_SIZE
            bgs_num_y = comp_frm_height / bgs_height;
            size_dpb_chr_1024 = bgs_row_bytes * bgs_num_y;

            size_dpb_chr = MAX(size_dpb_chr, size_dpb_chr_1024);
        }
    }
    else if (mapType == COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT) {
        Uint32 pad_x = 16;
        Uint32 pad_y = 4;
        Uint32 bgs_width;
        Uint32 bgs_height, valid_width, comp_frm_width, valid_height, comp_frm_height, bgs_row_bytes, bgs_num_y;
        Uint32 size_dpb_chr_512 = 0;
        Uint32 width = stride;
        //=====================================================================
        // BGS WIDTH calc for 10bit
        // if(h.264) bgs_width = 512
        // else if (hevc_encoder) bgs_width = 256.
        // else if (hevc_decoder} MAX(comp_size for bgs_width=256, comp_size for bgs_width=512)
        //=====================================================================
        if (inst->codecMode == W_AVC_DEC || inst->codecMode == W_AVC_ENC) {
            bgs_width = 512;
        }
        else if (inst->codecMode == W_HEVC_ENC || inst->codecMode == W_HEVC_DEC) {
            bgs_width = 256;
        }
        else {
            codec_slogerr("CodecMode is not supported for COMPRESSED_FRAME_MAP_DUAL_CORE\n");
            return 0;
        }
        if (IS_WAVE_DECODER_HANDLE(inst) == TRUE)
            width = inst->CodecInfo->decInfo.initialInfo.picWidth;
        else
            width = inst->CodecInfo->encInfo.openParam.picWidth;

        bgs_height = (1<<14) / bgs_width / 2 ;
        valid_width = VPU_CEIL(width/2, 16);      // 16 align = BLK_WIDTH
        comp_frm_width  = VPU_CEIL(valid_width+pad_x, 16); // 16 align = BLK_WIDTH
        valid_height = VPU_CEIL(height, 4); // 4 = BLK_HEIGHT
        comp_frm_height = VPU_CEIL(valid_height + pad_y, bgs_height);
        bgs_row_bytes = VPU_CEIL(comp_frm_width*bgs_height*10/8, 512); // 512 = BURST_SIZE
        bgs_num_y = comp_frm_height / bgs_height;
        size_dpb_chr = bgs_row_bytes * bgs_num_y;

        if (inst->codecMode == W_HEVC_DEC) {
            bgs_width = 512;
            bgs_height = (1<<14) / bgs_width / 2 ;
            valid_width = VPU_CEIL(width/2, 16);      // 16 align = BLK_WIDTH
            comp_frm_width  = VPU_CEIL(valid_width+pad_x, 16); // 16 align = BLK_WIDTH
            valid_height = VPU_CEIL(height, 4); // 4 = BLK_HEIGHT
            comp_frm_height = VPU_CEIL(valid_height + pad_y, bgs_height);
            bgs_row_bytes = VPU_CEIL(comp_frm_width*bgs_height*10/8, 512); // 512 = BURST_SIZE
            bgs_num_y = comp_frm_height / bgs_height;
            size_dpb_chr_512 = bgs_row_bytes * bgs_num_y;

            size_dpb_chr = MAX(size_dpb_chr, size_dpb_chr_512);
        }
    }
    else {
            chr_size_y = (height>>field_map)/chr_hscale;
            chr_size_x = cbcrIntl == TRUE ? stride : stride/chr_vscale;

            unit_size_hor_chr = (chr_size_x> 4096) ? 8192:
                                (chr_size_x> 2048) ? 4096 :
                                (chr_size_x > 1024) ? 2048 :
                                (chr_size_x >  512) ? 1024 : 512;
            unit_size_ver_chr = ((chr_size_y+63)/64) * 64; // unit vertical size is 64 pixel (4MB)

            size_dpb_chr  = (format==FORMAT_400) ? 0 : unit_size_hor_chr * (unit_size_ver_chr<<field_map);
            size_dpb_chr /= (cbcrIntl == TRUE ? 2 : 1);
    }
    return size_dpb_chr;
}

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
typedef struct  {
    int core_idx;
#ifdef SUPPORT_SW_UART_ON_NONOS
    char uartTx[1024];
#else
    pthread_t thread_id;
#endif
    int sw_uart_thread_run;
} SwUartContext;
static SwUartContext s_SwUartContext;
void SwUartHandler(void *context)
{
    unsigned int regSwUartStatus;
    unsigned int regSwUartTxData;
    unsigned char *strRegSwUartTxData;
    int i = 0;
#ifdef SUPPORT_SW_UART_ON_NONOS
    char *uartTx = &s_SwUartContext.uartTx[0];
#else
    char uartTx[1024];
#endif

#ifdef SUPPORT_SW_UART_ON_NONOS
#else
    codec_sloginfo("enter %s \n", __FUNCTION__);
#endif
#ifdef SUPPORT_SW_UART_ON_NONOS
#else
    osal_memset(uartTx, 0, sizeof(char)*1024);
#endif
#ifdef SUPPORT_SW_UART_ON_NONOS
    // if (s_SwUartContext.sw_uart_thread_run != 1)
    //     return;
#else
    while(s_SwUartContext.sw_uart_thread_run == 1)
#endif
    {
        regSwUartStatus = vdi_read_register(s_SwUartContext.core_idx, W5_SW_UART_STATUS);
        if (regSwUartStatus == (unsigned int)-1)
        {
#ifdef SUPPORT_SW_UART_ON_NONOS
            return;
#else
            continue;
#endif
        }

        if (regSwUartStatus == 0)
        {
            vdi_write_register(0, W5_SW_UART_STATUS,  (1<<0));
            regSwUartStatus = vdi_read_register(s_SwUartContext.core_idx, W5_SW_UART_STATUS);
        }
        if ((regSwUartStatus & (1<<1)))
        {
            regSwUartTxData = vdi_read_register(s_SwUartContext.core_idx, W5_SW_UART_TX_DATA);
            if (regSwUartTxData == (unsigned int)-1)
            {
#ifdef SUPPORT_SW_UART_ON_NONOS
                return;
#else
                continue;
#endif
            }
            regSwUartStatus &= ~(1<<1);
            vdi_write_register(s_SwUartContext.core_idx, W5_SW_UART_STATUS, regSwUartStatus);
            strRegSwUartTxData = (unsigned char *)&regSwUartTxData;
            for (i=0; i < 4; i++)
            {
                if (strRegSwUartTxData[i] == '\n')
                {
                    codec_sloginfo("[%" PRIu64 "] %s \n", osal_gettime(), uartTx+1);
                    osal_memset(uartTx, 0, sizeof(unsigned char)*1024);
                }
                else
                {
                    strncat((char *)uartTx, (const char *)(strRegSwUartTxData + i), 1);
                }
            }
        }

    }
#ifdef SUPPORT_SW_UART_ON_NONOS
#else
    codec_sloginfo("exit %s \n", __FUNCTION__);
#endif

}

int create_sw_uart_thread(unsigned long coreIdx)
{
#ifdef SUPPORT_SW_UART_ON_NONOS
#else
    int ret;
#endif

    if (s_SwUartContext.sw_uart_thread_run == 1)
        return 1;

    vdi_write_register(coreIdx, W5_SW_UART_STATUS,  (1<<0)); // enable SW UART. this will be checked by firmware to know SW UART enabled


    s_SwUartContext.core_idx = coreIdx;
    s_SwUartContext.sw_uart_thread_run = 1;

#ifdef SUPPORT_SW_UART_ON_NONOS
    osal_memset(s_SwUartContext.uartTx, 0, sizeof(char)*1024);
    codec_sloginfo("enter %s \n", __FUNCTION__);
#else
    ret = pthread_create(&s_SwUartContext.thread_id, NULL, (void*)SwUartHandler, &s_SwUartContext);

    if (ret != 0)
    {
        destory_sw_uart_thread(coreIdx);
        return 0;
    }
#endif
    return 1;
}

void destory_sw_uart_thread(unsigned long coreIdx)
{
    int inst_num;
    int task_num;
    int i;

    inst_num = 0;
    for (i=0; i < MAX_NUM_VPU_CORE; i++)
    {
        inst_num = inst_num + vdi_get_instance_num(i);
    }

    if (inst_num > 0)
        return;

    task_num = 0;
    for (i=0; i < MAX_NUM_VPU_CORE; i++)
    {
        task_num = task_num + vdi_get_task_num(i);
    }

    if (task_num > 1)
        return;

    if (s_SwUartContext.sw_uart_thread_run == 1)
    {
        s_SwUartContext.sw_uart_thread_run = 0;

        vdi_write_register(coreIdx, W5_SW_UART_STATUS, 0); // disable SW UART. this will be checked by firmware to know SW UART enabled

#ifdef SUPPORT_SW_UART_ON_NONOS
        codec_sloginfo("exit %s \n", __FUNCTION__);
#else
        if (s_SwUartContext.thread_id)
        {
            pthread_join(s_SwUartContext.thread_id, NULL);
            s_SwUartContext.thread_id = 0;
        }
#endif
    }

    return ;
}
#endif
