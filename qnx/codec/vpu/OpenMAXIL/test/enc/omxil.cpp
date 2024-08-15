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

#include <inttypes.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> // calloc
#include <unistd.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <string.h>
#include <stdbool.h>
#include <gulliver.h>
#include <sys/slog2.h>
#include <sys/slogcodes.h>
#include "OMX_Extension_video_TI.h"
#include "OMX_Extension_index_TI.h"
#include "omxil.h"

static const char *QNX_ENC_COMP_NAME = "OMX.qnx.video.encoder";
// NAL start code length in bytes.

#define OMX_SPEC_VERSION 0x00000001     // OMX Version
#define SET_OMX_VERSION_SIZE( param, size ) {             \
    param.nVersion.nVersion = OMX_SPEC_VERSION;           \
    param.nSize = size;                                   \
}

#if defined (DEBUG_MODE)
    #define TIMEOUT_WAIT (0x1FFFFFFFFFFF) /* for debug */
#else
    #define TIMEOUT_WAIT (5000 * 1000LL * 1000LL)  /* 5000 ms */
#endif

const char* OmxErrorTypeToStr( OMX_ERRORTYPE  err )
{
  const char *str = "Unknown error";
  switch( err )
  {
    case OMX_ErrorNone:                               str = "OMX_ErrorNone";                               break;
    case OMX_ErrorInsufficientResources:              str = "OMX_ErrorInsufficientResources";              break;
    case OMX_ErrorUndefined:                          str = "OMX_ErrorUndefined";                          break;
    case OMX_ErrorInvalidComponentName:               str = "OMX_ErrorInvalidComponentName";               break;
    case OMX_ErrorComponentNotFound:                  str = "OMX_ErrorComponentNotFound";                  break;
    case OMX_ErrorInvalidComponent:                   str = "OMX_ErrorInvalidComponent";                   break;
    case OMX_ErrorBadParameter:                       str = "OMX_ErrorBadParameter";                       break;
    case OMX_ErrorNotImplemented:                     str = "OMX_ErrorNotImplemented";                     break;
    case OMX_ErrorUnderflow:                          str = "OMX_ErrorUnderflow";                          break;
    case OMX_ErrorOverflow:                           str = "OMX_ErrorOverflow";                           break;
    case OMX_ErrorHardware:                           str = "OMX_ErrorHardware";                           break;
    case OMX_ErrorInvalidState:                       str = "OMX_ErrorInvalidState";                       break;
    case OMX_ErrorStreamCorrupt:                      str = "OMX_ErrorStreamCorrupt";                      break;
    case OMX_ErrorPortsNotCompatible:                 str = "OMX_ErrorPortsNotCompatible";                 break;
    case OMX_ErrorResourcesLost:                      str = "OMX_ErrorResourcesLost";                      break;
    case OMX_ErrorNoMore:                             str = "OMX_ErrorNoMore";                             break;
    case OMX_ErrorVersionMismatch:                    str = "OMX_ErrorVersionMismatch";                    break;
    case OMX_ErrorNotReady:                           str = "OMX_ErrorNotReady";                           break;
    case OMX_ErrorTimeout:                            str = "OMX_ErrorTimeout";                            break;
    case OMX_ErrorSameState:                          str = "OMX_ErrorSameState";                          break;
    case OMX_ErrorResourcesPreempted:                 str = "OMX_ErrorResourcesPreempted";                 break;
    case OMX_ErrorPortUnresponsiveDuringAllocation:   str = "OMX_ErrorPortUnresponsiveDuringAllocation";   break;
    case OMX_ErrorPortUnresponsiveDuringDeallocation: str = "OMX_ErrorPortUnresponsiveDuringDeallocation"; break;
    case OMX_ErrorPortUnresponsiveDuringStop:         str = "OMX_ErrorPortUnresponsiveDuringStop";         break;
    case OMX_ErrorIncorrectStateTransition:           str = "OMX_ErrorIncorrectStateTransition";           break;
    case OMX_ErrorIncorrectStateOperation:            str = "OMX_ErrorIncorrectStateOperation";            break;
    case OMX_ErrorUnsupportedSetting:                 str = "OMX_ErrorUnsupportedSetting";                 break;
    case OMX_ErrorUnsupportedIndex:                   str = "OMX_ErrorUnsupportedIndex";                   break;
    case OMX_ErrorBadPortIndex:                       str = "OMX_ErrorBadPortIndex";                       break;
    case OMX_ErrorPortUnpopulated:                    str = "OMX_ErrorPortUnpopulated";                    break;
    case OMX_ErrorComponentSuspended:                 str = "OMX_ErrorComponentSuspended";                 break;
    case OMX_ErrorDynamicResourcesUnavailable:        str = "OMX_ErrorDynamicResourcesUnavailable";        break;
    case OMX_ErrorMbErrorsInFrame:                    str = "OMX_ErrorMbErrorsInFrame";                    break;
    case OMX_ErrorFormatNotDetected:                  str = "OMX_ErrorFormatNotDetected";                  break;
    case OMX_ErrorContentPipeOpenFailed:              str = "OMX_ErrorContentPipeOpenFailed";              break;
    case OMX_ErrorContentPipeCreationFailed:          str = "OMX_ErrorContentPipeCreationFailed";          break;
    case OMX_ErrorSeperateTablesUsed:                 str = "OMX_ErrorSeperateTablesUsed";                 break;
    case OMX_ErrorTunnelingUnsupported:               str = "OMX_ErrorTunnelingUnsupported";               break;
    default: AO_LOG(AO_LOG_ERROR, "OmxilEnc:%s Unlnown OMX_ERRORTYPE=0x%x", __func__, err);      break;
  }
  return str;
}

