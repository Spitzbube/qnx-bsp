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

#ifndef _MAIN_HELPER_H_
#define _MAIN_HELPER_H_

#include <stdio.h>
#include <ctype.h>
#include "config.h"
#include "vpuapifunc.h"
#include "vpuapi.h"
#include "vputypes.h"
#ifdef PLATFORM_QNX
    #include <sys/stat.h>
#endif

#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define MATCH_OR_MISMATCH(_expected, _value, _ret)        ((_ret=(_expected == _value)) ? "MATCH" : "MISMATCH")

#define MAX_GETOPT_OPTIONS 100
//extension of option struct in getopt
struct OptionExt
{
    const char *name;
    int has_arg;
    int *flag;
    int val;
    const char *help;
};

#define MAX_FILE_PATH               256
#define MAX_PIC_SKIP_NUM            5



#define EXTRA_SRC_BUFFER_NUM            0
#define VPU_WAIT_TIME_OUT               10  //should be less than normal decoding time to give a chance to fill stream. if this value happens some problem. we should fix VPU_WaitInterrupt function
#define VPU_WAIT_TIME_OUT_CQ            1
#define VPU_WAIT_TIME_OUT_LONG          5000
#define MAX_NOT_DEC_COUNT               2000
#define COMPARE_RESOLUTION(_src, _dst)  (_src->width == _dst->width && _src->height == _dst->height)

/************************************************************************/
/* User Parameters (ENCODER)                                            */
/************************************************************************/
// user scaling list
#define SL_NUM_MATRIX (6)

typedef struct
{
    Uint8 s4[SL_NUM_MATRIX][16]; // [INTRA_Y/U/V,INTER_Y/U/V][NUM_COEFF]
    Uint8 s8[SL_NUM_MATRIX][64];
    Uint8 s16[SL_NUM_MATRIX][64];
    Uint8 s32[SL_NUM_MATRIX][64];
    Uint8 s16dc[SL_NUM_MATRIX];
    Uint8 s32dc[2];
}UserScalingList;

enum ScalingListSize
{
    SCALING_LIST_4x4 = 0,
    SCALING_LIST_8x8,
    SCALING_LIST_16x16,
    SCALING_LIST_32x32,
    SCALING_LIST_SIZE_NUM
};

int parse_user_scaling_list(
    UserScalingList* sl,
    FILE* fp_sl,
    CodStd  stdMode
    );

// custom lambda
#define NUM_CUSTOM_LAMBDA   (2*52)
int parse_custom_lambda(Uint32 buf[NUM_CUSTOM_LAMBDA], FILE* fp);

int mkdir_recursive(char *path, mode_t omode);

typedef union {
    struct {
        Uint32  ctu_force_mode  :  2; //[ 1: 0]
        Uint32  ctu_coeff_drop  :  1; //[    2]
        Uint32  reserved        :  5; //[ 7: 3]
        Uint32  sub_ctu_qp_0    :  6; //[13: 8]
        Uint32  sub_ctu_qp_1    :  6; //[19:14]
        Uint32  sub_ctu_qp_2    :  6; //[25:20]
        Uint32  sub_ctu_qp_3    :  6; //[31:26]

        Uint32  lambda_sad_0    :  8; //[39:32]
        Uint32  lambda_sad_1    :  8; //[47:40]
        Uint32  lambda_sad_2    :  8; //[55:48]
        Uint32  lambda_sad_3    :  8; //[63:56]
    } field;
} EncCustomMap; // for wave5xx custom map (1 CTU = 64bits)

typedef union {
    struct {
        Uint8  mb_force_mode  :  2; //lint !e46 [ 1: 0]
        Uint8  mb_qp          :  6; //lint !e46 [ 7: 2]
    } field;
} AvcEncCustomMap; // for AVC custom map on wave  (1 MB = 8bits)

typedef enum {
    MODE_YUV_LOAD = 0,
    MODE_COMP_JYUV,
    MODE_SAVE_JYUV,

    MODE_COMP_CONV_YUV,
    MODE_SAVE_CONV_YUV,

    MODE_SAVE_LOAD_YUV,

    MODE_COMP_RECON,
    MODE_SAVE_RECON,

    MODE_COMP_ENCODED,
    MODE_SAVE_ENCODED
} CompSaveMode;

typedef enum {
    ENCODER_STATE_OPEN,
    ENCODER_STATE_INIT_SEQ,
    ENCODER_STATE_REGISTER_FB,
    ENCODER_STATE_ENCODE_HEADER,
    ENCODER_STATE_ENCODING,
} EncoderState;

typedef struct tivpu_enc_config_t
{
    Int32  bitFormat;
    Int32  cbcrInterleave;
    Int32  packedFormat;
    Uint32 width;
    Uint32 height;
    Uint32 stride;
    Uint32 profile;
    Uint32 level;
    Uint32 framerate;
    Uint32 keyFrameInterval;
    Uint32 qpI;
    Uint32 qpP;
    Uint32 bitrate;
    Uint32 rateControl;
    BOOL arithmeticEncoding;
    Uint32 sliceType;
    Uint32 sliceSize;
    Uint32 coreIdx;
    Uint32 sourceBufCount;
    Uint32 streamBufCount;
    Uint32 streamBufSize;
    Uint32 setLossless;
    Uint32 setGOP;
} tivpu_enc_config_t;

