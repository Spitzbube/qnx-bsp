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

#ifndef OMX_EXTENSION_INDEX_TI_H
#define OMX_EXTENSION_INDEX_TI_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMX_IndexExt.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/

enum {
    /**< reference: OMX_VIDEO_CODEC_PARAM_HEVCTYPE */
    OMXQ_IndexParamVideoHevc = OMX_IndexConfigVideoVp8ReferenceFrameType + 0x1,
};

enum {
    /* CnM VPU specific vendor config to identify VPU core Index */
    OMX_VendorTIVPUConfigCoreIndex = OMX_IndexVendorStartUnused + 0x1,
    OMX_VendorTIVPUConfigSetLossless, OMX_VendorTIVPUConfigSetGOP,
    OMX_VendorTIVPUConfigErrorConceal, OMX_VendorTIVPUDecBufCount
};

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/** @ingroup comp */
typedef struct OMX_VENDOR_TIVPU_PARAM_TYPE {
    OMX_U32 coreIdx;              /**< vpu coreIdx set by the app */
    OMX_U32 maxVPUCores;   /**< max cores supported by the VPU set by the component */
    OMX_U32 setLossless;   /**< setLossless parameter for the VPU set by the app*/
    OMX_U32 setGOP;             /**< GOP preset 1 or 9 (0-custom is default) */
    OMX_U32 error_conceal;      /**< temporal & spatial error concealment setting for decoder */
    OMX_U32 dec_buf_num;        /**< number of buffers dedicated for the decoder */
} OMX_VENDOR_TIVPU_PARAM_TYPE; 


/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMX_EXTENSION_INDEX_TI_H */


