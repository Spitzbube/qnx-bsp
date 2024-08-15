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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//QNX system headers
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <pthread.h>

//OMX IL headers
#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Index.h>
#include <OMX_Image.h>
#include <OMX_Audio.h>
#include <OMX_Video.h>
#include <OMX_IVCommon.h>
#include <OMX_Other.h>
#include <OMX_Extension_video_TI.h>
#include <OMX_Extension_index_TI.h>

#include "log.h"
#include "core.h"
#include "list.h"
#include "omxil_enc_interface.h"
#include "omxil_queue.h"
#include "omxil_component_util.h"
#include "ti/shmemallocator/SharedMemoryAllocatorUsr.h"

/*
*  D E C L A R A T I O N S
*/
#define OMX_NOPORT 0xFFFFFFFE
#define NUM_IN_BUFFERS 3        // Input Buffers
#define MIN_NUM_IN_BUFFERS 3        // Minimum Input Buffers
#define NUM_OUT_BUFFERS 2       // Output Buffers
#define MIN_NUM_OUT_BUFFERS 2       // Minimum Output Buffers
#define OMX_TIMEOUT 80000      // Timeout value in microseconds
#define OMX_MAX_TIMEOUTS 4      // Count of Maximum number of times the component can time out
#define OMX_MAX_QUEUE_SIZE 128      // Count of Maximum number of queue items for command and data queue

/*
*     D E F I N I T I O N S
*/
static const char *COMPONENT_NAME = "OMX.qnx.video.encoder";

static void* ComponentThread(void* pThreadData);

/*
 * Enumeration for the commands processed by the component
 */

typedef enum QCompCmdType
{
    CmdNull,
    SetState,
    Flush,
    StopPort,
    RestartPort,
    MarkBuf,
    FillBuf,
    EmptyBuf,
} QCompCmdType;


/*
 * Private data of the component.
 * It includes input and output port related information (Port definition, format),
 *   buffer related information, specifications for the video decoder, the command and data queue
 *   and the BufferList structure for storing input and output buffers.
 */

typedef struct QCOMPDATATYPE_ {
    OMX_STATETYPE state;
    OMX_CALLBACKTYPE *pCallbacks;
    OMX_PTR pAppData;
    OMX_HANDLETYPE hSelf;
    OMX_PORT_PARAM_TYPE sPortParam;
    OMX_PARAM_PORTDEFINITIONTYPE sInPortDef;
    OMX_PARAM_PORTDEFINITIONTYPE sOutPortDef;
    OMX_VIDEO_PARAM_PORTFORMATTYPE sInPortFormat;
    OMX_VIDEO_PARAM_PORTFORMATTYPE sOutPortFormat;
    OMX_PRIORITYMGMTTYPE sPriorityMgmt;
    OMX_PARAM_BUFFERSUPPLIERTYPE sInBufSupplier;
    OMX_PARAM_BUFFERSUPPLIERTYPE sOutBufSupplier;
    OMX_VIDEO_PARAM_MPEG2TYPE sMpeg2;
    OMX_VENDOR_TIVPU_PARAM_TYPE vendorTIVPUParam;
    OMX_MARKTYPE *pMarkBuf;
    pthread_t thread_id;
    bool      thread_running;
    qfifo_t *cmdQueue;
    qfifo_t *dataQueue;
    QCompCmdType eTCmd;
    BufferList sInBufList;
    BufferList sOutBufList;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    void       *codec_hdl;
    int codec_core_idx;
    int setLossless;
    int setGOP;
    OMX_VIDEO_PARAM_AVCTYPE           m_sAvc;
    OMX_VIDEO_CODEC_PARAM_HEVCTYPE    m_sHevc;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE  m_sProfileLevel;
    OMX_VIDEO_PARAM_BITRATETYPE       m_sBitrateControl;
    OMX_VIDEO_PARAM_QUANTIZATIONTYPE  m_sQuantization;
    OMX_VIDEO_CONFIG_BITRATETYPE      m_sBitrate;
    OMX_CONFIG_FRAMERATETYPE          m_sFrameRate;
    OMX_CONFIG_INTRAREFRESHVOPTYPE    m_sIntraRefreshVOP;
    OMX_VIDEO_CONFIG_AVCINTRAPERIOD   m_sAVCIntraPeriod;
    OMX_U32 firstFrame;


    bool	   m_bEncoderConfigured;
    encoder_config m_sConfig;
    bool       m_bInPortParamsValid;
    bool       m_bOutPortParamsValid;

} QCompDataType;

/*
 * Initializes a data structure using a pointer to the structure.
 * The initialization of OMX structures always sets up the nSize and nVersion fields
 *   of the structure.
 */
#define OMX_CONF_INIT_STRUCT_PTR(_s_, _name_) \
    memset((_s_), 0x0, sizeof(_name_)); \
    (_s_)->nSize = sizeof(_name_); \
    (_s_)->nVersion.s.nVersionMajor = 0x1; \
    (_s_)->nVersion.s.nVersionMinor = 0x0; \
    (_s_)->nVersion.s.nRevision = 0x0;  \
    (_s_)->nVersion.s.nStep = 0x0



    /*
     * Checking for version compliance.
     * If the nSize of the OMX structure is not set, raises bad parameter error.
     * In case of version mismatch, raises a version mismatch error.
     */
#define OMX_CONF_CHK_VERSION(_s_, _name_, _e_) \
    if((_s_)->nSize != sizeof(_name_)) _e_ = OMX_ErrorBadParameter; \
    if(((_s_)->nVersion.s.nVersionMajor != 0x1)|| \
            ((_s_)->nVersion.s.nVersionMinor != 0x0)|| \
            ((_s_)->nVersion.s.nRevision != 0x0)|| \
            ((_s_)->nVersion.s.nStep != 0x0)) _e_ = OMX_ErrorVersionMismatch;\
    if(_e_ != OMX_ErrorNone) goto OMX_CONF_CMD_BAIL;



    /*
     * Checking paramaters for non-NULL values.
     * The macro takes three parameters because inside the code the highest
     *   number of parameters passed for checking in a single instance is three.
     * In case one or two parameters are passed, the ramaining parameters
     *   are set to 1 (or a nonzero value).
     */
#define OMX_CONF_CHECK_CMD(_ptr1, _ptr2, _ptr3)	\
    { \
        if(!_ptr1 || !_ptr2 || !_ptr3){ \
            eError = OMX_ErrorBadParameter; \
            goto OMX_CONF_CMD_BAIL; \
        } \
    }



    /*
     * Sets error type and redirects control flow to error handling and cleanup section
     */
#define OMX_CONF_SET_ERROR_BAIL(_eError, _eCode)\
    {\
        _eError = _eCode;\
        goto OMX_CONF_CMD_BAIL; \
    }


/*
 *     F U N C T I O N S
 */
static OMX_ERRORTYPE queueCommand(QCompDataType *pQCompData, QCompCmdType cmd, void *cmdData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    pthread_mutex_lock( &pQCompData->mutex );
    if(QueuePut(pQCompData->cmdQueue, (void*)(intptr_t)cmd) == false) {
        LOG( LOG_ERROR, "OMXCOMP=>%s:%d Error Put cmdQueue failure", __func__, __LINE__);
        pthread_mutex_unlock( &pQCompData->mutex );
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    if(QueuePut(pQCompData->dataQueue, cmdData) == false) {
        LOG( LOG_ERROR, "OMXCOMP=>%s:%d Error Put dataQueue failure", __func__, __LINE__);
        pthread_mutex_unlock( &pQCompData->mutex );
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }
    pthread_cond_signal(&pQCompData->cond);
    pthread_mutex_unlock( &pQCompData->mutex );

 OMX_CONF_CMD_BAIL:
    return eError;
}

/*****************************************************************************/
OMX_ERRORTYPE QCompSendCommand(OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_COMMANDTYPE Cmd,
        OMX_IN  OMX_U32 nParam1,
        OMX_IN  OMX_PTR pCmdData)
{
    QCompDataType *pQCompData;
    QCompCmdType eCmd = CmdNull;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    void *cmdData;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pQCompData, 1, 1);
    if (Cmd == OMX_CommandMarkBuffer)
        OMX_CONF_CHECK_CMD(pCmdData, 1, 1);

    if (pQCompData->state == OMX_StateInvalid)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInvalidState);

    switch (Cmd){
        case OMX_CommandStateSet:
            eCmd = SetState;
            break;
        case OMX_CommandFlush:
            eCmd = Flush;
            if (nParam1 > 1 && nParam1 != -1)
                OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadPortIndex);
            break;
        case OMX_CommandPortDisable:
            eCmd = StopPort;
            break;
        case OMX_CommandPortEnable:
            eCmd = RestartPort;
            break;
        case OMX_CommandMarkBuffer:
            eCmd = MarkBuf;
            if (nParam1 > 0)
                OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadPortIndex);
            break;
        default:
            break;
    }

    // In case of MarkBuf, the pCmdData parameter is used to carry the data.
    // In other cases, the nParam1 parameter carries the data.
    if(eCmd == MarkBuf) {
        cmdData = pCmdData;
    }else{
        cmdData = (void*)(intptr_t)nParam1;
    }

    eError = queueCommand(pQCompData, eCmd, cmdData);

 OMX_CONF_CMD_BAIL:
    return eError;
}



/*****************************************************************************/
OMX_ERRORTYPE QCompGetState(OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STATETYPE* pState)
{
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pQCompData, pState, 1);

    pthread_mutex_lock(&pQCompData->mutex);
    *pState = pQCompData->state;
    pthread_mutex_unlock(&pQCompData->mutex);

OMX_CONF_CMD_BAIL:
    return eError;
}



/*****************************************************************************/
OMX_ERRORTYPE QCompSetCallbacks(OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
        OMX_IN  OMX_PTR pAppData)
{
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pQCompData, pCallbacks, pAppData);

    pthread_mutex_lock(&pQCompData->mutex);
    pQCompData->pCallbacks = pCallbacks;
    pQCompData->pAppData = pAppData;
    pthread_mutex_unlock(&pQCompData->mutex);

OMX_CONF_CMD_BAIL:
    return eError;
}