//------------------------------------------------------------------------------
static void timedwait( OmxilEnc_t *encH, const char *caller )
{
    // !!! the mutex is already being held by the caller
    struct timespec to;
    clock_gettime( CLOCK_MONOTONIC, &to );
    nsec2timespec( &to, timespec2nsec( &to ) + (uint64_t)TIMEOUT_WAIT );
    int r = pthread_cond_timedwait( &encH->cond, &encH->mutex, &to );
    switch ( r ) {
    case EOK:
        break;
    case ETIMEDOUT:
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s timed-out", caller );
        encH->compError = OMX_ErrorTimeout;
        break;
    default:
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s undefined error: %d", caller, r );
        encH->compError = OMX_ErrorUndefined;
    }
}

static OMX_ERRORTYPE waitForCommandComplete( OmxilEnc_t *encH )
{
    pthread_mutex_lock( &encH->mutex );
    while( !encH->cmdComplete && encH->compError == OMX_ErrorNone) {
        timedwait( encH, __func__ );
    }
    OMX_ERRORTYPE err = encH->compError;
    pthread_mutex_unlock( &encH->mutex );
    return err;
}

//------------------------------------------------------------------------------
static OMX_ERRORTYPE EventHandler(
    OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent,
    OMX_U32 nData1,
    OMX_U32 nData2,
    OMX_PTR pEventData )
{
    OmxilEnc_t *encH = (OmxilEnc_t *) pAppData;
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    if( encH == NULL ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> EventHandler Fatal error encH is NULL");
        return OMX_ErrorUndefined;
    }

    if( encH->compHandle == NULL) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> EventHandler: compHandle is NULL" );
        return OMX_ErrorUndefined;
    }

    switch( eEvent ) {
        case OMX_EventError:
        {
            if( hComponent == encH->compHandle ) {
                if (OMX_ErrorStreamCorrupt == (OMX_ERRORTYPE) nData1) {
                    AO_LOG(AO_LOG_ERROR, "OmxilEnc:%s corrupted stream detected; continuing...", __func__);
                } else {
                    pthread_mutex_lock( &encH->mutex );
                    encH->compError = (OMX_ERRORTYPE) nData1;
                    AO_LOG(AO_LOG_ERROR, "OmxilEnc:%s err=0x%x:'%s' encH=%p", __func__, encH->compError, OmxErrorTypeToStr(encH->compError), encH);
                    pthread_cond_broadcast( &encH->cond );
                    pthread_mutex_unlock( &encH->mutex );
                }
            }

            break;
        }

        case OMX_EventCmdComplete:
        {
            switch( (OMX_COMMANDTYPE) nData1 ) {
                case OMX_CommandStateSet:
                {
                    // In this case, nData2 is the arrived at state
                    if( hComponent == encH->compHandle ) {
                        AO_LOG( AO_LOG_INFO,
                                  "OmxilEnc=> Reached compState: %d, encH=%p",
                                  (OMX_STATETYPE) nData2, encH );
                        pthread_mutex_lock( &encH->mutex );
                        encH->cmdComplete = true;
                        pthread_cond_signal( &encH->cond );
                        pthread_mutex_unlock( &encH->mutex );

                    }

                    break;
                }

                case OMX_CommandFlush:
                    if( hComponent == encH->compHandle ) {
                        pthread_mutex_lock( &encH->mutex );
                        if(nData2 == encH->inPortIndex)
                            encH->inPortFlushed = true;
                        if(nData2 == encH->outPortIndex)
                            encH->outPortFlushed = true;
                        if(encH->outPortFlushed && encH->inPortFlushed) {
                            encH->cmdComplete = true;
                            pthread_cond_signal( &encH->cond );
                        }
                        pthread_mutex_unlock( &encH->mutex );
                    }
                    break;

                case OMX_CommandPortDisable:
                case OMX_CommandPortEnable:
                    if( hComponent == encH->compHandle ) {
                        pthread_mutex_lock( &encH->mutex );
                        encH->cmdComplete = true;
                        pthread_cond_signal( &encH->cond );
                        pthread_mutex_unlock( &encH->mutex );
                    }
                    break;
                default:
                    // do nothing
                    break;
            }

            break;
        }

        case OMX_EventBufferFlag:
        {
            if( nData2 & OMX_BUFFERFLAG_EOS ) {
                AO_LOG( AO_LOG_INFO, "OmxilEnc=> Component detected EOS, encH=%p", encH );
                pthread_mutex_lock( &encH->mutex );
                encH->eos_received = true;
                pthread_cond_signal( &encH->cond );
                pthread_mutex_unlock( &encH->mutex );
            }

            break;
        }

        case OMX_EventPortSettingsChanged:
        {
            if( encH && hComponent == encH->compHandle ) {
                OMX_PARAM_PORTDEFINITIONTYPE portParam;
                OMX_ERRORTYPE omxErr = OMX_ErrorNone;

                AO_LOG( AO_LOG_INFO, "OmxilEnc=> port settings changed %u %u, encH=%p, hComponent=%p", nData1, nData2, encH, hComponent );
                pthread_mutex_lock( &encH->mutex );
                SET_OMX_VERSION_SIZE( portParam, sizeof(portParam) );
                portParam.nPortIndex = encH->outPortIndex;
                omxErr = OMX_GetParameter( encH->compHandle,
                        OMX_IndexParamPortDefinition,
                        &portParam);
                if( omxErr != OMX_ErrorNone ) {
                    AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d OutPort OMX_GetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
                }

                AO_LOG( AO_LOG_DEBUG1,
                          "OmxilEnc=> encH=%p, nbuffers=%u, ofsize=%u, oheight=%u, owidth=%u, nBufferCountMin=%u",
                          encH,
                          portParam.nBufferCountActual,
                          portParam.nBufferSize,
                          portParam.format.video.nFrameHeight,
                          portParam.format.video.nFrameWidth,
                          portParam.nBufferCountMin );

                pthread_cond_signal( &encH->cond );
                pthread_mutex_unlock( &encH->mutex );
            }
            break;
        }

        default:
            AO_LOG( AO_LOG_INFO, "OmxilEnc=> Event %d %u %u encH=%p", eEvent, nData1, nData2, encH);
            break;
    }

    return omxErr;

}