typedef struct {
    int picX;
    int picY;
    int internalBitDepth;
    int losslessEnable;
    int constIntraPredFlag;
    int gopSize;
    int numTemporalLayers;
    int decodingRefreshType;
    int intraQP;
    int intraPeriod;
    int frameRate;

    int confWinTop;
    int confWinBot;
    int confWinLeft;
    int confWinRight;

    int independSliceMode;
    int independSliceModeArg;
    int dependSliceMode;
    int dependSliceModeArg;
    int intraRefreshMode;
    int intraRefreshArg;

    int useRecommendEncParam;
    int scalingListEnable;
    int cuSizeMode;
    int tmvpEnable;
    int wppenable;
    int maxNumMerge;

    int disableDeblk;
    int lfCrossSliceBoundaryEnable;
    int betaOffsetDiv2;
    int tcOffsetDiv2;
    int skipIntraTrans;
    int saoEnable;
    int intraNxNEnable;
    int rcEnable;

    int bitRate;
    int bitAllocMode;
    int fixedBitRatio[MAX_GOP_NUM];
    int cuLevelRCEnable;
    int hvsQPEnable;

    int hvsQpScale;
    int minQp;
    int maxQp;
    int maxDeltaQp;

    int gopPresetIdx;
    // CUSTOM_GOP
    CustomGopParam gopParam;

    // ROI / CTU mode
    int roiEnable;                      /**< It enables ROI map. NOTE: It is valid when rcEnable is on. */
    char roiFileName[MAX_FILE_PATH];
    char roiQpMapFile[MAX_FILE_PATH];

    // VUI
    Uint32 numUnitsInTick;
    Uint32 timeScale;
    Uint32 numTicksPocDiffOne;

    int encAUD;
    int encEOS;
    int encEOB;

    int chromaCbQpOffset;
    int chromaCrQpOffset;

    Uint32 initialRcQp;

    Uint32  nrYEnable;
    Uint32  nrCbEnable;
    Uint32  nrCrEnable;
    Uint32  nrNoiseEstEnable;
    Uint32  nrNoiseSigmaY;
    Uint32  nrNoiseSigmaCb;
    Uint32  nrNoiseSigmaCr;

    Uint32  nrIntraWeightY;
    Uint32  nrIntraWeightCb;
    Uint32  nrIntraWeightCr;

    Uint32  nrInterWeightY;
    Uint32  nrInterWeightCb;
    Uint32  nrInterWeightCr;

    Uint32 useAsLongtermPeriod;
    Uint32 refLongtermPeriod;

    // newly added for encoder
    Uint32 monochromeEnable;
    Uint32 strongIntraSmoothEnable;
    Uint32 roiAvgQp;
    Uint32 weightPredEnable;
    Uint32 bgDetectEnable;
    Uint32 bgThrDiff;
    Uint32 bgThrMeanDiff;
    Uint32 bgLambdaQp;
    int    bgDeltaQp;
    Uint32 lambdaMapEnable;
    Uint32 customLambdaEnable;
    Uint32 customMDEnable;
    int    pu04DeltaRate;
    int    pu08DeltaRate;
    int    pu16DeltaRate;
    int    pu32DeltaRate;
    int    pu04IntraPlanarDeltaRate;
    int    pu04IntraDcDeltaRate;
    int    pu04IntraAngleDeltaRate;
    int    pu08IntraPlanarDeltaRate;
    int    pu08IntraDcDeltaRate;
    int    pu08IntraAngleDeltaRate;
    int    pu16IntraPlanarDeltaRate;
    int    pu16IntraDcDeltaRate;
    int    pu16IntraAngleDeltaRate;
    int    pu32IntraPlanarDeltaRate;
    int    pu32IntraDcDeltaRate;
    int    pu32IntraAngleDeltaRate;
    int    cu08IntraDeltaRate;
    int    cu08InterDeltaRate;
    int    cu08MergeDeltaRate;
    int    cu16IntraDeltaRate;
    int    cu16InterDeltaRate;
    int    cu16MergeDeltaRate;
    int    cu32IntraDeltaRate;
    int    cu32InterDeltaRate;
    int    cu32MergeDeltaRate;
    int    coefClearDisable;
    int    forcePicSkipStart;
    int    forcePicSkipEnd;
    int    forceCoefDropStart;
    int    forceCoefDropEnd;
    char   scalingListFileName[MAX_FILE_PATH];
    char   customLambdaFileName[MAX_FILE_PATH];

    Uint32 enStillPicture;

    // custom map
    int    customLambdaMapEnable;
    char   customLambdaMapFileName[MAX_FILE_PATH];
    int    customModeMapFlag;
    char   customModeMapFileName[MAX_FILE_PATH];

    char   WpParamFileName[MAX_FILE_PATH];

    // for H.264 on WAVE
    int idrPeriod;
    int rdoSkip;
    int lambdaScalingEnable;
    int transform8x8;
    int avcSliceMode;
    int avcSliceArg;
    int intraMbRefreshMode;
    int intraMbRefreshArg;
    int mbLevelRc;
    int entropyCodingMode;

    int s2fmeDisable;
    int forceIdrPicIdx;
    int forcedIdrHeaderEnable;
    Uint32 vuiDataEnable;
    Uint32 vuiDataSize;
    char   vuiDataFileName[MAX_FILE_PATH];
    Uint32 hrdInVPS;
    Uint32 hrdDataSize;
    char   hrdDataFileName[MAX_FILE_PATH];
    Uint32 prefixSeiEnable;
    Uint32 prefixSeiDataSize;
    char   prefixSeiDataFileName[MAX_FILE_PATH];
    Uint32 suffixSeiEnable;
    Uint32 suffixSeiDataSize;
    char   suffixSeiDataFileName[MAX_FILE_PATH];
    Uint32 rcWeightParam;
    Uint32 rcWeightBuf;
} WAVE_ENC_CFG;

typedef struct {
    // ChangePara
    int setParaChgFrmNum;
    int enableOption;
    char cfgName[MAX_FILE_PATH];
} W5ChangeParam;

typedef struct {
    char SrcFileName[MAX_FILE_PATH];
    char BitStreamFileName[MAX_FILE_PATH];
    BOOL srcCbCrInterleave;
    int NumFrame;
    int PicX;
    int PicY;
    int FrameRate;

    // MPEG4 ONLY
    int VerId;
    int DataPartEn;
    int RevVlcEn;
    int ShortVideoHeader;
    int AnnexI;
    int AnnexJ;
    int AnnexK;
    int AnnexT;
    int IntraDcVlcThr;
    int VopQuant;

    // H.264 ONLY
    int ConstIntraPredFlag;
    int DisableDeblk;
    int DeblkOffsetA;
    int DeblkOffsetB;
    int ChromaQpOffset;
    int PicQpY;
    // H.264 VUI information
    int VuiPresFlag;
    int VideoSignalTypePresFlag;
    char VideoFormat;
    char VideoFullRangeFlag;
    int ColourDescripPresFlag;
    char ColourPrimaries;
    char TransferCharacteristics;
    char MatrixCoeff;
    int NumReorderFrame;
    int MaxDecBuffering;
    int aud_en;
    int level;
    // COMMON
    int GopPicNum;
    int SliceMode;
    int SliceSizeMode;
    int SliceSizeNum;
    // COMMON - RC
    int RcEnable;
    int RcBitRate;
    int RcBitRateBL;
    int RcInitDelay;
    int VbvBufferSize;
    int RcBufSize;
    int IntraRefreshNum;
    int ConscIntraRefreshEnable;
    int ConstantIntraQPEnable;
    int MaxQpSetEnable;
    int MaxQp;

    int frameCroppingFlag;
    int frameCropLeft;
    int frameCropRight;
    int frameCropTop;
    int frameCropBottom;

    //H.264 only
    int MaxDeltaQpSetEnable;
    int MaxDeltaQp;
    int MinQpSetEnable;
    int MinQp;
    int MinDeltaQpSetEnable;
    int MinDeltaQp;
    int intraCostWeight;

    //MP4 Only
    int RCIntraQP;
    int HecEnable;

    int GammaSetEnable;
    int Gamma;

    // NEW RC Scheme
    int skipPicNums[MAX_PIC_SKIP_NUM];

    int MeUseZeroPmv;	// will be removed. must be 264 = 0, mpeg4 = 1 263 = 0
    int MeBlkModeEnable; // only api option
    int IDRInterval;
    int SrcBitDepth;
    int Profile;

    WAVE_ENC_CFG waveCfg;

    int numChangeParam;
    W5ChangeParam changeParam[10];
    int rcWeightFactor;
} ENC_CFG;