/*****************************************************************************/
OMX_ERRORTYPE QCompGetParameter(OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR ComponentParameterStructure)
{
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32       index  = nParamIndex;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pQCompData, ComponentParameterStructure, 1);

    if (pQCompData->state == OMX_StateInvalid)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);

    pthread_mutex_lock(&pQCompData->mutex);

    switch (index){ /* Switch on "index" here to use added index values and avoid compile warnings */
        // Gets OMX_PORT_PARAM_TYPE structure
        case OMX_IndexParamVideoInit:
            memcpy(ComponentParameterStructure, &pQCompData->sPortParam, sizeof
                    (OMX_PORT_PARAM_TYPE));
            break;
            // Gets OMX_PARAM_PORTDEFINITIONTYPE structure
        case OMX_IndexParamPortDefinition:
            if (((OMX_PARAM_PORTDEFINITIONTYPE *)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->sInPortDef.nPortIndex)
                memcpy(ComponentParameterStructure, &pQCompData->sInPortDef, sizeof
                        (OMX_PARAM_PORTDEFINITIONTYPE));
            else if (((OMX_PARAM_PORTDEFINITIONTYPE *)
                        (ComponentParameterStructure))->nPortIndex ==
                    pQCompData->sOutPortDef.nPortIndex)
                memcpy(ComponentParameterStructure, &pQCompData->sOutPortDef, sizeof
                        (OMX_PARAM_PORTDEFINITIONTYPE));
            else
                eError = OMX_ErrorBadPortIndex;
            break;
            // Gets OMX_VIDEO_PARAM_PORTFORMATTYPE structure
        case OMX_IndexParamVideoPortFormat:
            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->sInPortFormat.nPortIndex){
                if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)
                            (ComponentParameterStructure))->nIndex >
                        pQCompData->sInPortFormat.nIndex)
                    eError = OMX_ErrorNoMore;
                else
                    memcpy(ComponentParameterStructure, &pQCompData->sInPortFormat, sizeof
                            (OMX_VIDEO_PARAM_PORTFORMATTYPE));
            }
            else if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)
                        (ComponentParameterStructure))->nPortIndex ==
                    pQCompData->sOutPortFormat.nPortIndex){
                if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)
                            (ComponentParameterStructure))->nIndex >
                        pQCompData->sOutPortFormat.nIndex)
                    eError = OMX_ErrorNoMore;
                else
                    memcpy(ComponentParameterStructure, &pQCompData->sOutPortFormat, sizeof
                            (OMX_VIDEO_PARAM_PORTFORMATTYPE));
            }
            else
                eError = OMX_ErrorBadPortIndex;
            break;
            // Gets OMX_PRIORITYMGMTTYPE structure
        case OMX_IndexParamPriorityMgmt:
            memcpy(ComponentParameterStructure, &pQCompData->sPriorityMgmt, sizeof
                    (OMX_PRIORITYMGMTTYPE));
            break;
        case OMX_IndexParamVideoAvc:
            if (((OMX_VIDEO_PARAM_AVCTYPE *)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->m_sAvc.nPortIndex)
                memcpy(ComponentParameterStructure, &pQCompData->m_sAvc, sizeof
                        (OMX_VIDEO_PARAM_AVCTYPE));
            else {
                eError = OMX_ErrorBadPortIndex;
                LOG(LOG_ERROR, "QOmxilComponentEnc::OMX_IndexParamVideoAvc invalid port index.");
            }
            break;
        case OMXQ_IndexParamVideoHevc:
            if (((OMX_VIDEO_CODEC_PARAM_HEVCTYPE *)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->m_sHevc.nPortIndex)
                memcpy(ComponentParameterStructure, &pQCompData->m_sHevc, sizeof
                        (OMX_VIDEO_CODEC_PARAM_HEVCTYPE));
            else {
                eError = OMX_ErrorBadPortIndex;
                LOG(LOG_ERROR, "QOmxilComponentEnc::OMXQ_IndexParamVideoHevc invalid port index.");
            }
            break;
        case OMX_IndexParamVideoBitrate:
            if (((OMX_VIDEO_PARAM_BITRATETYPE*)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->m_sBitrateControl.nPortIndex)
                memcpy(ComponentParameterStructure, &pQCompData->m_sBitrateControl, sizeof
                        (OMX_VIDEO_PARAM_BITRATETYPE));
            else {
                eError = OMX_ErrorBadPortIndex;
                LOG(LOG_ERROR, "QOmxilComponentEnc::OMX_IndexParamVideoAvc invalid port index.");
            }
            break;
        case OMX_IndexParamVideoQuantization:
            if (((OMX_VIDEO_PARAM_QUANTIZATIONTYPE*)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->m_sQuantization.nPortIndex)
                memcpy(ComponentParameterStructure, &pQCompData->m_sQuantization, sizeof
                        (OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
            else {
                eError = OMX_ErrorBadPortIndex;
                LOG(LOG_ERROR, "QOmxilComponentEnc::OMX_IndexParamVideoQuantization invalid port index.");
            }
            break;
        case OMX_IndexParamVideoProfileLevelCurrent:
            if (((OMX_VIDEO_PARAM_PROFILELEVELTYPE*)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->m_sProfileLevel.nPortIndex)
                memcpy(ComponentParameterStructure, &pQCompData->m_sProfileLevel, sizeof
                        (OMX_VIDEO_PARAM_PROFILELEVELTYPE));
            else {
                eError = OMX_ErrorBadPortIndex;
                LOG(LOG_ERROR, "QOmxilComponentEnc::OMX_IndexParamVideoProfileLevelCurrent invalid port index.");
            }
            break;
        case OMX_IndexParamVideoProfileLevelQuerySupported:
            //TODO query supported profilelevel here
            LOG(LOG_INFO, "QOmxilComponentEnc:: OMX_IndexParamVideoProfileLevelQuerySupported not supported yet.");
            eError = OMX_ErrorUnsupportedIndex;
            break;
        default:
            /* Handle Vendor specific indices here . Move to a separate function if 
             * The if checks get unmanagable */
            if(index == (OMX_INDEXTYPE)OMX_VendorTIVPUConfigCoreIndex) {
                memcpy(ComponentParameterStructure, &pQCompData->vendorTIVPUParam, sizeof
                        (OMX_VENDOR_TIVPU_PARAM_TYPE));
            } else
                eError = OMX_ErrorUnsupportedIndex;
            break;
    }
    pthread_mutex_unlock(&pQCompData->mutex);

OMX_CONF_CMD_BAIL:
    return eError;
}

/*****************************************************************************/
static OMX_ERRORTYPE configEncoder(encoder_config *config, QCompDataType* pQCompData)
{
    OMX_ERRORTYPE err;

    memset(config, 0 ,sizeof(*config));
    config->coreIdx = pQCompData->codec_core_idx;
    config->setLossless = pQCompData->setLossless;
    config->setGOP = pQCompData->setGOP;
    config->iFormat = pQCompData->sInPortDef.format.video.eColorFormat;
    config->oFormat = pQCompData->sOutPortDef.format.video.eCompressionFormat;
    config->width = pQCompData->sInPortDef.format.video.nFrameWidth;
    config->height = pQCompData->sInPortDef.format.video.nFrameHeight;
    config->stride = pQCompData->sInPortDef.format.video.nStride;
    if(pQCompData->m_sProfileLevel.eProfile && pQCompData->m_sProfileLevel.eLevel) {
        config->profile = pQCompData->m_sProfileLevel.eProfile;
        config->level = pQCompData->m_sProfileLevel.eLevel;
    }
    else if(config->oFormat == OMX_VIDEO_CodingAVC) {
        config->profile = pQCompData->m_sAvc.eProfile;
        config->level = pQCompData->m_sAvc.eLevel;
    }
    else if(config->oFormat == (OMX_VIDEO_CODINGTYPE)OMXQ_VIDEO_CodingHEVC) {
        config->profile = pQCompData->m_sHevc.eProfile;
        config->level = pQCompData->m_sHevc.eLevel;
    }

    if(pQCompData->m_sFrameRate.xEncodeFramerate > 0)
        config->framerate = pQCompData->m_sFrameRate.xEncodeFramerate >> 16;
    else if(pQCompData->sInPortDef.format.video.xFramerate > 0)
        config->framerate =  pQCompData->sInPortDef.format.video.xFramerate >> 16;
    else if(pQCompData->sOutPortDef.format.video.xFramerate > 0)
        config->framerate =  pQCompData->sOutPortDef.format.video.xFramerate >> 16;

    if((config->oFormat == OMX_VIDEO_CodingAVC) ||
       (config->oFormat == (OMX_VIDEO_CODINGTYPE)OMXQ_VIDEO_CodingHEVC)) {
        config->keyFrameInterval = pQCompData->m_sAVCIntraPeriod.nIDRPeriod;
    }

    if(pQCompData->m_sBitrate.nEncodeBitrate)
        config->bitrate = pQCompData->m_sBitrate.nEncodeBitrate;
    else
        config->bitrate = pQCompData->m_sBitrateControl.nTargetBitrate;

    if(config->bitrate > 0)
        config->rateControl = pQCompData->m_sBitrateControl.eControlRate;
    else
        config->rateControl = OMX_Video_ControlRateDisable;

    config->qpI = pQCompData->m_sQuantization.nQpI;
    config->qpP = pQCompData->m_sQuantization.nQpP;

    LOG(LOG_DEBUG1, "= config->coreIdx = 0x%x", config->coreIdx);
    LOG(LOG_DEBUG1, "= config->iFormat = 0x%x", config->iFormat);
    LOG(LOG_DEBUG1, "= config->oFormat = 0x%x", config->oFormat);
    LOG(LOG_DEBUG1, "= config->width   = %u", config->width);
    LOG(LOG_DEBUG1, "= config->height  = %u", config->height);
    LOG(LOG_DEBUG1, "= config->stride  = %u", config->stride);
    LOG(LOG_DEBUG1, "= config->profile = 0x%x", config->profile);
    LOG(LOG_DEBUG1, "= config->level   = 0x%x", config->level);
    LOG(LOG_DEBUG1, "= config->framerate        = %u", config->framerate);
    LOG(LOG_DEBUG1, "= config->keyFrameInterval = %u", config->keyFrameInterval);
    LOG(LOG_DEBUG1, "= config->bitrate     = %u", config->bitrate);
    LOG(LOG_DEBUG1, "= config->rateControl = 0x%x", config->rateControl);
    LOG(LOG_DEBUG1, "= config->qpI         = 0x%x", config->qpI);
    LOG(LOG_DEBUG1, "= config->qpP         = 0x%x", config->qpP);

    err = omxil_enc_config(pQCompData->codec_hdl, &pQCompData->sInPortDef, &pQCompData->sOutPortDef, config);
    if(err != OMX_ErrorNone) {
        LOG(LOG_ERROR, "Encoder config return Error(0x%x)", err);
        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                OMX_EventError, OMX_ErrorHardware, 0, NULL);
    }
    else {
        OMX_U32 num_bufs;
        OMX_U32 buf_size;

        // JB: Need to revisit this later. The number of output buffer need to be decided by the VPU codec
        num_bufs = NUM_OUT_BUFFERS;

        if((err = omxil_query_buf_info(pQCompData->codec_hdl, &num_bufs, &buf_size)) != OMX_ErrorNone) {
            LOG(LOG_INFO, "%s omxil_query_buf_info failed, set default value", __func__);
            pQCompData->sOutPortDef.nBufferSize = pQCompData->sOutPortDef.format.video.nFrameWidth * pQCompData->sOutPortDef.format.video.nFrameHeight * 3 / 2;
        }
        else {
            LOG(LOG_INFO, "%s omxil_query_buf_info buf_size=%d num_bufs=%d", __func__, buf_size, num_bufs);
            pQCompData->sOutPortDef.nBufferSize = buf_size;
            pQCompData->sOutPortDef.nBufferCountActual = num_bufs > MIN_NUM_OUT_BUFFERS ? num_bufs : MIN_NUM_OUT_BUFFERS;
        }
    }

    return err;
}

/*****************************************************************************/
OMX_ERRORTYPE QCompSetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR ComponentParameterStructure)
{
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32       index  = nIndex;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDefIn, *pPortDef;
    BufferList *pBufList;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pQCompData, ComponentParameterStructure, 1);

    if (pQCompData->state != OMX_StateLoaded)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);

    pthread_mutex_lock(&pQCompData->mutex);
    switch (index){ /* Switch on "index" here to use added index values and avoid compile warnings */
        // Sets OMX_PORT_PARAM_TYPE structure
        case OMX_IndexParamVideoInit:
            if(((OMX_PORT_PARAM_TYPE*)ComponentParameterStructure)->nPorts != pQCompData->sPortParam.nPorts ||
                ((OMX_PORT_PARAM_TYPE*)ComponentParameterStructure)->nStartPortNumber != pQCompData->sPortParam.nStartPortNumber) {
                LOG(LOG_ERROR, "%s:%d Error invalid parameters", __func__, __LINE__);
                eError = OMX_ErrorBadParameter;
            }
            break;
        // Sets OMX_PARAM_PORTDEFINITIONTYPE structure
        case OMX_IndexParamPortDefinition:
            pPortDefIn = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
            if (pPortDefIn->nPortIndex == pQCompData->sInPortDef.nPortIndex) {
                pPortDef = &pQCompData->sInPortDef;
                pBufList = &pQCompData->sInBufList;
                if(omxil_validate_inport_parameters(pQCompData, pPortDefIn) != OMX_ErrorNone) {
                    eError = OMX_ErrorBadParameter;
                    LOG(LOG_ERROR, "%s:%d Invalid input port parameter)", __func__, __LINE__);
                    break;
                }
                else {
                    pQCompData->m_bInPortParamsValid = true;
                }
            }
            else if (pPortDefIn->nPortIndex == pQCompData->sOutPortDef.nPortIndex) {
                pPortDef = &pQCompData->sOutPortDef;
                pBufList = &pQCompData->sOutBufList;
                if(omxil_validate_outport_parameters(pQCompData, pPortDefIn) != OMX_ErrorNone) {
                    eError = OMX_ErrorBadParameter;
                    LOG(LOG_ERROR, "%s:%d Invalid output port parameter)", __func__, __LINE__);
                    break;
                }
                else {
                    pQCompData->m_bOutPortParamsValid = true;
                }
            }
            else {
                eError = OMX_ErrorBadPortIndex;
                LOG(LOG_ERROR, "%s:%d Error bad port index(%d)", __func__, __LINE__, pPortDefIn->nPortIndex);
                break;
            }

            if(pPortDefIn->nBufferCountActual <  pPortDef->nBufferCountMin) {
                LOG(LOG_ERROR, "%s:%d Error invalid parameters", __func__, __LINE__);
                eError = OMX_ErrorBadParameter;
                break;
            }

            LOG(LOG_DEBUG2, "%s:%d alloc memory for Inport buffer headers ", __func__, __LINE__);
            pBufList->pBufHdr = (OMX_BUFFERHEADERTYPE**)calloc(
                    pPortDefIn->nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE*));
            pBufList->pAllocHdr = (OMX_BUFFERHEADERTYPE**)calloc(
                    pPortDefIn->nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE*));
            pBufList->pShmBufs = (void **)calloc(pPortDefIn->nBufferCountActual, sizeof(shm_buf *));
            if (!pBufList->pBufHdr || !pBufList->pAllocHdr || !pBufList->pShmBufs) {
                LOG(LOG_ERROR, "%s:%d Error alloc memory for (%d) buffers",
                        __func__, __LINE__, pPortDefIn->nBufferCountActual);
                eError = OMX_ErrorInsufficientResources;
            }
            else {

                memcpy(pPortDef, ComponentParameterStructure, sizeof
                        (OMX_PARAM_PORTDEFINITIONTYPE));

                if(pQCompData->m_bOutPortParamsValid && pQCompData->m_bInPortParamsValid && !pQCompData->m_bEncoderConfigured) {
                    if((configEncoder(&pQCompData->m_sConfig, pQCompData)) == OMX_ErrorNone) {
                        pQCompData->m_bEncoderConfigured = true;
                    }
                    else {
                        eError = OMX_ErrorUndefined;
                        LOG(LOG_ERROR, "%s:%d Error configEncoder", __func__, __LINE__);
                    }
                }
            }
            break;
            // Sets OMX_VIDEO_PARAM_PORTFORMATTYPE structure
        case OMX_IndexParamVideoPortFormat:
            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->sInPortFormat.nPortIndex){
                if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)(ComponentParameterStructure))->nIndex
                        > pQCompData->sInPortFormat.nIndex)
                    eError = OMX_ErrorNoMore;
                else
                    memcpy(&pQCompData->sInPortFormat, ComponentParameterStructure, sizeof
                            (OMX_VIDEO_PARAM_PORTFORMATTYPE));
            }
            else if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)
                        (ComponentParameterStructure))->nPortIndex ==
                    pQCompData->sOutPortFormat.nPortIndex){
                if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)
                            (ComponentParameterStructure))->nIndex >
                        pQCompData->sOutPortFormat.nIndex)
                    eError = OMX_ErrorNoMore;
                else
                    memcpy(&pQCompData->sOutPortFormat, ComponentParameterStructure, sizeof
                            (OMX_VIDEO_PARAM_PORTFORMATTYPE));
            }
            else
                eError = OMX_ErrorBadPortIndex;
            break;
            // Sets OMX_PRIORITYMGMTTYPE structure
        case OMX_IndexParamPriorityMgmt:
            memcpy(&pQCompData->sPriorityMgmt, ComponentParameterStructure, sizeof
                    (OMX_PRIORITYMGMTTYPE));
            break;
            // Sets OMX_VIDEO_PARAM_AVCTYPE structure
        case OMX_IndexParamVideoAvc:
            if (((OMX_VIDEO_PARAM_AVCTYPE*)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->m_sAvc.nPortIndex)
                memcpy(&pQCompData->m_sAvc, ComponentParameterStructure, sizeof
                        (OMX_VIDEO_PARAM_AVCTYPE));
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        case OMXQ_IndexParamVideoHevc:
            if (((OMX_VIDEO_CODEC_PARAM_HEVCTYPE*)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->m_sHevc.nPortIndex)
                memcpy(&pQCompData->m_sHevc, ComponentParameterStructure, sizeof
                        (OMX_VIDEO_CODEC_PARAM_HEVCTYPE));
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        case OMX_IndexParamVideoBitrate:
            if (((OMX_VIDEO_PARAM_BITRATETYPE*)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->m_sBitrateControl.nPortIndex)
                memcpy(&pQCompData->m_sBitrateControl, ComponentParameterStructure, sizeof
                        (OMX_VIDEO_PARAM_BITRATETYPE));
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        case OMX_IndexParamVideoQuantization:
            if (((OMX_VIDEO_PARAM_QUANTIZATIONTYPE*)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->m_sQuantization.nPortIndex)
                memcpy(&pQCompData->m_sQuantization, ComponentParameterStructure, sizeof
                        (OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        case OMX_IndexParamVideoProfileLevelCurrent:
            if (((OMX_VIDEO_PARAM_PROFILELEVELTYPE*)(ComponentParameterStructure))->nPortIndex
                    == pQCompData->m_sProfileLevel.nPortIndex)
                memcpy(&pQCompData->m_sProfileLevel, ComponentParameterStructure, sizeof
                        (OMX_VIDEO_PARAM_PROFILELEVELTYPE));
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        default:
            /* Handle Vendor specific indices here . Move to a separate function if 
             * The if checks get unmanagable */
            if (index == (OMX_INDEXTYPE)(OMX_VendorTIVPUConfigCoreIndex)) {
                if (((OMX_VENDOR_TIVPU_PARAM_TYPE*)(ComponentParameterStructure))->coreIdx >= 0  &&
                        ((OMX_VENDOR_TIVPU_PARAM_TYPE*)(ComponentParameterStructure))->coreIdx < pQCompData->vendorTIVPUParam.maxVPUCores) {
                    pQCompData->codec_core_idx = ((OMX_VENDOR_TIVPU_PARAM_TYPE*)(ComponentParameterStructure))->coreIdx;
                } else {
                    LOG(LOG_ERROR, "%s:%d Error invalid parameters", __func__, __LINE__);
                    eError = OMX_ErrorBadParameter;
                }
            }
            else if (index == (OMX_INDEXTYPE)(OMX_VendorTIVPUConfigSetLossless)) {
                if (((OMX_VENDOR_TIVPU_PARAM_TYPE*)(ComponentParameterStructure))->setLossless == 0  ||
                        ((OMX_VENDOR_TIVPU_PARAM_TYPE*)(ComponentParameterStructure))->setLossless == 1) {
                    pQCompData->setLossless = ((OMX_VENDOR_TIVPU_PARAM_TYPE*)(ComponentParameterStructure))->setLossless;
                } else {
                    LOG(LOG_ERROR, "%s:%d Set Lossless Error invalid parameters", __func__, __LINE__);
                    eError = OMX_ErrorBadParameter;
                }
            }
            else if (index == (OMX_INDEXTYPE)(OMX_VendorTIVPUConfigSetGOP)) {
                if (((OMX_VENDOR_TIVPU_PARAM_TYPE*)(ComponentParameterStructure))->setGOP == 1  ||
                        ((OMX_VENDOR_TIVPU_PARAM_TYPE*)(ComponentParameterStructure))->setGOP == 9) {
                    pQCompData->setGOP = ((OMX_VENDOR_TIVPU_PARAM_TYPE*)(ComponentParameterStructure))->setGOP;
                        } else {
                    LOG(LOG_ERROR, "%s:%d Set GOP Error invalid parameters", __func__, __LINE__);
                    eError = OMX_ErrorBadParameter;
                        }
            }

            else
                eError = OMX_ErrorUnsupportedIndex;
            break;
    }
    pthread_mutex_unlock(&pQCompData->mutex);

OMX_CONF_CMD_BAIL:
    return eError;
}



/*****************************************************************************/
OMX_ERRORTYPE QCompUseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes,
        OMX_IN OMX_U8* pBuffer)
{
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    OMX_U32 nIndex = 0x0;

    LOG(LOG_DEBUG2, "%s:%d In hComponent=%p nPortIndex=%d", __func__, __LINE__, hComponent, nPortIndex);
    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pQCompData, ppBufferHdr, pBuffer);

    if (nPortIndex == pQCompData->sInPortDef.nPortIndex)
        pPortDef = &pQCompData->sInPortDef;
    else if (nPortIndex == pQCompData->sOutPortDef.nPortIndex)
        pPortDef = &pQCompData->sOutPortDef;
    else
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);

    if (!pPortDef->bEnabled)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);

    if (nSizeBytes < pPortDef->nBufferSize || pPortDef->bPopulated)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);

    // Find an empty position in the BufferList and allocate memory for the buffer header.
    // Use the buffer passed by the client to initialize the actual buffer
    // inside the buffer header.
    pthread_mutex_lock(&pQCompData->mutex);
    if (nPortIndex == pQCompData->sInPortDef.nPortIndex){
        ListAllocate(pQCompData->sInBufList, nIndex);
        if (pQCompData->sInBufList.pAllocHdr[nIndex] == NULL){
            pQCompData->sInBufList.pAllocHdr[nIndex] = (OMX_BUFFERHEADERTYPE*)
                malloc(sizeof(OMX_BUFFERHEADERTYPE)) ;
            if (!pQCompData->sInBufList.pAllocHdr[nIndex]) {
                pthread_mutex_unlock(&pQCompData->mutex);
                OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
            }
            OMX_CONF_INIT_STRUCT_PTR (pQCompData->sInBufList.pAllocHdr[nIndex], OMX_BUFFERHEADERTYPE);
        }
        pQCompData->sInBufList.pAllocHdr[nIndex]->pBuffer = pBuffer;
        pQCompData->sInBufList.bAllocated = OMX_FALSE;
        LoadBufferHeader(pQCompData->sInBufList, pQCompData->sInBufList.pAllocHdr[nIndex], pAppPrivate,
                nSizeBytes, nPortIndex, *ppBufferHdr, pPortDef);
    }else {
        ListAllocate(pQCompData->sOutBufList,  nIndex);
        if (pQCompData->sOutBufList.pAllocHdr[nIndex] == NULL){
            pQCompData->sOutBufList.pAllocHdr[nIndex] = (OMX_BUFFERHEADERTYPE*)
                malloc(sizeof(OMX_BUFFERHEADERTYPE));
            if (!pQCompData->sOutBufList.pAllocHdr[nIndex]) {
                pthread_mutex_unlock(&pQCompData->mutex);
                OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
            }
            OMX_CONF_INIT_STRUCT_PTR (pQCompData->sOutBufList.pAllocHdr[nIndex], OMX_BUFFERHEADERTYPE);
        }
        pQCompData->sOutBufList.pAllocHdr[nIndex]->pBuffer = pBuffer;
        pQCompData->sOutBufList.bAllocated = OMX_FALSE;
        LoadBufferHeader(pQCompData->sOutBufList, pQCompData->sOutBufList.pAllocHdr[nIndex],
                pAppPrivate, nSizeBytes, nPortIndex, *ppBufferHdr, pPortDef);
    }
    if(pPortDef->bPopulated)
        pthread_cond_signal(&pQCompData->cond);
    pthread_mutex_unlock(&pQCompData->mutex);