//------------------------------------------------------------------------------
static OMX_ERRORTYPE EmptyBufferDone(
    OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE *pBufHdr )
{
    OmxilEnc_t *encH = (OmxilEnc_t*) pAppData;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    if( encH == NULL ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> EmptyBufferDone Wrong Venc %p", encH );
        return OMX_ErrorUndefined;
    }

    if( encH->compHandle != hComponent ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> EmptyBufferDone Unknown Component %p", hComponent );
        return OMX_ErrorNone;
    }

    pthread_mutex_lock( &encH->mutex );
    encH->qInputBufHdr.push(pBufHdr);
    pthread_cond_signal( &encH->cond );
    pthread_mutex_unlock( &encH->mutex );

    return err;
}

//------------------------------------------------------------------------------
static OMX_ERRORTYPE FillBufferDone(
    OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE *pBufHdr )
{
    OmxilEnc_t *encH = (OmxilEnc_t*) pAppData;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    if( encH == NULL ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> FillBufferDone Wrong Venc %p", encH );
        return OMX_ErrorUndefined;
    }

    if( encH->compHandle != hComponent ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> FillBufferDone Unknown Component %p", hComponent );
        return OMX_ErrorNone;
    }

    pthread_mutex_lock( &encH->mutex );
    if(pBufHdr->nFilledLen > 0 && encH->out_fd != -1) {
        AO_LOG( AO_LOG_INFO, "OmxilEnc=> FillBufferDone (nFilledLen=%d, %2x %2x %2x %2x %2x)", pBufHdr->nFilledLen, pBufHdr->pBuffer[0], pBufHdr->pBuffer[1], pBufHdr->pBuffer[2], pBufHdr->pBuffer[3], pBufHdr->pBuffer[4]);
        if((write(encH->out_fd, pBufHdr->pBuffer, pBufHdr->nFilledLen)) != pBufHdr->nFilledLen)
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> FillBufferDone error when writing to file");
    }
    else {
        AO_LOG( AO_LOG_INFO, "OmxilEnc=> FillBufferDone unprocessed data(nFilledLen=%d)", pBufHdr->nFilledLen);
    }

    pBufHdr->nFilledLen = 0;
    encH->qOutputBufHdr.push(pBufHdr);

    pthread_mutex_unlock( &encH->mutex );

    return err;

}