typedef struct {
    Uint32 subFrameSyncOn;
    Uint32 subFrameSyncSrcWriteMode;                /**< It indicates the number of pixel rows to run VPU_EncStartOneFrame() with. (default/min: 64, max: picture height) This is used for test purpose.  */
    EncSubFrameSyncState subFrameSyncState;         /**< representing the status of Subframe Sync(SFS) when SFS works in register-based mode.*/
} ENC_subFrameSyncCfg; /* SFS = SubFrameSync */

typedef struct ParamEncNeedFrameBufferNum {
    Uint32  reconFbNum;
    Uint32  srcFbNum;
} ParamEncNeedFrameBufferNum;

typedef struct ParamEncFrameBuffer {
    Uint32               reconFbStride;
    Uint32               reconFbHeight;
    FrameBuffer*         reconFb;
    FrameBuffer*         srcFb;
    FrameBufferAllocInfo reconFbAllocInfo;
    FrameBufferAllocInfo srcFbAllocInfo;
} ParamEncFrameBuffer;

typedef struct ParamEncBitstreamBuffer {
    Uint32          num;
    vpu_buffer_t*   bs;
} ParamEncBitstreamBuffer;

typedef struct {
    EncHandle                   handle;
    EncOpenParam                encOpenParam;
    ParamEncNeedFrameBufferNum  fbCount;
    Uint32                      fbCountValid;
    Uint32                      srcStride;
    FrameBufferAllocInfo        srcFbAllocInfo;
    FrameBufferAllocInfo        reconFbAllocInfo;
    BOOL                        fbAllocated;
    Uint32                      frameIdx;
    vpu_buffer_t                vbCustomLambda;
    vpu_buffer_t                vbScalingList;
    Uint32                      customLambda[NUM_CUSTOM_LAMBDA];
    UserScalingList             scalingList;
    vpu_buffer_t                vbCustomMap[MAX_REG_FRAME];
    vpu_buffer_t                vbPrefixSeiNal[MAX_REG_FRAME];
    vpu_buffer_t                vbSuffixSeiNal[MAX_REG_FRAME];
    vpu_buffer_t                vbHrdRbsp;
    vpu_buffer_t                vbVuiRbsp;
    EncoderState                state;
    BOOL                        stateDoing;
    BOOL                        stop;
    BOOL                        terminate;
    EncInitialInfo              initialInfo;
    EncParam                    encParam;
    Int32                       encodedSrcFrmIdxArr[ENC_SRC_BUF_NUM];
    ParamEncBitstreamBuffer     bsBuf;
    BOOL                        fullInterrupt;
    Uint32                      changedCount;
    Uint64                      startTimeout;
    Uint64                      desStTimeout;
    Uint32                      iterationCnt;
    VpuAttr                     attr;
    ENC_subFrameSyncCfg         subFrameSyncCfg;
    Uint32                      cyclePerTick;
    vpu_buffer_t                *bsBuffer[20];
    char                        inputPath[MAX_FILE_PATH];
    Int32                       rotAngle;
    Int32                       mirDir;
    Int32                       frameOutNum;
    Int32                       yuvMode;
    FrameBuffer                 pFbRecon[MAX_REG_FRAME];
    vpu_buffer_t                pFbReconMem[MAX_REG_FRAME];
    FrameBuffer                 pFbSrc[ENC_SRC_BUF_NUM];
    vpu_buffer_t                pFbSrcMem[ENC_SRC_BUF_NUM];
    FrameBuffer                 pFbOffsetTbl[ENC_SRC_BUF_NUM];
    vpu_buffer_t                pFbOffsetTblMem[ENC_SRC_BUF_NUM];
    FrameBuffer                 fb;
    FrameBuffer                 fbOffsetTbl;
    Int32                       srcFbIndex;
    Uint32                      inSize;
    Uint32                      outSize;
    Uint32                      headerSize;
    BOOL                        prevMapReuse;
    BOOL                        srcCanBeWritten;
    BOOL                        first;
    BOOL                        last;
    BOOL                        streamBufFull;
} EncoderContext_t;

typedef enum {
    DEC_STATE_NONE,
    DEC_STATE_OPEN_DECODER,
    DEC_STATE_INIT_SEQ,
    DEC_STATE_REGISTER_FB,
    DEC_STATE_DECODING,
    DEC_STATE_CLOSE,
} DecoderState;

typedef struct tivpu_dec_config_t
{
    CodStd              bitFormat;
    Uint32              width;
    Uint32              height;
    Uint32              framerate;
    Uint32              coreIdx;
    Uint32              numOutBufs;
    Uint32              numDecBufs;
    FrameBufferFormat   format;
    Int32               cbcrInterleave;
    Uint8               error_conceal;
} tivpu_dec_config_t;

typedef struct ParamDecBitstreamBuffer {
    Uint32          num;
    vpu_buffer_t*   bs;
} ParamDecBitstreamBuffer;

typedef struct ParamDecNeedFrameBufferNum {
    Uint32  linearNum;                       /*!<< the number of framebuffers which are used to decompress or converter to linear data */
    Uint32  nonLinearNum;                   /*!<< the number of tiled or compressed framebuffers which are used as a reconstruction */
} ParamDecNeedFrameBufferNum;

typedef struct ParamDecFrameBuffer {
    Uint32          stride;
    Uint32          linearNum;               /*!<< the number of framebuffers which are used to decompress or converter to linear data */
    Uint32          nonLinearNum;           /*!<< the number of tiled or compressed framebuffers which are used as a reconstruction */
    FrameBuffer*    fb;
} ParamDecFrameBuffer;

typedef struct ParamDecReallocFB {
    Int32           linearIdx;
    Int32           compressedIdx;
    Int32           indexInterFrameDecoded; /*!<< In case of VP9 codec, index of the frame buffer to reallocate */
    Uint32          width;                  /*!<< New picture width */
    Uint32          height;                 /*!<< New picture hieght */
    FrameBuffer     newFbs[2];              /*!<< Reallocated framebuffers. newFbs[0] for compressed fb, newFbs[1] for linear fb */
} ParamDecReallocFB;