OMX_CONF_CMD_BAIL:
    LOG(LOG_DEBUG2, "%s:%d Out return eError=%x outbuf list size=(%d,%d)",
                    __func__, __LINE__, eError, pQCompData->sOutBufList.nSizeOfList, pQCompData->sOutBufList.nAllocSize);
    return eError;
}

/* Helper function to abstract the actual shm alloc.
 * This falls back to VISION APPS carveout if the
 * codec carveout fails.
 */
static int allocate_shm(uint64_t size, shm_buf *buf)
{
    int status = 0;

    status = SHM_alloc_aligned_fromBlock_withFlags(size,
            0x1000, BLOCK_IDX_2, buf, PROT_NOCACHE, 0);

    /* not enough memory in the codec carveout, use the vision apps memory as the default carveout */
    if (status != 0)
        status = SHM_alloc_aligned_fromBlock_withFlags(size,
                0x1000, BLOCK_IDX_1, buf, PROT_NOCACHE, 0);

    return status;
}

/*****************************************************************************/
OMX_ERRORTYPE QCompAllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes)
{
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_S8 nIndex = 0x0;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    int status = 0;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pQCompData, ppBufferHdr, 1);

    if (nPortIndex == pQCompData->sInPortDef.nPortIndex)
        pPortDef = &pQCompData->sInPortDef;
    else{
        if (nPortIndex == pQCompData->sOutPortDef.nPortIndex)
            pPortDef = &pQCompData->sOutPortDef;
        else {
            LOG(LOG_ERROR, "%s:%d Invalid port index hComponent=%p nPortIndex=%d", __func__, __LINE__, hComponent, nPortIndex);
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
        }
    }

    if (!pPortDef->bEnabled) {
        LOG(LOG_ERROR, "%s:%d port is not enabled hComponent=%p nPortIndex=%d", __func__, __LINE__, hComponent, nPortIndex);
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);
    }

    if (nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated) {
        LOG(LOG_ERROR, "%s:%d Invaid port size (%d, %d)", __func__, __LINE__, nSizeBytes, pPortDef->nBufferSize);
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);
    }

    // Find an empty position in the BufferList and allocate memory for the buffer header
    // and the actual buffer
    pthread_mutex_lock(&pQCompData->mutex);
    if (nPortIndex == pQCompData->sInPortDef.nPortIndex){
        ListAllocate(pQCompData->sInBufList,  nIndex);
        if (pQCompData->sInBufList.pAllocHdr[nIndex] == NULL){
            pQCompData->sInBufList.pAllocHdr[nIndex] = (OMX_BUFFERHEADERTYPE*)
                malloc(sizeof(OMX_BUFFERHEADERTYPE)) ;
            if (!pQCompData->sInBufList.pAllocHdr[nIndex]) {
                LOG(LOG_ERROR, "%s:%d Failed to allocate memory for buffer headers", __func__, __LINE__);
                OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
            }
            OMX_CONF_INIT_STRUCT_PTR (pQCompData->sInBufList.pAllocHdr[nIndex], OMX_BUFFERHEADERTYPE);
        }

        /* NOTE: BLOCK_IDX_2 is the block reserved for the codec carveout today */
        shm_buf **buf = (shm_buf **)(pQCompData->sInBufList.pShmBufs);
        buf[nIndex] = calloc(1, sizeof(shm_buf));
        if (!buf[nIndex]) {
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
        }

        status = allocate_shm(nSizeBytes, buf[nIndex]);

        if(status != 0 ) {
            LOG(LOG_ERROR, "%s:%d Failed to map memory for output buffers", __func__, __LINE__);
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
        }
        else {
            pQCompData->sInBufList.pAllocHdr[nIndex]->pBuffer = (OMX_U8*)(buf[nIndex]->vir_addr);
            pQCompData->sInBufList.bAllocated = OMX_TRUE;
            LOG(LOG_DEBUG2, "%s:%d, Input buffer allocated with virt addr=0x%lx and phy addr=0x%lx",
                __func__, __LINE__, buf[nIndex]->vir_addr, buf[nIndex]->phy_addr);
        }
        LoadBufferHeader(pQCompData->sInBufList, pQCompData->sInBufList.pAllocHdr[nIndex], pAppPrivate,
                nSizeBytes, nPortIndex, *ppBufferHdr, pPortDef);
    }
    else{
        ListAllocate(pQCompData->sOutBufList,  nIndex);
        if (pQCompData->sOutBufList.pAllocHdr[nIndex] == NULL){
            pQCompData->sOutBufList.pAllocHdr[nIndex] = (OMX_BUFFERHEADERTYPE*)
                malloc(sizeof(OMX_BUFFERHEADERTYPE)) ;
            if (!pQCompData->sOutBufList.pAllocHdr[nIndex]) {
                LOG(LOG_ERROR, "%s:%d Failed to allocate memory for buffer headers", __func__, __LINE__);
                OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
            }
            OMX_CONF_INIT_STRUCT_PTR(pQCompData->sOutBufList.pAllocHdr[nIndex],OMX_BUFFERHEADERTYPE);
        }

        shm_buf **buf = (shm_buf **)(pQCompData->sOutBufList.pShmBufs);
        buf[nIndex] = calloc(1, sizeof(shm_buf));
        if (!buf[nIndex]) {
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
        }

        status = allocate_shm(nSizeBytes, buf[nIndex]);

        if(status != 0 ) {
            LOG(LOG_ERROR, "%s:%d Failed to map memory for output buffers", __func__, __LINE__);
            OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
        }
        else {
            pQCompData->sOutBufList.pAllocHdr[nIndex]->pBuffer = (OMX_U8*)(buf[nIndex]->vir_addr);
            pQCompData->sOutBufList.bAllocated = OMX_TRUE;
            LOG(LOG_DEBUG2, "%s:%d, Output buffer allocated with virt addr=0x%lx and phy addr=0x%lx",
                __func__, __LINE__, buf[nIndex]->vir_addr, buf[nIndex]->phy_addr);
        }
        LoadBufferHeader(pQCompData->sOutBufList, pQCompData->sOutBufList.pAllocHdr[nIndex],
                pAppPrivate, nSizeBytes, nPortIndex, *ppBufferHdr, pPortDef);
    }
    if(pPortDef->bPopulated)
        pthread_cond_signal(&pQCompData->cond);

