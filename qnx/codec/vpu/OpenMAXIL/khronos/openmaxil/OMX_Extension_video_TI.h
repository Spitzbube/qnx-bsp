/*
 * Copyright (c) 2016 The Khronos Group Inc.
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

#ifndef OMX_EXTENSION_VIDEO_TI_H
#define OMX_EXTENSION_VIDEO_TI_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMX_Video.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
/**
 * Buffer header nFlags field extension.
 *
 * Decoder component sets the DISPLAYONLY flag to
 * indicate that the buffer is for display only
 * can not be released yet.
 * client can release the buffer if this flag is not set
 *
 */
#define OMXQ_BUFFERFLAG_DISPLAY 0x80000000

enum {
    OMXQ_VIDEO_CodingHEVC = OMX_VIDEO_CodingVendorStartUnused + 0x1, /**< QNX Extensions for HEVC format */
    OMXQ_VIDEO_CodingVP9,                                            /**< QNX Extensions for VP9 format */
};

enum {
    OMXQ_COLOR_FormatNV16 = OMX_COLOR_FormatVendorStartUnused + 0x1,
    OMXQ_COLOR_FormatP010,
    OMXQ_COLOR_FormatI420,
    OMXQ_COLOR_FormatYV12,
    OMXQ_COLOR_FormatIMC3,
    OMXQ_COLOR_Format422H,
    OMXQ_COLOR_Format422V,
    OMXQ_COLOR_Format444P,
    OMXQ_COLOR_FormatY800,
    OMXQ_COLOR_FormatYUY2,
    OMXQ_COLOR_FormatUYVY,
    OMXQ_COLOR_FormatRGBX,
    OMXQ_COLOR_FormatRGBA,
    OMXQ_COLOR_FormatBGRX,
};
/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/**
 * HEVC profile types, each profile indicates support for various
 * performance bounds and different annexes.
 */
typedef enum OMX_VIDEO_CODEC_HEVCPROFILETYPE {
    OMX_VIDEO_CODEC_HEVCProfileMain         = 0x01,   /**< Main profile */
    OMX_VIDEO_CODEC_HEVCProfileMain10       = 0x02,   /**< Main10 profile */
    OMX_VIDEO_CODEC_HEVCProfileMainStillPic = 0x04,   /**< Main Still Picture profile */
    OMX_VIDEO_CODEC_HEVCProfileMax          = 0x7FFFFFFF
} OMX_VIDEO_CODEC_HEVCPROFILETYPE;

/**
 * HEVC level types, each level indicates support for various frame sizes,
 * bit rates, decoder frame rates.
 */
typedef enum OMX_VIDEO_CODEC_HEVCLEVELTYPE {
    OMX_VIDEO_CODEC_HEVCLevel1       = 0x01,     /**< Level 1 */
    OMX_VIDEO_CODEC_HEVCLevel2       = 0x02,     /**< Level 2 */
    OMX_VIDEO_CODEC_HEVCLevel21      = 0x04,     /**< Level 2.1 */
    OMX_VIDEO_CODEC_HEVCLevel3       = 0x08,     /**< Level 3 */
    OMX_VIDEO_CODEC_HEVCLevel31      = 0x10,     /**< Level 3.1 */
    OMX_VIDEO_CODEC_HEVCLevel4Main   = 0x20,     /**< Level 4, Main tier */
    OMX_VIDEO_CODEC_HEVCLevel4High   = 0x40,     /**< Level 4, High tier */
    OMX_VIDEO_CODEC_HEVCLevel41Main  = 0x80,     /**< Level 4.1, Main tier */
    OMX_VIDEO_CODEC_HEVCLevel41High  = 0x100,    /**< Level 4.1, High tier */
    OMX_VIDEO_CODEC_HEVCLevel5Main   = 0x200,    /**< Level 5, Main tier */
    OMX_VIDEO_CODEC_HEVCLevel5High   = 0x400,    /**< Level 5, High tier */
    OMX_VIDEO_CODEC_HEVCLevel51Main  = 0x800,    /**< Level 5.1, Main tier */
    OMX_VIDEO_CODEC_HEVCLevel51High  = 0x1000,   /**< Level 5.1, High tier */
    OMX_VIDEO_CODEC_HEVCLevel52Main  = 0x2000,   /**< Level 5.2, Main tier */
    OMX_VIDEO_CODEC_HEVCLevel52High  = 0x4000,   /**< Level 5.2, High tier */
    OMX_VIDEO_CODEC_HEVCLevel6Main   = 0x8000,   /**< Level 6, Main tier */
    OMX_VIDEO_CODEC_HEVCLevel6High   = 0x10000,  /**< Level 6, High tier */
    OMX_VIDEO_CODEC_HEVCLevel61Main  = 0x20000,  /**< Level 6.1, Main tier */
    OMX_VIDEO_CODEC_HEVCLevel61High  = 0x40000,  /**< Level 6.1, High tier */
    OMX_VIDEO_CODEC_HEVCLevel62Main  = 0x80000,  /**< Level 6.2, Main tier */
    OMX_VIDEO_CODEC_HEVCLevel62High  = 0x100000, /**< Level 6.2, High tier */
    OMX_VIDEO_CODEC_HEVCLevelMax     = 0x7FFFFFFF
} OMX_VIDEO_CODEC_HEVCLEVELTYPE;

/**
 * HEVC loop filter modes
 *
 * OMX_VIDEO_CODEC_HEVCLoopFilterEnable             : Enable
 * OMX_VIDEO_CODEC_HEVCLoopFilterDisable            : Disable
 * OMX_VIDEO_CODEC_HEVCLoopFilterEnableSliceDisable : Enabled, but disable across-slice filtering
 */
typedef enum OMX_VIDEO_CODEC_HEVCLOOPFILTERTYPE {
    OMX_VIDEO_CODEC_HEVCLoopFilterEnable = 0,
    OMX_VIDEO_CODEC_HEVCLoopFilterDisable,
    OMX_VIDEO_CODEC_HEVCLoopFilterEnableSliceDisable,
    OMX_VIDEO_CODEC_HEVCLoopFilterMax    = 0x7FFFFFFF
} OMX_VIDEO_CODEC_HEVCLOOPFILTERTYPE;

/**
 * HEVC params
 *
 * STRUCT MEMBERS:
 *  nSize                     : Size of the structure in bytes
 *  nVersion                  : OMX specification version information
 *  nPortIndex                : Port that this structure applies to
 *  eProfile                  : HEVC profile(s) to use
 *  eLevel                    : HEVC level(s) to use
 *  eLoopFilterMode           : Enable/disable loop filter
 */
typedef struct OMX_VIDEO_CODEC_PARAM_HEVCTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_VIDEO_CODEC_HEVCPROFILETYPE eProfile;
    OMX_VIDEO_CODEC_HEVCLEVELTYPE eLevel;
    OMX_VIDEO_CODEC_HEVCLOOPFILTERTYPE eLoopFilterMode;
} OMX_VIDEO_CODEC_PARAM_HEVCTYPE;

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMX_EXTENSION_VIDEO_TI_H */