//------------------------------------------------------------------------------
static OMX_ERRORTYPE AllocatePortBuffers( OmxilEnc_t *encH )
{
    OMX_U32 i;
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    //Allocate input port buffers
    AO_LOG( AO_LOG_INFO, "OmxilEnc=> AllocatePortBuffers allocating %u input buffers of %u size",
              encH->nInputBufs, encH->inputPortBufSize);

    pthread_mutex_lock(&encH->mutex);
    for( i = 0; i < encH->nInputBufs; i++ ) {
        // Buffers are allocated by MMF and shared with component
        OmxilEncIOBuffer_t *pBuf = &(encH->input_bufs[i]);
        OMX_BUFFERHEADERTYPE *pBufHdr = NULL;

        omxErr = OMX_AllocateBuffer( encH->compHandle,
                &pBufHdr,
                encH->inPortIndex,
                NULL,
                pBuf->size);

        if( omxErr != OMX_ErrorNone ) {
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d OMX_AllocateBuffer() returned 0x%08x", __func__, __LINE__, omxErr );
            pthread_mutex_unlock(&encH->mutex);
            return omxErr;
        } else {
            pBuf->addr = pBufHdr->pBuffer;
            pBufHdr->pAppPrivate = (OMX_PTR)pBuf->addr;
            AO_LOG( AO_LOG_INFO, "OmxilEnc=> %s:%d comp %p, port %u, bufHdr 0x%p, bufPtr 0x%p, size %u", __func__, __LINE__,
                    encH->compHandle, encH->inPortIndex, pBufHdr, pBufHdr->pBuffer, pBufHdr->nAllocLen);
            encH->qInputBufHdr.push(pBufHdr);
        }
    }

    //Allocate output port buffers
    AO_LOG( AO_LOG_INFO,
              "OmxilEnc=> AllocatePortBuffers allocating %u output buffers of %u size",
              encH->nOutputBufs, encH->outputPortBufSize);

    for( i = 0; i < encH->nOutputBufs; i++ ) {
        // Buffers are allocated by MMF and shared with component
        OmxilEncIOBuffer_t *pBuf = &(encH->output_bufs[i]);
        OMX_BUFFERHEADERTYPE *pBufHdr = NULL;

        omxErr = OMX_AllocateBuffer( encH->compHandle,
                &pBufHdr,
                encH->outPortIndex,
                NULL,
                pBuf->size);

        if( omxErr != OMX_ErrorNone ) {
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_AllocateBuffer() returned 0x%08x", __func__, __LINE__, omxErr );
            pthread_mutex_unlock(&encH->mutex);
            return omxErr;
        } else {
            pBuf->addr = pBufHdr->pBuffer;
            AO_LOG( AO_LOG_INFO, "OmxilEnc=> %s:%d Port comp %p, port %u, bufHdr 0x%p, size %u", __func__, __LINE__,
                    encH->compHandle, encH->outPortIndex, pBufHdr, encH->outputPortBufSize);
            encH->qOutputBufHdr.push(pBufHdr);
        }
    }

    pthread_mutex_unlock(&encH->mutex);
    return OMX_ErrorNone;
}

//------------------------------------------------------------------------------
static OMX_ERRORTYPE FreeOutPortBuffers( OmxilEnc_t *encH )
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufHdr = NULL;
    OMX_U32 nBufFreed = 0;

    pthread_mutex_lock(&encH->mutex);
    while(nBufFreed < encH->nOutputBufs){
        pBufHdr = encH->qOutputBufHdr.front();
        if(pBufHdr == NULL) {
            AO_LOG( AO_LOG_DEBUG1, "OmxilEnc=> FreeOutPortBuffers waiting for FillBufferDone... (%u, %u)", nBufFreed, encH->nOutputBufs );
            pthread_cond_wait( &encH->cond, &encH->mutex );
            continue;
        }
        omxErr = OMX_FreeBuffer( encH->compHandle, encH->outPortIndex, pBufHdr );
        if( omxErr != OMX_ErrorNone ) {
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d OutPort OMX_FreeBuffer() returned 0x%08x", __func__, __LINE__, omxErr );
        }
        nBufFreed ++;
        encH->qOutputBufHdr.pop();
    }
    pthread_mutex_unlock(&encH->mutex);
    return omxErr;
}

static OMX_ERRORTYPE FreeInPortBuffers( OmxilEnc_t *encH )
{
    OMX_U32 nBufFreed = 0;

    OMX_ERRORTYPE omxErr = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufHdr = NULL;
    pthread_mutex_lock(&encH->mutex);
    while( nBufFreed < encH->nInputBufs ) {
        pBufHdr = encH->qInputBufHdr.front();
        if(pBufHdr == NULL) {
            AO_LOG( AO_LOG_DEBUG1, "OmxilEnc=> FreeInPortBuffers waiting for EmptyBufferDone... (%u, %u)", nBufFreed, encH->nInputBufs );
            pthread_cond_wait( &encH->cond, &encH->mutex );
            continue;
        }

        omxErr = OMX_FreeBuffer( encH->compHandle, encH->inPortIndex, pBufHdr );
        if( omxErr != OMX_ErrorNone ) {
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d InPort OMX_FreeBuffer() returned 0x%08x", __func__, __LINE__, omxErr );
            break;
        }
        nBufFreed ++;
        encH->qInputBufHdr.pop();
    }
    pthread_mutex_unlock(&encH->mutex);

    return omxErr;
}