OMX_CONF_CMD_BAIL:
    pthread_mutex_unlock(&pQCompData->mutex);
    return eError;
}



/*****************************************************************************/
OMX_ERRORTYPE QCompFreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_U32 nPortIndex,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    OMX_S8 nIndex = 0x0;
    int i;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pQCompData, pBufferHdr, 1);
    OMX_CONF_CHK_VERSION(pBufferHdr, OMX_BUFFERHEADERTYPE, eError);

    // Match the pBufferHdr to the appropriate entry in the BufferList
    // and free the allocated memory
    pthread_mutex_lock(&pQCompData->mutex);
    if (nPortIndex == pQCompData->sInPortDef.nPortIndex){
        pPortDef = &pQCompData->sInPortDef;
        if (pQCompData->sInBufList.bAllocated == OMX_TRUE) {
            shm_buf **buf = (shm_buf **)(pQCompData->sInBufList.pShmBufs);
            for(i = 0; i < pPortDef->nBufferCountActual; i++) {
                if (buf[i]->vir_addr == (unsigned long) pBufferHdr->pBuffer) {
                    SHM_release(buf[i]);
                    break;
                }
            }
        }
        ListFreeBuffer(pQCompData->sInBufList, pBufferHdr, pPortDef, nIndex)
    }
    else if (nPortIndex == pQCompData->sOutPortDef.nPortIndex){
        pPortDef = &pQCompData->sOutPortDef;
        if (pQCompData->sOutBufList.bAllocated == OMX_TRUE) {
            shm_buf **buf = (shm_buf **)(pQCompData->sOutBufList.pShmBufs);
            for(i = 0; i < pPortDef->nBufferCountActual; i++) {
                if (buf[i]->vir_addr == (unsigned long) pBufferHdr->pBuffer) {
                    SHM_release(buf[i]);
                    break;
                }
            }
        }
        ListFreeBuffer(pQCompData->sOutBufList, pBufferHdr, pPortDef, nIndex)
    }
    else
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadParameter);

    if (pPortDef->bEnabled && pQCompData->state != OMX_StateIdle)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);

    if(!pPortDef->bPopulated)
        pthread_cond_signal(&pQCompData->cond);