typedef struct {
    DecHandle           handle;
    DecOpenParam        decOpenParam;
    DecParam            decParam;
    DecOutputInfo       decOutInfo;
    FrameBufferFormat   wtlFormat;
    Uint64              startTimeout;
    Uint64              desStTimeout;
    Uint32              iterationCnt;
    Uint32              enableUserData;
    vpu_buffer_t        vbUserData;
    BOOL                stateDoing;
    DecoderState        state;
    DecInitialInfo      initialInfo;
    Uint32              numDecoded;             /*!<< The number of decoded frames */
    Uint32              numOutput;
    PhysicalAddress     decodedAddr;
    BOOL                doingReset;
    Uint32              cyclePerTick;
    Uint32              chromaIDCFlag;
    VpuAttr             attr;
    struct {
        BOOL    enable;
        Uint32  skipCmd;                        /*!<< a skip command to be restored */
    }                   autoErrorRecovery;
    vpu_buffer_t*       bsBuffer;
    Uint32              numBuffers;
    Uint32              bsSize;
    Uint32              numOutBuffers;
    Uint32              numDecBuffers;
    uintptr_t           rdPtr;
    Uint32              triggerSwap;
    Int32               idxDisplayFrame;
    Int32               idxDecodedFrame;
    PhysicalAddress     nextWrPtr;
    BOOL                first;
    BOOL                reuse;
    BOOL                consumed;
    BOOL                last;
    BOOL                streamEndFlag;
    BOOL                terminate;
    Uint32              size;
    Uint32              loopCount;
    osal_thread_t       threadHandle;
    Uint32              framebufStride;
    Uint32              displayPeriodTime;
    FrameBuffer         pFrame[MAX_REG_FRAME];
    vpu_buffer_t        pFbMem[MAX_REG_FRAME];
    vpu_buffer_t*       pFbWtlMem;
    BOOL                enablePPU;
    FrameBuffer         pPPUFrame[MAX_REG_FRAME];
    vpu_buffer_t        pPPUFbMem[MAX_REG_FRAME];
    BOOL                fbAllocated;
    ParamDecNeedFrameBufferNum  fbCount;
    osal_mutex_t        lock;
    Int32               delayDisplay;
    Int32               dispFrameToClearNext;
} DecoderContext_t;


void replace_character(char* str,
    char  c,
    char  r);

extern Uint32 randomSeed;

/* yuv & md5 */
#define NO_COMPARE         0
#define YUV_COMPARE        1
#define MD5_COMPARE        2
#define STREAM_COMPARE     3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Performance report */
typedef void*   PFCtx;

PFCtx PFMonitorSetup(
    Uint32  coreIndex,
    Uint32  instanceIndex,
    Uint32  referenceClkInMHz,
    Uint32  fps,
    char*   strLogDir,
    BOOL    isEnc
    );

void PFMonitorRelease(
    PFCtx   context
    );

void PFMonitorUpdate(
    Uint32  coreIndex,
    PFCtx   context,
    Uint32  cycles,
    ...
    );

void PrepareDecoderTest(
    DecHandle decHandle
    );

void byte_swap(
    unsigned char* data,
    int len
    );

void word_swap(
    unsigned char* data,
    int len
    );

void dword_swap(
    unsigned char* data,
    int len
    );

void lword_swap(
    unsigned char* data,
    int len
    );


typedef enum {
    COMPONENT_PARAM_FAILURE,
    COMPONENT_PARAM_SUCCESS,
    COMPONENT_PARAM_NOT_READY,
    COMPONENT_PARAM_NOT_FOUND,
    COMPONENT_PARAM_TERMINATED,
    COMPONENT_PARAM_MAX
} ComponentParamRet;

/* \brief   Parameter Return Test: returns TRUE to go to the next step, else returns FALSE
*/
BOOL ParamReturnTest(ComponentParamRet ret, BOOL* success);

Int32 LoadFirmware(
    Int32       productId,
    Uint8**   retFirmware,
    Uint32*   retSizeInWord,
    const char* path
    );

void PrintDecSeqWarningMessages(
    Uint32          productId,
    DecInitialInfo* seqInfo
    );

void DisplayDecodedInformation(
    DecHandle      handle,
    CodStd         codec,
    Uint32         frameNo,
    DecOutputInfo* decodedInfo,
    ...
    );

void
DisplayEncodedInformation(
    EncHandle      handle,
    CodStd         codec,
    Uint32         frameNo,
    EncOutputInfo* encodedInfo,
    ...
    );

void PrintEncSppStatus(
    Uint32 coreIdx,
    Uint32 productId
    );
/*
 * VPU Helper functions
 */
/************************************************************************/
/* Video                                                                */
/************************************************************************/

#define PUT_BYTE(_p, _b) \
    *_p++ = (unsigned char)_b;

#define PUT_BUFFER(_p, _buf, _len) \
    osal_memcpy(_p, _buf, _len); \
    (_p) += (_len);

#define PUT_LE32(_p, _var) \
    *_p++ = (unsigned char)((_var)>>0);  \
    *_p++ = (unsigned char)((_var)>>8);  \
    *_p++ = (unsigned char)((_var)>>16); \
    *_p++ = (unsigned char)((_var)>>24);

#define PUT_BE32(_p, _var) \
    *_p++ = (unsigned char)((_var)>>24);  \
    *_p++ = (unsigned char)((_var)>>16);  \
    *_p++ = (unsigned char)((_var)>>8); \
    *_p++ = (unsigned char)((_var)>>0);

#define PUT_LE16(_p, _var) \
    *_p++ = (unsigned char)((_var)>>0);  \
    *_p++ = (unsigned char)((_var)>>8);

#define PUT_BE16(_p, _var) \
    *_p++ = (unsigned char)((_var)>>8);  \
    *_p++ = (unsigned char)((_var)>>0);

Int32 ConvFOURCCToMp4Class(
    Int32   fourcc
    );

Int32 ConvFOURCCToCodStd(
    Uint32 fourcc
    );

Int32 ConvCodecIdToMp4Class(
    Uint32 codecId
    );

Int32 ConvCodecIdToCodStd(
    Int32   codecId
    );

Int32 ConvCodecIdToFourcc(
    Int32   codecId
    );


/************************************************************************/
/* ETC                                                                  */
/************************************************************************/
Uint32 GetRandom(
    Uint32 start,
    Uint32 end
    );

/************************************************************************/
/* MD5                                                                  */
/************************************************************************/

typedef struct MD5state_st {
    Uint32 A,B,C,D;
    Uint32 Nl,Nh;
    Uint32 data[16];
    Uint32 num;
} MD5_CTX;

Int32 MD5_Init(
    MD5_CTX *c
    );

Int32 MD5_Update(
    MD5_CTX*    c,
    const void* data,
    size_t      len);

Int32 MD5_Final(
    Uint8*      md,
    MD5_CTX*    c
    );