//------------------------------------------------------------------------------
static OMX_ERRORTYPE MoveToState( OMX_STATETYPE newState, OmxilEnc_t *encH )
{
    OMX_ERRORTYPE omxErr;
    OMX_STATETYPE currState;

    switch( newState ) {
        case OMX_StateLoaded:
        case OMX_StateIdle:
        case OMX_StatePause:
        case OMX_StateExecuting:
            break;
        default:
            return OMX_ErrorBadParameter;
    }

    // Make sure the next state is legitimate
    omxErr = OMX_GetState(encH->compHandle, &currState);
    if (omxErr != OMX_ErrorNone)
    {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Failed to get current state!!!" );
        return omxErr;
    }

    if( currState == OMX_StateInvalid ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Transition from Invalid state is not allowed!!!" );
        return OMX_ErrorInvalidState;
    }

    if( currState == newState ) {
        // State for component(s) has already been set
        return OMX_ErrorNone;
    }

    AO_LOG( AO_LOG_INFO, "OmxilEnc=> StateTransition target state %d", newState );
    encH->cmdComplete = false;
    omxErr = OMX_SendCommand( encH->compHandle, OMX_CommandStateSet, newState, NULL );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> StateTransition(IDLE) returned 0x%08x", omxErr );
        return omxErr;
    }

    if( currState == OMX_StateLoaded && newState == OMX_StateIdle ) {
        // Allocate buffers for active ports
        omxErr = AllocatePortBuffers( encH );
        if( omxErr != OMX_ErrorNone ) {
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> AllocatePortBuffers returned 0x%08x", omxErr );
            return omxErr;
        }
    }
    else if( currState == OMX_StateIdle && newState == OMX_StateLoaded ) {
        // Free buffers for active ports
        omxErr = FreeOutPortBuffers( encH );
        if( omxErr != OMX_ErrorNone ) {
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> FreeOutPortBuffers returned 0x%08x", omxErr );
            return omxErr;
        }
        omxErr = FreeInPortBuffers( encH );
        if( omxErr != OMX_ErrorNone ) {
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> FreeInPortBuffers returned 0x%08x", omxErr );
            return omxErr;
        }
    }

    if( (omxErr = waitForCommandComplete( encH )) != OMX_ErrorNone ) {
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> waitForCommandComplete returned 0x%08x", omxErr );
    }
    return omxErr;
}