OMX_CONF_CMD_BAIL:
    pthread_mutex_unlock(&pQCompData->mutex);
    return eError;
}

/*****************************************************************************/
OMX_ERRORTYPE QCompEmptyThisBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    QCompDataType *pQCompData;
    QCompCmdType eCmd = EmptyBuf;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pQCompData, pBufferHdr, 1);
    OMX_CONF_CHK_VERSION(pBufferHdr, OMX_BUFFERHEADERTYPE, eError);

    if (!pQCompData->sInPortDef.bEnabled)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);

    if (pBufferHdr->nInputPortIndex != 0x0  || pBufferHdr->nOutputPortIndex != OMX_NOPORT)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadPortIndex);

    if (pQCompData->state != OMX_StateExecuting && pQCompData->state != OMX_StatePause)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);

    // Put the command and data in the queue
    eError = queueCommand(pQCompData, eCmd, (void*)pBufferHdr);

OMX_CONF_CMD_BAIL:
    return eError;
}

/*****************************************************************************/
OMX_ERRORTYPE QCompFillThisBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    QCompDataType *pQCompData;
    QCompCmdType eCmd = FillBuf;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    OMX_CONF_CHECK_CMD(pQCompData, pBufferHdr, 1);
    OMX_CONF_CHK_VERSION(pBufferHdr, OMX_BUFFERHEADERTYPE, eError);

    if (!pQCompData->sOutPortDef.bEnabled)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);

    if (pBufferHdr->nOutputPortIndex != 0x1 || pBufferHdr->nInputPortIndex != OMX_NOPORT)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorBadPortIndex);

    if (pQCompData->state != OMX_StateExecuting && pQCompData->state != OMX_StatePause)
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorIncorrectStateOperation);

    // Put the command and data in the queue
    eError = queueCommand(pQCompData, eCmd, (void *)pBufferHdr);

OMX_CONF_CMD_BAIL:
    return eError;
}


/*****************************************************************************/
OMX_ERRORTYPE QCompDeInit(OMX_IN  OMX_HANDLETYPE hComponent)
{
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    LOG(LOG_DEBUG2,"OMXCOMP:%s:%d pQCompData %p",__func__,__LINE__, pQCompData);
    // Put the command and data in the queue
    pthread_mutex_lock(&pQCompData->mutex);
    pQCompData->thread_running = false;
    pthread_cond_signal(&pQCompData->cond);
    pthread_mutex_unlock(&pQCompData->mutex);

    // Wait for thread to exit so we can get the status into "error"
    pthread_join(pQCompData->thread_id, NULL);

    // close encoder
    if(pQCompData->codec_hdl != NULL) {
        omxil_close_encoder(pQCompData->codec_hdl);
        pQCompData->codec_hdl = NULL;
    }

    if (pQCompData->sInBufList.nAllocSize > 0) {
        LOG(LOG_WARNING,"OMXCOMP:%s:%d Memory leak: %d bufHdr in InBufList was not freed !!!",
                            __func__,__LINE__,pQCompData->sInBufList.nAllocSize );
    }
    free(pQCompData->sInBufList.pAllocHdr);
    free(pQCompData->sInBufList.pBufHdr);

    if (pQCompData->sOutBufList.nAllocSize > 0) {
        LOG(LOG_WARNING,"OMXCOMP:%s:%d Memory leak: %d bufHdr header in OutBufList was not freed !!!",
                            __func__,__LINE__,pQCompData->sOutBufList.nAllocSize );
    }
    free(pQCompData->sOutBufList.pAllocHdr);
    free(pQCompData->sOutBufList.pBufHdr);

    pthread_mutex_destroy(&pQCompData->mutex);
    pthread_cond_destroy(&pQCompData->cond);

    // destroy the command/data queue
    DestroyQueue(pQCompData->cmdQueue);
    DestroyQueue(pQCompData->dataQueue);
    free(pQCompData);
    LOG(LOG_DEBUG2,"OMXCOMP:%s:%d exit",__func__,__LINE__);

    return eError;
}

/*****************************************************************************/
OMX_ERRORTYPE StubbedGetComponentVersion(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_OUT OMX_STRING pComponentName,
            OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
            OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
            OMX_OUT OMX_UUIDTYPE* pComponentUUID)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE QCompGetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex,
            OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);


    pthread_mutex_lock(&pQCompData->mutex);
    switch (nIndex){
        case OMX_IndexConfigVideoBitrate:
            if (((OMX_VIDEO_CONFIG_BITRATETYPE*)(pComponentConfigStructure))->nPortIndex
                    == pQCompData->m_sBitrate.nPortIndex)
                memcpy(pComponentConfigStructure, &pQCompData->m_sBitrate, sizeof
                        (OMX_VIDEO_CONFIG_BITRATETYPE));
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        case OMX_IndexConfigVideoFramerate:
            if (((OMX_CONFIG_FRAMERATETYPE*)(pComponentConfigStructure))->nPortIndex
                    == pQCompData->m_sFrameRate.nPortIndex)
                memcpy(pComponentConfigStructure, &pQCompData->m_sFrameRate, sizeof
                        (OMX_CONFIG_FRAMERATETYPE));
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        case OMX_IndexConfigVideoIntraVOPRefresh:
            if (((OMX_CONFIG_INTRAREFRESHVOPTYPE*)(pComponentConfigStructure))->nPortIndex
                    == pQCompData->m_sIntraRefreshVOP.nPortIndex)
                memcpy(pComponentConfigStructure, &pQCompData->m_sIntraRefreshVOP, sizeof
                        (OMX_CONFIG_INTRAREFRESHVOPTYPE));
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        case OMX_IndexConfigVideoAVCIntraPeriod:
            if (((OMX_VIDEO_CONFIG_AVCINTRAPERIOD*)(pComponentConfigStructure))->nPortIndex
                    == pQCompData->m_sAVCIntraPeriod.nPortIndex)
                memcpy(pComponentConfigStructure, &pQCompData->m_sAVCIntraPeriod, sizeof
                        (OMX_VIDEO_CONFIG_AVCINTRAPERIOD));
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        default:
            eError = OMX_ErrorUnsupportedIndex;
            break;
    }
    pthread_mutex_unlock(&pQCompData->mutex);

    return eError;
}