Uint8* MD5(
    const Uint8*  d,
    size_t        n,
    Uint8*        md
    );

void plane_md5(MD5_CTX *md5_ctx,
    Uint8  *src,
    int    src_x,
    int    src_y,
    int    out_x,
    int    out_y,
    int    stride,
    int    bpp,
    Uint16 zero
);

/************************************************************************/
/* Comparator                                                           */
/************************************************************************/
#define COMPARATOR_SKIP 0xF0F0F0F0
typedef enum {
    COMPARATOR_CONF_SET_GOLDEN_DATA_SIZE,
    COMPARATOR_CONF_SKIP_GOLDEN_DATA,       /*!<< 2nd parameter pointer of Queue
                                                  containing skip command */
    COMPARATOR_CONF_SET_PICINFO,            //!<< This command is followed by YUVInfo structure.
    COMPARATOR_CONF_SET_MONOCHROME,     //!<< It means a monochrome picture
    COMPARATOR_CONF_SET_NOT_MONOCHROME,     //!<< It means a normal picture
} ComparatorConfType;

typedef void*   Comparator;
typedef struct ComparatorImpl {
    void*       context;
    char*       filename;
    Uint32      curIndex;
    Uint32      numOfFrames;
    BOOL        (*Create)(struct ComparatorImpl* impl, char* path);
    BOOL        (*Destroy)(struct ComparatorImpl* impl);
    BOOL        (*Compare)(struct ComparatorImpl* impl, void* data, PhysicalAddress size, Uint32 insNum);
    BOOL        (*Configure)(struct ComparatorImpl* impl, ComparatorConfType type, void* val);
    BOOL        (*Rewind)(struct ComparatorImpl* impl);
    BOOL        eof;
    BOOL        enableScanMode;
    BOOL        usePrevDataOneTime;
} ComparatorImpl;

typedef struct {
    Uint32          totalFrames;
    ComparatorImpl* impl;
} AbstractComparator;

// YUV Comparator
typedef struct {
    Uint32            width;
    Uint32            height;
    FrameBufferFormat   format;
    BOOL                cbcrInterleave;
    BOOL                isVp9;
} PictureInfo;

Comparator Comparator_Create(
    Uint32    type,               //!<<   1: yuv
    char* goldenPath,
    ...
    );

BOOL Comparator_Destroy(
    Comparator  comp
    );

BOOL Comparator_Act(
    Comparator  comp,
    void*       data,
    Uint32      size,
    Uint32      insNum
    );

BOOL Comparator_CheckFrameCount(
    Comparator  comp
    );

BOOL Comparator_SetScanMode(
    Comparator  comp,
    BOOL        enable
    );

BOOL Comparator_Rewind(
    Comparator  comp
    );

BOOL Comparator_CheckEOF(
    Comparator  comp
    );

Uint32 Comparator_GetFrameCount(
    Comparator comp
    );

BOOL Comparator_Configure(
    Comparator              comp,
    ComparatorConfType      cmd,
    void*                   val
    );

BOOL IsEndOfFile(
    FILE* fp
    );

/************************************************************************/
/* Bitstream Feeder                                                     */
/************************************************************************/
typedef enum {
    FEEDING_METHOD_FIXED_SIZE,
    FEEDING_METHOD_FRAME_SIZE,
    FEEDING_METHOD_SIZE_PLUS_ES,
    FEEDING_METHOD_MAX
} FeedingMethod;

typedef struct {
    void*       data;
    Uint32    size;
    BOOL        eos;        //!<< End of stream
    int seqHeaderSize;
} BSChunk;

typedef void* BSFeeder;

typedef void (*BSFeederHook)(BSFeeder feeder, void* data, Uint32 size, void* arg);

/**
 * \brief           BitstreamFeeder consumes bitstream and updates information of bitstream buffer of VPU.
 * \param handle    handle of decoder
 * \param path      bitstream path
 * \param method    feeding method. see FeedingMethod.
 * \param loopCount If @loopCount is greater than 1 then BistreamFeeder reads the start of bitstream again
 *                  when it encounters the end of stream @loopCount times.
 * \param ...       FEEDING_METHOD_FIXED_SIZE:
 *                      This value of parameter is size of chunk at a time.
 *                      If the size of chunk is equal to zero than the BitstreamFeeder reads bistream in random size.(1Byte ~ 4MB)
 * \return          It returns the pointer of handle containing the context of the BitstreamFeeder.
 */
void* BitstreamFeeder_Create(
    Uint32          coreIdx,
    const char*     path,
    CodStd          codecId,
    FeedingMethod   method,
    EndianMode      endian
    );

/**
 * \brief           This is helper function set to simplify the flow that update bit-stream
 *                  to the VPU.
 */
Uint32 BitstreamFeeder_Act(
    BSFeeder        feeder,
    vpu_buffer_t*   bsBuffer,
    PhysicalAddress wrPtr,
    Uint32          room,
    PhysicalAddress* newWrPtr
    );

BOOL BitstreamFeeder_SetFeedingSize(
    BSFeeder    feeder,
    Uint32      size
    );
/**
 * \brief           Set filling bitstream as ringbuffer mode or linebuffer mode.
 * \param   mode    0 : auto
 *                  1 : ringbuffer
 *                  2 : linebuffer.
 */
#define BSF_FILLING_AUTO                    0
#define BSF_FILLING_RINGBUFFER              1
#define BSF_FILLING_LINEBUFFER              2
/* BSF_FILLING_RINBGUFFER_WITH_ENDFLAG:
 * Scenario:
 * - Application writes 1 ~ 10 frames into bitstream buffer.
 * - Set stream end flag by using VPU_DecUpdateBitstreamBuffer(handle, 0).
 * - Application clears stream end flag by using VPU_DecUpdateBitstreamBuffer(handle, -1).
 *   when indexFrameDisplay is equal to -1.
 * NOTE:
 * - Last frame cannot be a complete frame.
 */
#define BSF_FILLING_RINGBUFFER_WITH_ENDFLAG 3
void BitstreamFeeder_SetFillMode(
    BSFeeder    feeder,
    Uint32      mode
    );

BOOL BitstreamFeeder_IsEos(
    BSFeeder    feeder
    );


Uint32 BitstreamFeeder_GetSeqHeaderSize(
    BSFeeder    feeder
    );


BOOL BitstreamFeeder_Destroy(
    BSFeeder    feeder
    );

BOOL BitstreamFeeder_Rewind(
    BSFeeder feeder
    );

BOOL BitstreamFeeder_SetHook(
    BSFeeder        feeder,
    BSFeederHook    hookFunc,
    void*           arg
    );

/************************************************************************/
/* YUV Feeder                                                           */
/************************************************************************/
#define SOURCE_YUV                  0
#define SOURCE_YUV_WITH_LOADER      2