//------------------------------------------------------------------------------
OMX_ERRORTYPE InitEncComp(OmxilEnc_t *encH )
{
    OMX_ERRORTYPE omxErr;
    OMX_PORT_PARAM_TYPE portParam;
    OMX_PARAM_PORTDEFINITIONTYPE inPortParam;
    OMX_PARAM_PORTDEFINITIONTYPE outPortParam;
    OMX_VENDOR_TIVPU_PARAM_TYPE vpuParam = {0};
    OMX_VENDOR_TIVPU_PARAM_TYPE vpusSetLossless = {0};
    OMX_VENDOR_TIVPU_PARAM_TYPE vpusetGOP = {0};

    // Initialize OpenMAX IL
    omxErr = OMX_Init();
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Component OMX_Init() returned 0x%08x", omxErr );
        return omxErr;
    }

    // Create the OMX component
    OMX_STRING comp_name = (OMX_STRING)QNX_ENC_COMP_NAME;
    // Obtain an instance of video decoder component
    static OMX_CALLBACKTYPE callbacks = { &EventHandler,
                                          &EmptyBufferDone,
                                          &FillBufferDone };

    omxErr = OMX_GetHandle( &encH->compHandle,
                            comp_name,
                            (OMX_PTR) encH,
                            &callbacks );
    if( !encH->compHandle || (omxErr != OMX_ErrorNone) ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Component(%s) OMX_GetHandle() returned 0x%08x", comp_name, omxErr );
        goto error_exit;
    }

    // Get number of VPU cores
    omxErr = OMX_GetParameter(encH->compHandle,
                (OMX_INDEXTYPE)OMX_VendorTIVPUConfigCoreIndex,
                &vpuParam);
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Component OMX_GetParameter() returned 0x%08x",
                  omxErr);
        goto error_exit;
    }

    vpuParam.coreIdx = encH->core_idx;

    // Set the chosen coreIdx for decode.
    omxErr = OMX_SetParameter(encH->compHandle, (OMX_INDEXTYPE)OMX_VendorTIVPUConfigCoreIndex,
                               &vpuParam);

    AO_LOG( AO_LOG_INFO, "OmxilEnc=> setLossless = %u", encH->setLossless);
    if(encH->setLossless) {
        vpusSetLossless.setLossless = 1;
        omxErr = OMX_SetParameter(encH->compHandle, (OMX_INDEXTYPE)OMX_VendorTIVPUConfigSetLossless,
                &vpusSetLossless);
    }

    if( encH->setGOP ) {
        vpusetGOP.setGOP = encH->setGOP;
        omxErr = OMX_SetParameter(encH->compHandle, (OMX_INDEXTYPE)OMX_VendorTIVPUConfigSetGOP, &vpusetGOP);
    }

    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Component OMX_SetParameter() returned 0x%08x",
                  omxErr);
        goto error_exit;
    }

    // Get component ports info and prepare internal port contexts.
    SET_OMX_VERSION_SIZE( portParam, sizeof(OMX_PORT_PARAM_TYPE) );
    portParam.nPorts = 0;
    omxErr = OMX_GetParameter( encH->compHandle,
                OMX_IndexParamVideoInit,
                &portParam );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Component OMX_GetParameter() returned 0x%08x version %u",
                  omxErr, portParam.nVersion.nVersion );
        goto error_exit;
    }

    encH->numOfPorts = portParam.nPorts;
    if(encH->numOfPorts < 2) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=>  Invalid number of ports %d", encH->numOfPorts );
        goto error_exit;
    }
    encH->inPortIndex = portParam.nStartPortNumber;
    encH->outPortIndex = portParam.nStartPortNumber + 1;

    //set frame rate
    OMX_CONFIG_FRAMERATETYPE frame_rate_t;
    SET_OMX_VERSION_SIZE( frame_rate_t, sizeof(OMX_CONFIG_FRAMERATETYPE) );
    frame_rate_t.nPortIndex = encH->outPortIndex;
    omxErr = OMX_GetConfig( encH->compHandle, OMX_IndexConfigVideoFramerate, &frame_rate_t);
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_GetConfig() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }
    frame_rate_t.xEncodeFramerate = (OMX_U32)(encH->frame_rate << 16);

    omxErr = OMX_SetConfig( encH->compHandle, OMX_IndexConfigVideoFramerate, &frame_rate_t);
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_SetConfig() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }

    //set bitrate
    OMX_VIDEO_CONFIG_BITRATETYPE bitrate_t;
    SET_OMX_VERSION_SIZE( bitrate_t, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE) );
    bitrate_t.nPortIndex = encH->outPortIndex;
    omxErr = OMX_GetConfig( encH->compHandle, OMX_IndexConfigVideoBitrate, &bitrate_t);
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_GetConfig() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }
    bitrate_t.nEncodeBitrate = encH->bitrate;

    omxErr = OMX_SetConfig( encH->compHandle, OMX_IndexConfigVideoBitrate, &bitrate_t);
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_SetConfig() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }

    //set intra period
    OMX_VIDEO_CONFIG_AVCINTRAPERIOD intra_period_t;
    SET_OMX_VERSION_SIZE( intra_period_t, sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD) );
    intra_period_t.nPortIndex = encH->outPortIndex;
    omxErr = OMX_GetConfig( encH->compHandle, OMX_IndexConfigVideoAVCIntraPeriod, &intra_period_t);
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_GetConfig() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }
    intra_period_t.nIDRPeriod = encH->idr_period;

    omxErr = OMX_SetConfig( encH->compHandle, OMX_IndexConfigVideoAVCIntraPeriod, &intra_period_t);
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_SetConfig() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }

    //set bitrate control
    OMX_VIDEO_PARAM_BITRATETYPE bitrate_ctl_t;
    SET_OMX_VERSION_SIZE( bitrate_ctl_t, sizeof(OMX_VIDEO_PARAM_BITRATETYPE) );
    bitrate_ctl_t.nPortIndex = encH->outPortIndex;
    omxErr = OMX_GetParameter( encH->compHandle,
                               OMX_IndexParamVideoBitrate,
                               &bitrate_ctl_t);
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_GetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }
    bitrate_ctl_t.eControlRate = (OMX_VIDEO_CONTROLRATETYPE)encH->rcmode;

    omxErr = OMX_SetParameter( encH->compHandle,
                               OMX_IndexParamVideoBitrate,
                               &bitrate_ctl_t);
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_SetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }

    //Configure input port
    pthread_mutex_lock(&encH->mutex);
    SET_OMX_VERSION_SIZE( inPortParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE) );
    inPortParam.nPortIndex = encH->inPortIndex;
    omxErr = OMX_GetParameter( encH->compHandle,
                               OMX_IndexParamPortDefinition,
                               &inPortParam );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_GetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }

    // Allocate input buffer for the worst case to avoid input buffer reconfiguration
    inPortParam.format.video.nFrameWidth   = encH->src_width;
    inPortParam.format.video.nFrameHeight  = encH->src_height;
    inPortParam.format.video.nStride = encH->src_stride;
    inPortParam.format.video.xFramerate    = (OMX_U32)(encH->frame_rate << 16); // FrameRate in Q16 format
    inPortParam.format.video.eColorFormat  = OMX_COLOR_FormatYUV420SemiPlanar;
    inPortParam.nBufferCountActual = VENC_INPUT_BUFFER_NUM;

    AO_LOG( AO_LOG_INFO, "OmxilEnc=> Port comp %p, port %u", encH->compHandle, inPortParam.nPortIndex );
    omxErr = OMX_SetParameter( encH->compHandle,
                               OMX_IndexParamPortDefinition,
                               &inPortParam );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_SetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }

    omxErr = OMX_GetParameter( encH->compHandle,
                               OMX_IndexParamPortDefinition,
                               &inPortParam );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_GetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }
    encH->nInputBufs = inPortParam.nBufferCountActual;
    encH->inputPortBufSize = inPortParam.nBufferSize;

    //Configure output port
    SET_OMX_VERSION_SIZE( outPortParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE) );
    outPortParam.nPortIndex = encH->outPortIndex;
    omxErr = OMX_GetParameter( encH->compHandle,
                               OMX_IndexParamPortDefinition,
                               &outPortParam );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_GetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }

    // Allow both HEVC and AVC coding standards
    if(encH->coding_std) {
        outPortParam.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMXQ_VIDEO_CodingHEVC;
    } else {
        outPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    }
    outPortParam.format.video.nFrameWidth   = encH->src_width;
    outPortParam.format.video.nFrameHeight  = encH->src_height;
    AO_LOG( AO_LOG_INFO, "OmxilEnc=> Port comp %p, port %u", encH->compHandle, outPortParam.nPortIndex );
    omxErr = OMX_SetParameter( encH->compHandle,
                               OMX_IndexParamPortDefinition,
                               &outPortParam );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_SetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }

    omxErr = OMX_GetParameter( encH->compHandle,
                               OMX_IndexParamPortDefinition,
                               &outPortParam );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_SetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }
    encH->outputPortBufSize = outPortParam.nBufferSize;
    encH->nOutputBufs = outPortParam.nBufferCountActual;

    // Allocate the output buffers and map them.
    encH->output_bufs = (OmxilEncIOBuffer_t *)malloc(encH->nOutputBufs * sizeof(OmxilEncIOBuffer_t));
    if( encH->output_bufs == NULL ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Failed to alloc memory for output_bufs Omxil Buffer descriptors", __func__, __LINE__ );
        goto error_unlock_mutex_and_exit;
    }

    for(int i = 0; i <(int)(encH->nOutputBufs); i++) {
        int buf_size = encH->outputPortBufSize;
        encH->output_bufs[i].size = buf_size;
        AO_LOG(AO_LOG_ERROR,"Get output buf size for buf[%d]: %d",i, buf_size);
    }

    if(encH->coding_std) {
        //set hevc type
        OMX_VIDEO_CODEC_PARAM_HEVCTYPE hevc;
        SET_OMX_VERSION_SIZE( hevc, sizeof(OMX_VIDEO_CODEC_PARAM_HEVCTYPE) );
        hevc.nPortIndex = encH->outPortIndex;
        omxErr = OMX_GetParameter( encH->compHandle,
                                   (OMX_INDEXTYPE)OMXQ_IndexParamVideoHevc,
                                   &hevc );
        if( omxErr != OMX_ErrorNone ) {
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_GetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
            goto error_unlock_mutex_and_exit;
        }
        hevc.eProfile = OMX_VIDEO_CODEC_HEVCProfileMain;
        hevc.eLevel = OMX_VIDEO_CODEC_HEVCLevel1;

        omxErr = OMX_SetParameter( encH->compHandle,
                                   (OMX_INDEXTYPE)OMXQ_IndexParamVideoHevc,
                                   &hevc);
    } else {
        //set avc type
        OMX_VIDEO_PARAM_AVCTYPE avc;
        SET_OMX_VERSION_SIZE( avc, sizeof(OMX_VIDEO_PARAM_AVCTYPE) );
        avc.nPortIndex = encH->outPortIndex;
        omxErr = OMX_GetParameter( encH->compHandle,
                                   OMX_IndexParamVideoAvc,
                                   &avc );
        if( omxErr != OMX_ErrorNone ) {
            AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_GetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
            goto error_unlock_mutex_and_exit;
        }
        avc.eProfile = OMX_VIDEO_AVCProfileHigh;
        avc.eLevel = OMX_VIDEO_AVCLevel1;

        omxErr = OMX_SetParameter( encH->compHandle,
                                   OMX_IndexParamVideoAvc,
                                   &avc);
    }

    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> %s:%d Port OMX_SetParameter() returned 0x%08x", __func__, __LINE__, omxErr );
        goto error_unlock_mutex_and_exit;
    }

    pthread_mutex_unlock(&encH->mutex);

    encH->compError = OMX_ErrorNone;

    //start the encoder
    omxErr = MoveToState( OMX_StateIdle, encH );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Transition LOADED->IDLE failed 0x%x", omxErr );
        goto error_unlock_mutex_and_exit;
    }

    omxErr = MoveToState( OMX_StateExecuting, encH );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Transition IDLE->EXECUTING failed 0x%x", omxErr );
        goto error_unlock_mutex_and_exit;
    }


    AO_LOG( AO_LOG_DEBUG2, "OmxilEnc=>%s video  created successfully", __func__ );
    return OMX_ErrorNone;