OMX_ERRORTYPE QCompSetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex,
            OMX_IN  OMX_PTR pComponentConfigStructure)
{
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    pQCompData = (QCompDataType *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    pthread_mutex_lock(&pQCompData->mutex);
    switch (nIndex){
        case OMX_IndexConfigVideoBitrate:
            if (((OMX_VIDEO_CONFIG_BITRATETYPE*)(pComponentConfigStructure))->nPortIndex
                    == pQCompData->m_sBitrate.nPortIndex) {
                memcpy(&pQCompData->m_sBitrate, pComponentConfigStructure, sizeof
                        (OMX_VIDEO_CONFIG_BITRATETYPE));
                if(pQCompData->m_bEncoderConfigured) {
                    LOG(LOG_INFO, "QOmxilComponentEnc::%s:%d too late, encoder already configured", __func__, __LINE__);
                }
            }
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        case OMX_IndexConfigVideoFramerate:
            if (((OMX_CONFIG_FRAMERATETYPE*)(pComponentConfigStructure))->nPortIndex
                    == pQCompData->m_sFrameRate.nPortIndex) {
                memcpy(&pQCompData->m_sFrameRate, pComponentConfigStructure, sizeof
                        (OMX_CONFIG_FRAMERATETYPE));
                if(pQCompData->m_bEncoderConfigured) {
                    LOG(LOG_INFO, "QOmxilComponentEnc::%s:%d too late, encoder already configured", __func__, __LINE__);
                }
            }
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        case OMX_IndexConfigVideoIntraVOPRefresh:
            if (((OMX_CONFIG_INTRAREFRESHVOPTYPE*)(pComponentConfigStructure))->nPortIndex
                    == pQCompData->m_sIntraRefreshVOP.nPortIndex) {
                memcpy(&pQCompData->m_sIntraRefreshVOP, pComponentConfigStructure, sizeof
                        (OMX_CONFIG_INTRAREFRESHVOPTYPE));
                if(pQCompData->m_bEncoderConfigured && pQCompData->m_sIntraRefreshVOP.IntraRefreshVOP)
                    eError = omxil_enc_forceKeyFrame(pQCompData->codec_hdl);
            }
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        case OMX_IndexConfigVideoAVCIntraPeriod:
            if (((OMX_VIDEO_CONFIG_AVCINTRAPERIOD*)(pComponentConfigStructure))->nPortIndex
                    == pQCompData->m_sAVCIntraPeriod.nPortIndex) {
                memcpy(&pQCompData->m_sAVCIntraPeriod, pComponentConfigStructure, sizeof
                        (OMX_VIDEO_CONFIG_AVCINTRAPERIOD));
                if(pQCompData->m_bEncoderConfigured) {
                    LOG(LOG_INFO, "QOmxilComponentEnc::%s:%d too late, encoder already configured", __func__, __LINE__);
                }
            }
            else
                eError = OMX_ErrorBadPortIndex;
            break;
        default:
            eError = OMX_ErrorUnsupportedIndex;
            break;
    }
    pthread_mutex_unlock(&pQCompData->mutex);

    return eError;
}

OMX_ERRORTYPE StubbedGetExtensionIndex(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE StubbedComponentTunnelRequest(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_U32 nPort,
        OMX_IN  OMX_HANDLETYPE hTunneledComp,
        OMX_IN  OMX_U32 nTunneledPort,
        OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE QComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_U8 *cRole,
        OMX_IN OMX_U32 nIndex)
{
    return omxil_comp_role_enum(cRole, nIndex);
}

static OMX_ERRORTYPE QComponentCallbackf(void *ctx, void *data, encoder_cb_type type)
{
    QCompDataType *pQCompData = (QCompDataType*) ctx;
    if(type == QOMX_EMPTY_BUFFER_DONE) {
        OMX_BUFFERHEADERTYPE *pInBufHdr = (OMX_BUFFERHEADERTYPE *)data;
        // Check for mark buffers
        if (pInBufHdr && pInBufHdr->pMarkData) {
            // Trigger event handler
            if (pInBufHdr->pMarkData && pInBufHdr->hMarkTargetComponent == pQCompData->hSelf) {
                LOG(LOG_ERROR, "QOmxilComponentEnc::%s:%d Send OMX_EventMark", __func__, __LINE__);
                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                        OMX_EventMark, 0, 0, pInBufHdr->pMarkData);
            }
        }
        if(pInBufHdr)
          pQCompData->pCallbacks->EmptyBufferDone(pQCompData->hSelf, pQCompData->pAppData, pInBufHdr);
    }
    else if(type == QOMX_FILL_BUFFER_DONE) {
        OMX_BUFFERHEADERTYPE *pOutBufHdr = (OMX_BUFFERHEADERTYPE *)data;
        if (pOutBufHdr->nFlags & OMX_BUFFERFLAG_EOS){
            LOG(LOG_DEBUG2, "%s:%d Send EOS event", __func__, __LINE__);
            pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                    OMX_EventBufferFlag, pQCompData->sOutPortDef.nPortIndex, pOutBufHdr->nFlags, NULL);
        }
        pQCompData->pCallbacks->FillBufferDone(pQCompData->hSelf, pQCompData->pAppData, pOutBufHdr);
    }
    else if(type == QOMX_ERROR) {
        LOG(LOG_ERROR, "Encoder callback Error");
        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                OMX_EventError, OMX_ErrorHardware, 0, NULL);
    }
    else if(type == QOMX_EOS) {
        LOG(LOG_DEBUG2, "%s:%d Send EOS event", __func__, __LINE__);
        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                OMX_EventBufferFlag, pQCompData->sOutPortDef.nPortIndex, OMX_BUFFERFLAG_EOS, NULL);
    }
    return OMX_ErrorNone;
}