typedef struct {
    Uint32   cbcrInterleave;
    Uint32   nv21;
    Uint32   i422;
    Uint32   packedFormat;
    Uint32   srcFormat;
    Uint32   srcPlanar;
    Uint32   srcStride;
    Uint32   srcHeight;
} YuvInfo;

BOOL yuvFeeder_Feed(
    EncoderContext_t*   ctx,
    Int32               coreIdx,
    vpu_buffer_t*       inbuf,
    FrameBuffer*        fb,
    size_t              picWidth,
    size_t              picHeight,
    Uint32              srcFbIndex,
    ENC_subFrameSyncCfg *subFrameSyncConfig
    );


/************************************************************************/
/* Video helper                                                         */
/************************************************************************/
/**
 *  \param  convertCbcrIntl     If this value is TRUE, it stores YUV as NV12 or NV21 to @fb
 */

typedef enum {
    SRC_0LINE_WRITE           = 0,
    SRC_64LINE_WRITE          = 64,
    SRC_128LINE_WRITE         = 128,
    SRC_192LINE_WRITE         = 192,
    //...
    REMAIN_SRC_DATA_WRITE     = 0x80000000
} SOURCE_LINE_WRITE;


/************************************************************************/
/* Simple Renderer                                                      */
/************************************************************************/
typedef void*       Renderer;

typedef enum {
    RENDER_DEVICE_NULL,
    RENDER_DEVICE_FBDEV,
    RENDER_DEVICE_HDMI,
    RENDER_DEVICE_MAX
} RenderDeviceType;

typedef struct RenderDevice {
    void*       context;
    DecHandle   decHandle;
    BOOL (*Open)(struct RenderDevice* device);
    void (*Render)(struct RenderDevice* device, DecOutputInfo* fbInfo, Uint8* yuv, Uint32 width, Uint32 height);
    BOOL (*Close)(struct RenderDevice* device);
} RenderDevice;

Renderer SimpleRenderer_Create(
    DecHandle           decHandle,
    RenderDeviceType    deviceType,
    const char*         yuvPath            //!<< path to store yuv iamge.
    );

Uint32 SimpleRenderer_Act(
    Renderer        renderer,
    DecOutputInfo*  fbInfo,
    Uint8*          pYuv,
    Uint32        width,
    Uint32        height
    );

void* SimpleRenderer_GetFreeFrameInfo(
    Renderer        renderer
    );

/* \brief       Flush display queues and clear display indexes
 */
void SimpleRenderer_Flush(
    Renderer        renderer
    );

BOOL SimpleRenderer_Destroy(
    Renderer    renderer
    );

BOOL SimpleRenderer_SetFrameRate(
    Renderer        renderer,
    Uint32          fps
    );


BOOL MkDir(
    char* path
    );

/*******************************************************************************
 * DATATYPES AND FUNCTIONS RELATED TO REPORT
 *******************************************************************************/
typedef struct VpuReportConfig_t {
    PhysicalAddress userDataBufAddr;
    BOOL            userDataEnable;
    Int32           userDataBufSize;

} VpuReportConfig_t;

void OpenDecReport(
    DecHandle           handle,
    VpuReportConfig_t*  cfg
    );

void CloseDecReport(
    DecHandle handle
    );

void ConfigDecReport(
    Uint32      core_idx,
    DecHandle   handle,
    CodStd      bitstreamFormat
    );

void SaveDecReport(
    Uint32          core_idx,
    DecHandle       handle,
    DecOutputInfo*  pDecInfo,
    CodStd          bitstreamFormat,
    Uint32          mbNumX,
    Uint32          mbNumY
    );

Int32 CalculateAuxBufferSize(
    AUX_BUF_TYPE    type,
    CodStd          codStd,
    Int32           width,
    Int32           height
    );

RetCode GetFBCOffsetTableSize(
    CodStd  codStd,
    int     width,
    int     height,
    int*    ysize,
    int*    csize
    );

#define MAX_ROI_LEVEL           (8)
#define LOG2_CTB_SIZE           (5)
#define CTB_SIZE                (1<<LOG2_CTB_SIZE)
#define LAMBDA_SCALE_FACTOR     (100000)
#define FLOATING_POINT_LAMBDA   (1)
#define TEMP_SCALABLE_RC        (1)
#define UI16_MAX                (0xFFFF)
#ifndef INT_MAX
#define INT_MAX                 (2147483647)
#endif

typedef struct {
    char  *name;
    int    min;
    int    max;
    int    def;
} WaveCfgInfo;

void PrintVpuVersionInfo(
    Uint32 coreIdx
    );

void ChangePathStyle(
    char *str
    );

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
    Int32   *yuv3p4b
    );


FrameBufferFormat GetPackedFormat (
    int srcBitDepth,
    int packedType,
    int p10bits,
    int msb
    );

char* GetDirname(
    const char* path
    );

char* GetBasename(
    const char* pathname
    );

char* GetFileExtension(
    const char* filename
    );

int parseWaveEncCfgFile(
    ENC_CFG*    pEncCfg,
    char*       FileName,
    int bitFormat
    );

int parseWaveChangeParamCfgFile(
    ENC_CFG*    pEncCfg,
    char*       FileName
    );

#if 0
int parseRoiCtuModeParam(
    char* lineStr,
    VpuRect* roiRegion,
    int* roiLevel,
    int picX,
    int picY
    );
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

/************************************************************************/
/* Structure                                                            */
/************************************************************************/
typedef struct TestDecConfig_struct {
    char                outputPath[MAX_FILE_PATH];
    char                inputPath[MAX_FILE_PATH];
    Int32               forceOutNum;
    CodStd              bitFormat;
    Int32               reorder;
    TiledMapType        mapType;
    BitStreamMode       bitstreamMode;
    BOOL                enableWTL;
    FrameFlag           wtlMode;
    FrameBufferFormat   wtlFormat;
    Int32               coreIdx;
    ProductId           productId;
    BOOL                enableCrop;                 //!<< option for saving yuv
    BOOL                cbcrInterleave;             //!<< 0: None, 1: NV12, 2: NV21
    BOOL                nv21;                       //!<< FALSE: NV12, TRUE: NV21,
                                                    //!<< This variable is valid when cbcrInterleave is TRUE
    EndianMode          streamEndian;
    EndianMode          frameEndian;
    Uint32              secondaryAXI;
    Int32               compareType;
    char                md5Path[MAX_FILE_PATH];
    char                fwPath[MAX_FILE_PATH];
    char                refYuvPath[MAX_FILE_PATH];
    RenderDeviceType    renderType;
    BOOL                thumbnailMode;
    Int32               skipMode;
    size_t              bsSize;
    BOOL                streamEndFlag;
    struct {
        Uint32      numVCores;                      //!<< This numVCores is valid on PRODUCT_ID_4102 multi-core version
        BOOL        craAsBla;
        Uint32      av1Format;
    } wave;
    Uint32          pfClock;                        //!<< performance clock in Hz
    BOOL            performance;
    Uint32          bandwidth;
    Uint32          fps;
    Uint32          enableUserData;
    /* FEEDER */
    FeedingMethod       feedingMode;
    Uint32              feedingSize;
    Uint32              loopCount;
    BOOL                errorInject;
    BOOL                ignoreHangup;
    BOOL                nonRefFbcWrite;         //!<< If it is TRUE, FBC data of non-reference picture are written into framebuffer. */
    ErrorConcealMode              errConcealMode;
    ErrorConcealUnit              errConcealUnit;
#ifdef SUPPORT_MULTI_INSTANCE_TEST
    Uint32 numMulti;
#endif
} TestDecConfig;