error_unlock_mutex_and_exit:
    pthread_mutex_unlock(&encH->mutex);
error_exit:
    OMX_Deinit();
    return omxErr;
}

//------------------------------------------------------------------------------
void CloseVenc( OmxilEnc_t *encH )
{
    OMX_ERRORTYPE omxErr;
    OMX_STATETYPE currState;
    AO_LOG( AO_LOG_DEBUG1, "OmxilEnc=> CloseVenc called encH= %p",(void *) encH );

    if( encH == NULL ) {
        AO_LOG( AO_LOG_WARNING, "OmxilEnc=> CloseVenc encH is NULL !!!" );
        return;
    }

    //wait util state change completed
    omxErr = OMX_GetState(encH->compHandle, &currState);
    if (omxErr == OMX_ErrorNone)
    {
        if( currState != OMX_StateInvalid &&
                currState != OMX_StateLoaded ) {
            StopVenc(encH);
        }
    }

    // Freeing component
    AO_LOG( AO_LOG_DEBUG1, "OmxilEnc=> Freeing component %p", encH->compHandle );
    omxErr = OMX_FreeHandle( encH->compHandle );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Component OMX_FreeHandle() returned 0x%08x", omxErr );
    }

    OMX_Deinit();

    AO_LOG( AO_LOG_INFO, "OmxilEnc=> CloseVenc Done" );

    return;
}