/*****************************************************************************/
OMX_ERRORTYPE QCompInit(OMX_IN  OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE *pComp;
    QCompDataType *pQCompData;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U32 err;

    LOG(LOG_DEBUG2, "%s:%d hComponent=%p", __func__, __LINE__, hComponent);
    if(hComponent == NULL) {
        LOG(LOG_ERROR, "QCompInit invalid parameter");
        return OMX_ErrorBadParameter;
    }

    pComp = (OMX_COMPONENTTYPE *)hComponent;

    // Create private data
    pQCompData = (QCompDataType *)calloc(1, sizeof(QCompDataType));
    if(pQCompData == NULL) {
        LOG(LOG_ERROR, "QCompInit failure: no memory for QCompDataType");
        return OMX_ErrorInsufficientResources;
    }

    pComp->pComponentPrivate = (OMX_PTR)pQCompData;
    pQCompData->state = OMX_StateLoaded;
    pQCompData->hSelf = hComponent;

    // Fill in function pointers
    pComp->SetCallbacks =           QCompSetCallbacks;
    pComp->GetComponentVersion =    StubbedGetComponentVersion;
    pComp->SendCommand =            QCompSendCommand;
    pComp->GetParameter =           QCompGetParameter;
    pComp->SetParameter =           QCompSetParameter;
    pComp->GetConfig =              QCompGetConfig;
    pComp->SetConfig =              QCompSetConfig;
    pComp->GetExtensionIndex =      StubbedGetExtensionIndex;
    pComp->GetState =               QCompGetState;
    pComp->ComponentTunnelRequest = StubbedComponentTunnelRequest;
    pComp->UseBuffer =              QCompUseBuffer;
    pComp->AllocateBuffer =         QCompAllocateBuffer;
    pComp->FreeBuffer =             QCompFreeBuffer;
    pComp->EmptyThisBuffer =        QCompEmptyThisBuffer;
    pComp->FillThisBuffer =         QCompFillThisBuffer;
    pComp->ComponentRoleEnum =      QComponentRoleEnum;
    pComp->ComponentDeInit =        QCompDeInit;

    // Initialize component data structures to default values
    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->sPortParam, OMX_PORT_PARAM_TYPE);
    pQCompData->sPortParam.nPorts = 0x2;
    pQCompData->sPortParam.nStartPortNumber = 0x0;
    pQCompData->vendorTIVPUParam.maxVPUCores = MAX_NUM_VPU_CORE;
    pQCompData->vendorTIVPUParam.coreIdx = 0;

    // Initialize the video parameters for input port
    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->sInPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    pQCompData->sInPortDef.nPortIndex = 0x0;
    pQCompData->sInPortDef.bEnabled = OMX_TRUE;
    pQCompData->sInPortDef.bPopulated = OMX_FALSE;
    pQCompData->sInPortDef.eDomain = OMX_PortDomainVideo;
    pQCompData->sInPortDef.format.video.cMIMEType = NULL;
    pQCompData->sInPortDef.format.video.nFrameWidth = 0;
    pQCompData->sInPortDef.format.video.nFrameHeight = 0;
    pQCompData->sInPortDef.eDir = OMX_DirInput;
    pQCompData->sInPortDef.nBufferCountMin = MIN_NUM_IN_BUFFERS;
    pQCompData->sInPortDef.nBufferCountActual = NUM_IN_BUFFERS;
    pQCompData->sInPortDef.nBufferSize =  0;
    pQCompData->sInPortDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;

    // Initialize the video parameters for output port
    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->sOutPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    pQCompData->sOutPortDef.nPortIndex = 0x1;
    pQCompData->sOutPortDef.bEnabled = OMX_TRUE;
    pQCompData->sOutPortDef.bPopulated = OMX_FALSE;
    pQCompData->sOutPortDef.eDomain = OMX_PortDomainVideo;
    pQCompData->sOutPortDef.format.video.cMIMEType = NULL;
    pQCompData->sOutPortDef.format.video.nFrameWidth = 0;
    pQCompData->sOutPortDef.format.video.nFrameHeight = 0;
    pQCompData->sOutPortDef.eDir = OMX_DirOutput;
    pQCompData->sOutPortDef.nBufferCountMin = MIN_NUM_OUT_BUFFERS;
    pQCompData->sOutPortDef.nBufferCountActual = NUM_OUT_BUFFERS;
    pQCompData->sOutPortDef.nBufferSize =  0;
    pQCompData->sOutPortDef.format.video.eColorFormat =  OMX_COLOR_FormatUnused;

    // Initialize the video compression format for input port
    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->sInPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    pQCompData->sInPortFormat.nPortIndex = 0x0;
    pQCompData->sInPortFormat.nIndex = 0x0;
    pQCompData->sInPortFormat.eCompressionFormat =  OMX_VIDEO_CodingUnused;

    // Initialize the compression format for output port
    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->sOutPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    pQCompData->sOutPortFormat.nPortIndex = 0x1;
    pQCompData->sOutPortFormat.nIndex = 0x0;
    pQCompData->sOutPortFormat.eCompressionFormat = OMX_VIDEO_CodingAVC;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->sInBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE );
    pQCompData->sInBufSupplier.nPortIndex = 0x0;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->sOutBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE );
    pQCompData->sOutBufSupplier.nPortIndex = 0x1;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->m_sAvc, OMX_VIDEO_PARAM_AVCTYPE);
    pQCompData->m_sAvc.nPortIndex = 0x1;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->m_sHevc, OMX_VIDEO_CODEC_PARAM_HEVCTYPE);
    pQCompData->m_sHevc.nPortIndex = 0x1;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->m_sProfileLevel, OMX_VIDEO_PARAM_PROFILELEVELTYPE);
    pQCompData->m_sProfileLevel.nPortIndex = 0x1;
    pQCompData->m_sProfileLevel.nProfileIndex = 0;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->m_sBitrateControl, OMX_VIDEO_PARAM_BITRATETYPE);
    pQCompData->m_sBitrateControl.nPortIndex = 0x1;
    pQCompData->m_sBitrateControl.nTargetBitrate = 0;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->m_sQuantization, OMX_VIDEO_PARAM_QUANTIZATIONTYPE);
    pQCompData->m_sQuantization.nPortIndex = 0x1;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->m_sBitrate, OMX_VIDEO_CONFIG_BITRATETYPE);
    pQCompData->m_sBitrate.nPortIndex = 0x1;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->m_sFrameRate, OMX_CONFIG_FRAMERATETYPE);
    pQCompData->m_sFrameRate.nPortIndex = 0x1;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->m_sIntraRefreshVOP, OMX_CONFIG_INTRAREFRESHVOPTYPE);
    pQCompData->m_sIntraRefreshVOP.nPortIndex = 0x1;

    OMX_CONF_INIT_STRUCT_PTR(&pQCompData->m_sAVCIntraPeriod, OMX_VIDEO_CONFIG_AVCINTRAPERIOD);
    pQCompData->m_sAVCIntraPeriod.nPortIndex = 0x1;

    // Initialize the input buffer list
    pQCompData->sInBufList.nSizeOfList = 0;
    pQCompData->sInBufList.nAllocSize = 0;
    pQCompData->sInBufList.nListEnd = -1;
    pQCompData->sInBufList.nWritePos = -1;
    pQCompData->sInBufList.nReadPos = -1;
    pQCompData->sInBufList.eDir = OMX_DirInput;

    // Initialize the output buffer list
    pQCompData->sOutBufList.nSizeOfList = 0;
    pQCompData->sOutBufList.nAllocSize = 0;
    pQCompData->sOutBufList.nListEnd = -1;
    pQCompData->sOutBufList.nWritePos = -1;
    pQCompData->sOutBufList.nReadPos = -1;
    pQCompData->sOutBufList.eDir = OMX_DirOutput;

    pQCompData->firstFrame = 1;

    // Create the queue used to send commands to the thread
    pQCompData->cmdQueue = CreateQueue(OMX_MAX_QUEUE_SIZE, "OMXIL command queue");
    if (pQCompData->cmdQueue == NULL){
        LOG( LOG_ERROR, "%s failure, No memory for cmdQueue", __func__ );
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    // Create the command/data used to send command data to the thread
    pQCompData->dataQueue = CreateQueue(OMX_MAX_QUEUE_SIZE, "OMXIL command data queue");
    if (pQCompData->dataQueue == NULL){
        LOG( LOG_ERROR, "%s failure, No memory for dataQueue", __func__ );
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    //create encoder handle
    pQCompData->codec_hdl = omxil_create_encoder(QComponentCallbackf, (void*)pQCompData);
    if(pQCompData->codec_hdl == NULL) {
        LOG( LOG_ERROR, "%s failure, codec create error", __func__ );
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    //thread lock for input/output port
    if(pthread_mutex_init( &pQCompData->mutex, NULL ) != EOK) {
        LOG( LOG_ERROR, "%s failure, mutex init err ", __func__ );
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    if(pthread_cond_init( &pQCompData->cond, NULL ) != EOK) {
        LOG( LOG_ERROR, "%s failure, cond init err ", __func__ );
        pthread_mutex_destroy(&pQCompData->mutex);
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

    // Create the component thread
    pQCompData->thread_running = true;
    err = pthread_create(&pQCompData->thread_id, NULL, ComponentThread, pQCompData);
    if( err || !pQCompData->thread_id ) {
        pthread_mutex_destroy(&pQCompData->mutex);
        pthread_cond_destroy( &pQCompData->cond );
        OMX_CONF_SET_ERROR_BAIL(eError, OMX_ErrorInsufficientResources);
    }

OMX_CONF_CMD_BAIL:
    LOG(LOG_DEBUG2, "%s:%d return eError=%x", __func__, __LINE__, eError);
    if(eError != OMX_ErrorNone) {
        if(pQCompData->cmdQueue)
            DestroyQueue(pQCompData->cmdQueue);
        if(pQCompData->dataQueue)
            DestroyQueue(pQCompData->dataQueue);
        if(pQCompData->codec_hdl)
            omxil_close_encoder(pQCompData->codec_hdl);
        free(pQCompData);
    }
    return eError;
}

static void ComponentHandleCmd(QCompDataType* pData, void *aCodec, QCompCmdType aCmd, void *aCmdData)
{
    QCompDataType* pQCompData = pData;
    // State transition command
    if (aCmd == SetState) {
        // If the parameter states a transition to the same state
        //   raise a same state transition error.
        if (pQCompData->state == (OMX_STATETYPE)(aCmdData))
            pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                    OMX_EventError, OMX_ErrorSameState, 0 , NULL);
        else {
            // transitions/callbacks made based on state transition table
            // cmddata contains the target state
            switch ((OMX_STATETYPE)(aCmdData)){
                case OMX_StateInvalid:
                    pQCompData->state = OMX_StateInvalid;
                    pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                            OMX_EventError, OMX_ErrorInvalidState, 0 , NULL);
                    pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                            OMX_EventCmdComplete, OMX_CommandStateSet, pQCompData->state, NULL);
                    break;
                case OMX_StateLoaded:
                    if (pQCompData->state == OMX_StateIdle ||
                            pQCompData->state == OMX_StateWaitForResources){
                        pthread_mutex_lock(&pQCompData->mutex);
                        while (pQCompData->thread_running){
                            // Transition happens only when the ports are unpopulated
                            if (!pQCompData->sInPortDef.bPopulated &&
                                    !pQCompData->sOutPortDef.bPopulated){

                                pQCompData->state = OMX_StateLoaded;
                                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,
                                        pQCompData->pAppData, OMX_EventCmdComplete,
                                        OMX_CommandStateSet, pQCompData->state, NULL);
                                break;
                            }
                            else {
                                LOG(LOG_DEBUG2, "%s:%d Transition to loaded, waiting for port unpopulated(In %d, Out %d)",
                                        __func__, __LINE__, pQCompData->sInPortDef.bPopulated, pQCompData->sOutPortDef.bPopulated);
                                pthread_cond_wait( &pQCompData->cond, &pQCompData->mutex );
                            }
                        }
                        pthread_mutex_unlock(&pQCompData->mutex);
                    }
                    else
                        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                                OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
                    break;
                case OMX_StateIdle:
                    if (pQCompData->state == OMX_StateInvalid)
                        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                                OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
                    else{
                        if(!pQCompData->m_bEncoderConfigured) {
                            if((configEncoder(&pQCompData->m_sConfig, pQCompData)) == OMX_ErrorNone) {
                                pQCompData->m_bEncoderConfigured = true;
                                pthread_cond_signal(&pQCompData->cond);
                            }
                        }

                        // Return buffers if currently in pause and executing
                        pthread_mutex_lock(&pQCompData->mutex);
                        if (pQCompData->state == OMX_StatePause ||
                                pQCompData->state == OMX_StateExecuting){
                            ListFlushEntries(pQCompData->sInBufList, pQCompData)
                            ListFlushEntries(pQCompData->sOutBufList, pQCompData)
                        }

                        while (pQCompData->thread_running){
                            // Ports have to be populated before transition completes
                            if ((!pQCompData->sInPortDef.bEnabled &&
                                        !pQCompData->sOutPortDef.bEnabled)||
                                    (pQCompData->sInPortDef.bPopulated &&
                                     pQCompData->sOutPortDef.bPopulated)){
                                pQCompData->state = OMX_StateIdle;
                                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,
                                        pQCompData->pAppData, OMX_EventCmdComplete,
                                        OMX_CommandStateSet, pQCompData->state, NULL);
                                break;
                            }
                            else {
                                LOG(LOG_DEBUG2, "%s:%d Transition to Idle, waiting for port populated(In %d, Out %d)",
                                        __func__, __LINE__, pQCompData->sInPortDef.bPopulated, pQCompData->sOutPortDef.bPopulated);
                                pthread_cond_wait( &pQCompData->cond, &pQCompData->mutex );
                            }
                        }
                        pthread_mutex_unlock(&pQCompData->mutex);
                    }
                    break;
                case OMX_StateExecuting:
                    // Transition can only happen from pause or idle state
                    if (pQCompData->state == OMX_StateIdle ||
                            pQCompData->state == OMX_StatePause){
                        // Return buffers if currently in pause
                        pthread_mutex_lock(&pQCompData->mutex);
                        if (pQCompData->state == OMX_StatePause){
                            ListFlushEntries(pQCompData->sInBufList, pQCompData)
                            ListFlushEntries(pQCompData->sOutBufList, pQCompData)
                        }
                        else if (pQCompData->state == OMX_StateIdle){
                            /* JB: VPU: Must call omxil_enc_register_buffers for sOutBufList (bitstream bufs) first! */
                            if((omxil_enc_register_buffers(pQCompData->codec_hdl, &(pQCompData->sOutBufList))) != OMX_ErrorNone ||
                                    (omxil_enc_register_buffers(pQCompData->codec_hdl, &(pQCompData->sInBufList))) != OMX_ErrorNone ) {
                                LOG(LOG_ERROR, "%s:%d omxil_enc_register_buffers returned error", __func__, __LINE__);
                                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                                        OMX_EventError, OMX_ErrorHardware, 0, NULL);
                                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                                        OMX_EventCmdComplete, OMX_CommandStateSet, pQCompData->state, NULL);
                                pthread_mutex_unlock(&pQCompData->mutex);
                                break;
                            }
                        }
                        pthread_mutex_unlock(&pQCompData->mutex);

                        if(omxil_enc_start(pQCompData->codec_hdl) != OMX_ErrorNone) {
                            LOG(LOG_ERROR, "Encoder start return Error");
                            pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                                    OMX_EventError, OMX_ErrorHardware, 0, NULL);
                        }
                        else {
                            pQCompData->state = OMX_StateExecuting;
                            pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                                    OMX_EventCmdComplete, OMX_CommandStateSet, pQCompData->state, NULL);
                        }
                    }
                    else
                        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                                OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
                    break;
                case OMX_StatePause:
                    // Transition can only happen from idle or executing state
                    if (pQCompData->state == OMX_StateIdle ||
                            pQCompData->state == OMX_StateExecuting){
                        pQCompData->state = OMX_StatePause;
                        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                                OMX_EventCmdComplete, OMX_CommandStateSet, pQCompData->state, NULL);
                    }
                    else
                        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                                OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
                    break;
                case OMX_StateWaitForResources:
                    if (pQCompData->state == OMX_StateLoaded) {
                        pQCompData->state = OMX_StateWaitForResources;
                        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                                OMX_EventCmdComplete, OMX_CommandStateSet, pQCompData->state, NULL);
                    }
                    else
                        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,pQCompData->pAppData,
                                OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
                    break;
                default:
                    break;
            }
        }
    }
    else if (aCmd == StopPort) {
        int32_t index = (intptr_t)aCmdData;
        pthread_mutex_lock(&pQCompData->mutex);
        // Stop Port(s)
        // cmddata contains the port index to be stopped.
        // It is assumed that 0 is input and 1 is output port for this component
        // The index value -1 means that both input and output ports will be stopped.
        if (index == 0x0 || index == -1){
            // Return all input buffers
            ListFlushEntries(pQCompData->sInBufList, pQCompData)

            // Disable port
            pQCompData->sInPortDef.bEnabled = OMX_FALSE;
        }
        if (index == 0x1 || index == -1){
            // Return all output buffers
            ListFlushEntries(pQCompData->sOutBufList, pQCompData)

            // Disable port
            pQCompData->sOutPortDef.bEnabled = OMX_FALSE;
        }

        // Wait for all buffers to be freed
        while (pQCompData->thread_running){
            if (index == 0x0 && !pQCompData->sInPortDef.bPopulated){
                // Return cmdcomplete event if input unpopulated
                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                        OMX_EventCmdComplete, OMX_CommandPortDisable, 0x0, NULL);
                break;
            }
            if (index == 0x1 && !pQCompData->sOutPortDef.bPopulated){
                // Return cmdcomplete event if output unpopulated
                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                        OMX_EventCmdComplete, OMX_CommandPortDisable, 0x1, NULL);
                break;
            }
            if (index == -1 &&  !pQCompData->sInPortDef.bPopulated &&
                    !pQCompData->sOutPortDef.bPopulated){
                // Return cmdcomplete event if inout & output unpopulated
                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                        OMX_EventCmdComplete, OMX_CommandPortDisable, 0x0, NULL);
                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                        OMX_EventCmdComplete, OMX_CommandPortDisable, 0x1, NULL);
                break;
            }
            pthread_cond_wait(&pQCompData->cond, &pQCompData->mutex);
        }
        pthread_mutex_unlock(&pQCompData->mutex);
    }
    else if (aCmd == RestartPort) {
        int32_t index = (intptr_t)aCmdData;
        pthread_mutex_lock(&pQCompData->mutex);
        // Restart Port(s)
        // cmddata contains the port index to be restarted.
        // It is assumed that 0 is input and 1 is output port for this component.
        // The index value -1 means both input and output ports will be restarted.
        if (index == 0x0 || index == -1)
            pQCompData->sInPortDef.bEnabled = OMX_TRUE;
        if (index == 0x1 || index == -1)
            pQCompData->sOutPortDef.bEnabled = OMX_TRUE;

        // Wait for port to be populated
        while (pQCompData->thread_running){
            // Return cmdcomplete event if input port populated
            if (index == 0x0 && (pQCompData->state == OMX_StateLoaded ||
                        pQCompData->sInPortDef.bPopulated)){
                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                        OMX_EventCmdComplete, OMX_CommandPortEnable, 0x0, NULL);
                break;
            }
            // Return cmdcomplete event if output port populated
            else if (index == 0x1 && (pQCompData->state == OMX_StateLoaded ||
                        pQCompData->sOutPortDef.bPopulated)){
                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,
                        pQCompData->pAppData, OMX_EventCmdComplete,
                        OMX_CommandPortEnable, 0x1, NULL);
                break;
            }
            // Return cmdcomplete event if input and output ports populated
            else if (index == -1 && (pQCompData->state == OMX_StateLoaded ||
                        (pQCompData->sInPortDef.bPopulated &&
                         pQCompData->sOutPortDef.bPopulated))){
                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,
                        pQCompData->pAppData, OMX_EventCmdComplete,
                        OMX_CommandPortEnable, 0x0, NULL);
                pQCompData->pCallbacks->EventHandler(pQCompData->hSelf,
                        pQCompData->pAppData, OMX_EventCmdComplete,
                        OMX_CommandPortEnable, 0x1, NULL);
                break;
            }
            pthread_cond_wait(&pQCompData->cond, &pQCompData->mutex);
        }
        pthread_mutex_unlock(&pQCompData->mutex);
    }
    else if (aCmd == Flush) {
        int32_t index = (intptr_t)aCmdData;

        // Flush port(s)
        // cmddata contains the port index to be flushed.
        // It is assumed that 0 is input and 1 is output port for this component
        // The index value -1 means that both input and output ports will be flushed.
        if (index == 0x0 || index == -1){
            // Return all input buffers and send cmdcomplete
            pthread_mutex_lock(&pQCompData->mutex);
            ListFlushEntries(pQCompData->sInBufList, pQCompData)
            pthread_mutex_unlock(&pQCompData->mutex);
            pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                OMX_EventCmdComplete, OMX_CommandFlush, 0x0, NULL);
        }
        if (index == 0x1 || index == -1){
            // Return all output buffers and send cmdcomplete
            pthread_mutex_lock(&pQCompData->mutex);
            ListFlushEntries(pQCompData->sOutBufList, pQCompData)
            pthread_mutex_unlock(&pQCompData->mutex);
            pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                    OMX_EventCmdComplete, OMX_CommandFlush, 0x1, NULL);
        }
    }
    else if (aCmd == FillBuf) {
        OMX_BUFFERHEADERTYPE *bufHdr = (OMX_BUFFERHEADERTYPE *)aCmdData;
        // Fill buffer
        pthread_mutex_lock(&pQCompData->mutex);
        ListSetEntry(pQCompData->sOutBufList, bufHdr)
        pthread_cond_signal(&pQCompData->cond);
        pthread_mutex_unlock(&pQCompData->mutex);
    }
    else if (aCmd == EmptyBuf) {
        OMX_BUFFERHEADERTYPE *bufHdr = (OMX_BUFFERHEADERTYPE *)aCmdData;
        // Empty buffer
        pthread_mutex_lock(&pQCompData->mutex);
        ListSetEntry(pQCompData->sInBufList, bufHdr)
        //Signal to ComponentThread
        //wait for input buffer/output buffer/other command
        pthread_cond_signal(&pQCompData->cond);
        pthread_mutex_unlock(&pQCompData->mutex);

        // Mark current buffer if there is outstanding command
        if (pQCompData->pMarkBuf){
            bufHdr->hMarkTargetComponent =  pQCompData->pMarkBuf->hMarkTargetComponent;
            bufHdr->pMarkData = pQCompData->pMarkBuf->pMarkData;
            pQCompData->pMarkBuf = NULL;
        }
    }
    else if (aCmd == MarkBuf) {
        if (!pQCompData->pMarkBuf)
            pQCompData->pMarkBuf = (OMX_MARKTYPE *)(aCmdData);
    }
}
/*
 *  Component Thread
 *    process command/data
 */