typedef struct {
    DecOutputInfo*  pOutInfo;
    BOOL            scaleX;
    BOOL            scaleY;
} RendererOutInfo;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void SetDefaultDecTestConfig(
    TestDecConfig* testConfig
    );

struct option* ConvertOptions(
    struct OptionExt*   cnmOpt,
    Uint32              nItems
    );

void ReleaseVideoMemory(
    DecHandle   handle,
    vpu_buffer_t*   memoryArr,
    Uint32        count
    );

BOOL AllocateDecFrameBuffer(
    DecoderContext_t* ctx,
    Uint32            nonLinearFbCount,
    Uint32            linearFbCount,
    FrameBuffer*      retFbArray,
    vpu_buffer_t*     retFbAddrs,
    vpu_buffer_t*     retFbWtlAddrs,
    Uint32*           retStride
    );

BOOL AllocFBMemory(
    Uint32 coreIdx,
    vpu_buffer_t *pFbMem,
    FrameBuffer *pFb,
    Uint32 memSize,
    Uint32 memNum,
    Int32 memTypes,
    Int32 instIndex
    );

BOOL SetFbDetails(
    Uint32 coreIdx,
    vpu_buffer_t *inbuf,
    FrameBuffer *pFb,
    Uint32 memSize,
    Uint32 memNum
    );

RetCode SetUpDecoderOpenParam(
    DecOpenParam*        param,
    tivpu_dec_config_t*  config
    );

#define OUTPUT_FP_NUMBER 4
BOOL OpenDisplayBufferFile(
    CodStd  codec,
    char *outputPath,
    VpuRect rcDisplay,
    TiledMapType mapType,
    FILE *fp[]
    );

void CloseDisplayBufferFile(
    FILE *fp[]
    );

void ProcessVc1MultiResolution(
    Uint8* image,
    Int32 width,
    Int32 height,
    BOOL horz_half,
    BOOL vert_half
    );

void GetUserData(
    Int32 coreIdx,
    Uint8* pBase,
    vpu_buffer_t vbUserData,
    DecOutputInfo outputInfo
    );



#ifdef __cplusplus
}
#endif /* __cplusplus */


typedef struct TestEncConfig_struct {
    char    yuvSourceBaseDir[MAX_FILE_PATH];
    char    yuvFileName[MAX_FILE_PATH];
    char    cmdFileName[MAX_FILE_PATH];
    char    bitstreamFileName[MAX_FILE_PATH];
    char    huffFileName[MAX_FILE_PATH];
    char    cInfoFileName[MAX_FILE_PATH];
    char    qMatFileName[MAX_FILE_PATH];
    char    qpFileName[MAX_FILE_PATH];
    char    cfgFileName[MAX_FILE_PATH];
    CodStd  stdMode;
    int     picWidth;
    int     picHeight;
    int     kbps;
    int     rotAngle;
    int     mirDir;
    int     useRot;
    int     qpReport;
    int     ringBufferEnable;
    int     rcIntraQp;
    int     outNum;
    int     skipPicNums[MAX_PIC_SKIP_NUM];
    Uint32     coreIdx;
    TiledMapType mapType;
    // 2D cache option

    int lineBufIntEn;
    int subFrameSyncEn;
    int subFrameSyncMode;
    int en_container;                   //enable container
    int container_frame_rate;           //framerate for container
    int picQpY;

    int cbcrInterleave;
    int nv21;
    int i422;
    BOOL needSourceConvert;         //!<< If the format of YUV file is YUV planar mode and EncOpenParam::cbcrInterleave or EncOpenParam::nv21 is true
                                    //!<< the value of needSourceConvert should be true.
    int packedFormat;
    FrameBufferFormat srcFormat;
    int secondaryAXI;
    EndianMode stream_endian;
    int frame_endian;
    int source_endian;

    ProductId productId;

    int compareType;
#define YUV_MODE_YUV 0
#define YUV_MODE_YUV_LOADER 2
#define YUV_MODE_CFBC       3
    int yuv_mode;
    char ref_stream_path[MAX_FILE_PATH];
    int loopCount;
    char ref_recon_md5_path[MAX_FILE_PATH];
    BOOL    nonRefFbcWrite;
    BOOL    performance;
    Uint32  bandwidth;
    Uint32  fps;
    Uint32  pfClock;
    char roi_file_name[MAX_FILE_PATH];
    FILE *roi_file;
    int roi_enable;

    int encAUD;
    int encEOS;
    int encEOB;
    int useAsLongtermPeriod;
    int refLongtermPeriod;

    // newly added for encoder
    FILE*  scaling_list_file;
    char   scaling_list_fileName[MAX_FILE_PATH];

    FILE*  custom_lambda_file;
    char   custom_lambda_fileName[MAX_FILE_PATH];
    Uint32 roi_avg_qp;

    FILE*  lambda_map_file;
    Uint32 lambda_map_enable;
    char   lambda_map_fileName[MAX_FILE_PATH];

    FILE*  mode_map_file;
    Uint32 mode_map_flag;
    char   mode_map_fileName[MAX_FILE_PATH];

    FILE*  wp_param_file;
    Uint32 wp_param_flag;
    char   wp_param_fileName[MAX_FILE_PATH];

    Int32  force_picskip_start;
    Int32  force_picskip_end;
    Int32  force_coefdrop_start;
    Int32  force_coefdrop_end;
    Int32  numChangeParam;
    W5ChangeParam changeParam[10];

    int    forceIdrPicIdx;

    char optYuvPath[MAX_FILE_PATH];
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
    int srcReleaseIntEnable;
#endif
    int ringBufferWrapEnable;

    HevcSEIDataEnc seiDataEnc;
    char hrd_rbsp_file_name[MAX_FILE_PATH];
    FILE *hrd_rbsp_fp;
    char vui_rbsp_file_name[MAX_FILE_PATH];
    FILE *vui_rbsp_fp;
    char prefix_sei_nal_file_name[MAX_FILE_PATH];
    FILE *prefix_sei_nal_fp;
    char suffix_sei_nal_file_name[MAX_FILE_PATH];
    FILE *suffix_sei_nal_fp;
#ifdef SUPPORT_MULTI_INSTANCE_TEST
    Uint32 numMulti;
#endif
} TestEncConfig;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
BOOL SetupEncoderOpenParam(
    EncOpenParam*        param,
    tivpu_enc_config_t*  config
    );