//------------------------------------------------------------------------------
OMX_ERRORTYPE StartVenc( OmxilEnc_t *encH )
{
    OMX_ERRORTYPE omxErr;

    omxErr = MoveToState( OMX_StateIdle, encH );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Transition LOADED->IDLE failed 0x%x", omxErr );
        return omxErr;
    }

    omxErr = MoveToState( OMX_StateExecuting, encH );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Transition IDLE->EXECUTING failed 0x%x", omxErr );
        return omxErr;
    }

    AO_LOG( AO_LOG_DEBUG1, "OmxilEnc=> Transition to EXECUTING done" );
    return OMX_ErrorNone;

}

//------------------------------------------------------------------------------
OMX_ERRORTYPE StopVenc( OmxilEnc_t *encH )
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    omxErr = MoveToState( OMX_StateIdle, encH );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> StopVenc Transition EXECUTING->IDLE failed 0x%x", omxErr );
        return omxErr;
    }

    omxErr = MoveToState( OMX_StateLoaded, encH );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> StopVenc Transition IDLE->LOADED failed 0x%x", omxErr );
        return omxErr;
    }

    return omxErr;
}

//------------------------------------------------------------------------------
OMX_ERRORTYPE PauseVenc( OmxilEnc_t *encH )
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    omxErr = MoveToState( OMX_StatePause, encH );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> PauseVenc Transition To Pause failed 0x%x", omxErr );
        return omxErr;
    }

    return omxErr;
}

//------------------------------------------------------------------------------
OMX_ERRORTYPE ResumeVenc( OmxilEnc_t *encH )
{
    OMX_ERRORTYPE omxErr = OMX_ErrorNone;

    omxErr = MoveToState( OMX_StateExecuting, encH );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> ResumeVenc Transition To Executing failed 0x%x", omxErr );
        return omxErr;
    }

    return omxErr;

}

//------------------------------------------------------------------------------
OMX_ERRORTYPE FlushVenc( OmxilEnc_t *encH )
{
    OMX_ERRORTYPE omxErr;
    OMX_STATETYPE currState;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    AO_LOG( AO_LOG_DEBUG2, "OmxilEnc=> FlushVenc In(%p)",encH );
    omxErr = OMX_GetState(encH->compHandle, &currState);
    if (omxErr != OMX_ErrorNone)
    {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> FlushVenc Failed to get current state!!!" );
        return omxErr;
    }

    if( currState == OMX_StateInvalid ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> flushing from INVALID state is not allowed!" );
        return omxErr;
    }

    if( currState == OMX_StateLoaded || currState == OMX_StateIdle) {
        AO_LOG( AO_LOG_DEBUG1, "OmxilEnc=> skip flushing due to compState=%d", currState );
        return OMX_ErrorNone;
    }

    encH->cmdComplete = false;
    encH->inPortFlushed = false;
    encH->outPortFlushed = false;
    omxErr = OMX_SendCommand( encH->compHandle, OMX_CommandFlush, OMX_ALL, NULL );
    if( omxErr != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> Send OMX_CommandFlush returned 0x%08x", omxErr );
        return omxErr;
    }
    if( (omxErr = waitForCommandComplete( encH )) != OMX_ErrorNone ) {
        AO_LOG( AO_LOG_ERROR, "OmxilEnc=> waitForCommandComplete returned 0x%08x", omxErr );
        return omxErr;
    }
    AO_LOG( AO_LOG_DEBUG2, "OmxilEnc=> FlushVenc complete");

    return err;
}