/*****************************************************************************/
static void* ComponentThread(void* pThreadData)
{
    OMX_ERRORTYPE err;
    // Recover the pointer to component specific data
    QCompDataType* pQCompData = (QCompDataType*)pThreadData;
    OMX_BUFFERHEADERTYPE *pOutBufHdr_first = NULL;
    pthread_setname_np(pthread_self(), "OmxilEncComponentThread");

    while (pQCompData->thread_running) {
        // Variables related to decoder buffer handling
        OMX_BUFFERHEADERTYPE *pInBufHdr = NULL;
        OMX_BUFFERHEADERTYPE *pOutBufHdr = NULL;
        void *pCodec = pQCompData->codec_hdl;

        pthread_mutex_lock(&pQCompData->mutex);
        QCompCmdType cmd = (QCompCmdType)QueueGet(pQCompData->cmdQueue);
        void *cmddata = QueueGet(pQCompData->dataQueue);

        //Need to wait if there is no command and state is not Executing
        //In Executing state, Need to process some buffered data
        if(cmd == CmdNull && pQCompData->state != OMX_StateExecuting) {
            pthread_cond_wait( &pQCompData->cond, &pQCompData->mutex );
            pthread_mutex_unlock(&pQCompData->mutex);
            continue;
        }
        pthread_mutex_unlock(&pQCompData->mutex);

        ComponentHandleCmd(pQCompData, pCodec, cmd, cmddata);
        // Buffer processing
        // Only happens when the component is in executing state.
        if (pQCompData->state == OMX_StateExecuting && pQCompData->sInPortDef.bEnabled &&
                pQCompData->sOutPortDef.bEnabled) {
            if(pQCompData->m_bEncoderConfigured) {
                pthread_mutex_lock(&pQCompData->mutex);
                ListPeekEntry(pQCompData->sInBufList, pInBufHdr);
                if(pInBufHdr)
                    ListPeekEntry(pQCompData->sOutBufList, pOutBufHdr);
                pthread_mutex_unlock(&pQCompData->mutex);

                if (pInBufHdr && pOutBufHdr) {
                    pOutBufHdr->nFilledLen = 0;
                    pthread_mutex_lock(&pQCompData->mutex);
                    ListFlushEntry(pQCompData->sInBufList);
                    if (pQCompData->firstFrame) {
                        ListFlushEntry(pQCompData->sOutBufList);
                        pOutBufHdr_first = pOutBufHdr;
                        ListPeekEntry(pQCompData->sOutBufList, pOutBufHdr);
                        pOutBufHdr->nFilledLen = 0;
                        pQCompData->firstFrame = 0;
                    }
                    ListFlushEntry(pQCompData->sOutBufList);
                    pthread_mutex_unlock(&pQCompData->mutex);
                    err = omxil_enc_encodeFrame(pQCompData->codec_hdl, pInBufHdr, pOutBufHdr, pOutBufHdr_first);
                    if (err != OMX_ErrorNone) {
                        LOG(LOG_ERROR, "QOmxilComponentEnc::%s:%d encodeFrame return error 0x%x", __func__, __LINE__, err);
                        pQCompData->pCallbacks->EventHandler(pQCompData->hSelf, pQCompData->pAppData,
                                OMX_EventError, OMX_ErrorHardware, 0, NULL);
                    }
                }
                else {
                    //Need to wait if there is no command, no output buffer.
                    pthread_mutex_lock(&pQCompData->mutex);
                    cmd = (QCompCmdType)QueuePeek(pQCompData->cmdQueue);
                    if(cmd == CmdNull) {
                        pthread_cond_wait( &pQCompData->cond, &pQCompData->mutex );
                    }
                    pthread_mutex_unlock(&pQCompData->mutex);
                }
            }//m_bEncoderConfigured
        }
    }

    LOG(LOG_INFO, "%s:%d exit", __func__, __LINE__);
    return (void*)OMX_ErrorNone;
}

/*****************************************************************************/
OMX_API OMX_ERRORTYPE QCompGetInfo(omx_component_info *pInfo)
{
    if(pInfo == NULL) {
        LOG(LOG_ERROR, "%s:%d Error Input parameter is NULL", __func__, __LINE__);
        return OMX_ErrorBadParameter;
    }

    pInfo->name = (char *)COMPONENT_NAME;

    omxil_get_roles(pInfo->roles, OMX_CORE_MAX_CMP_ROLES);

    pInfo->fn_ptr = QCompInit;
    return OMX_ErrorNone;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