Int32 GetEncOpenParamDefault(
    EncOpenParam*        pEncOP,
    tivpu_enc_config_t*  pEncConfig
    );

void GenRegionToMap(
    VpuRect *region,        /**< The size of the ROI region for H.265 (start X/Y in CTU, end X/Y in CTU)  */
    int *roiLevel,
    int num,
    Uint32 mapWidth,
    Uint32 mapHeight,
    Uint8 *roiCtuMap
    );

#define VUI_HRD_RBSP_BUF_SIZE           0x4000
#define SEI_NAL_DATA_BUF_SIZE           0x4000
Int32 writeVuiRbsp(
    int coreIdx,
    TestEncConfig *encConfig,
    EncOpenParam *encOP,
    vpu_buffer_t *vbVuiRbsp
    );
Int32 writeHrdRbsp(
    int coreIdx,
    TestEncConfig *encConfig,
    EncOpenParam *encOP,
    vpu_buffer_t *vbHrdRbsp
    );

void setEncBgMode(
    EncParam *encParam,
    TestEncConfig encConfig
    );

void GenRegionToQpMap(
    VpuRect *region,        /**< The size of the ROI region for H.265 (start X/Y in CTU, end X/Y int CTU)  */
    int *roiLevel,
    int num,
    int initQp,
    Uint32 mapWidth,
    Uint32 mapHeight,
    Uint8 *roiCtuMap
    );

void CheckParamRestriction(
    Uint32 productId,
    TestEncConfig *encConfig
    );
int openRoiMapFile(
    TestEncConfig *encConfig
    );
int allocateRoiMapBuf(
    EncHandle handle,
    TestEncConfig encConfig,
    vpu_buffer_t *vbROi,
    int srcFbNum,
    int ctuNum
    );

#if 0
RetCode SetChangeParam(
    EncHandle handle,
    TestEncConfig encConfig,
    EncOpenParam encOP,
    Int32 changedCount
    );
#endif

BOOL GetBitstreamToBuffer(
    EncHandle handle,
    Uint8* pBuffer,
    PhysicalAddress rdAddr,
    PhysicalAddress wrAddr,
    PhysicalAddress streamBufStartAddr,
    PhysicalAddress streamBufEndAddr,
    Uint32 streamSize,
    EndianMode endian,
    BOOL enabledRinbuffer
    );


void SetDefaultEncTestConfig(
    TestEncConfig* testConfig
    );

typedef enum {
    GET_PARAM_COM_STATE,                    /*!<< It returns state of component. Param: ComponentState* */
    GET_PARAM_COM_IS_CONTAINER_CONUSUMED,   /*!<< pointer of PortContainer */
    GET_PARAM_FEEDER_BITSTREAM_BUF,         /*!<< to a feeder component  : ParamDecBitstreamBuffer */
    GET_PARAM_FEEDER_EOS,                   /*!<< to a feeder component  : BOOL */
    GET_PARAM_VPU_STATUS,                   /*!<< to a component. Get status information of the VPU : ParamVpuStatus. */
    GET_PARAM_DEC_HANDLE,
    GET_PARAM_DEC_CODEC_INFO,               /*!<< It returns a codec information. Param: DecInitialInfo */
    GET_PARAM_DEC_BITSTREAM_BUF_POS,        /*!<< to a decoder component in ring-buffer mode. */
    GET_PARAM_DEC_FRAME_BUF_NUM,            /*!<< to a decoder component : ParamDecNeedFrameBufferNum*/
    GET_PARAM_RENDERER_FRAME_BUF,           /*!<< to a renderer component. ParamDecFrameBuffer */
    GET_PARAM_RENDERER_PPU_FRAME_BUF,       /*!<< to a renderer component. ParamDecPPUFrameBuffer */
    GET_PARAM_SRC_FRAME_INFO,
    GET_PARAM_ENC_HANDLE,
    GET_PARAM_ENC_FRAME_BUF_NUM,
    GET_PARAM_ENC_FRAME_BUF_REGISTERED,
    GET_PARAM_YUVFEEDER_FRAME_BUF,
    GET_PARAM_READER_BITSTREAM_BUF,
    GET_PARAM_MAX
} GetParameterCMD;

/* tivpucodec specific. */
typedef struct tivpu_context_s {
    int32_t				ch_id;
    BOOL				in_use;
    void                *codec_ctx;
    BOOL                is_dec;
    vpu_buffer_t        *input_bufs;
    vpu_buffer_t        *output_bufs;
    int                 input_buf_num;
    int                 output_buf_num;
    pid_t               pid;
    int32_t             core_idx;
}tivpu_context_t;

typedef struct enc_status_s {
	 uint8_t 	is_first;
	 uint32_t	hdr_size;
	 uint32_t	out_size;
}vpu_enc_status_t;

typedef struct dec_status_s {
	 uint8_t 	is_first; // not sure if these are required for dec
	 uint32_t	hdr_size; // not sure if these are required for dec
	 uint32_t	out_size;
     uintptr_t  vpu_return_buf;
     uint32_t   o_filled_len;
     int32_t    displayed_frames;
     int32_t    decoded_frames;
     BOOL       terminate;
}vpu_dec_status_t;


/******************************* DEBUG INFO ****************************/
typedef struct codec_dbg_info {
    pid_t pid;
    uint32_t cnt;
}codec_dbg_info_t;

int32_t dump_buf_to_file(const char * dir, void *buf, size_t buf_len, codec_dbg_info_t *info);
void print_buf_info(const char *message, vpu_buffer_t *buf);
/******************************* DEBUG INFO ****************************/



#if 0
typedef struct codec_handle
{
    void                  *tivpu_hdl;
    vpu_buffer_t          *input_bufs;
    vpu_buffer_t          *output_bufs;
    int                   input_buf_num;
    int                   output_buf_num;
    omxil_encode_callback en_callback;
    omxil_decode_callback dec_callback;
    uint32_t              output_buf_cnt;   //count of buffers send to codec
    uint32_t              output_frame_cnt; //count of frames returned from codec.
    bool                  eos;
    void*                 cb_ctx;
} codec_t;
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* _MAIN_HELPER_H_ */
