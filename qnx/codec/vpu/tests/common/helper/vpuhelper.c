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
#include <string.h>
#include "main_helper.h"

#ifndef min
#define min(a,b)       (((a) < (b)) ? (a) : (b))
#endif
/*******************************************************************************
 * REPORT                                                                      *
 *******************************************************************************/
#define USER_DATA_INFO_OFFSET       (8*17)
#define FN_PIC_INFO             "dec_pic_disp_info.log"
#define FN_SEQ_INFO             "dec_seq_disp_info.log"
#define FN_PIC_TYPE             "dec_pic_type.log"
#define FN_USER_DATA            "dec_user_data.log"
#define FN_SEQ_USER_DATA        "dec_seq_user_data.log"

// VC1 specific
enum {
    BDU_SEQUENCE_END               = 0x0A,
    BDU_SLICE                      = 0x0B,
    BDU_FIELD                      = 0x0C,
    BDU_FRAME                      = 0x0D,
    BDU_ENTRYPOINT_HEADER          = 0x0E,
    BDU_SEQUENCE_HEADER            = 0x0F,
    BDU_SLICE_LEVEL_USER_DATA      = 0x1B,
    BDU_FIELD_LEVEL_USER_DATA      = 0x1C,
    BDU_FRAME_LEVEL_USER_DATA      = 0x1D,
    BDU_ENTRYPOINT_LEVEL_USER_DATA = 0x1E,
    BDU_SEQUENCE_LEVEL_USER_DATA   = 0x1F
};

// AVC specific - SEI
enum {
    SEI_REGISTERED_ITUTT35_USERDATA = 0x04,
    SEI_UNREGISTERED_USERDATA       = 0x05,
    SEI_MVC_SCALABLE_NESTING        = 0x25
};

void OpenDecReport(
    DecHandle           handle,
    VpuReportConfig_t*  cfg
    )
{
    vpu_rpt_info_t *rpt = &handle->CodecInfo->decInfo.rpt_info;
    rpt->fpPicDispInfoLogfile = NULL;
    rpt->fpPicTypeLogfile     = NULL;
    rpt->fpSeqDispInfoLogfile = NULL;
    rpt->fpUserDataLogfile    = NULL;
    rpt->fpSeqUserDataLogfile = NULL;

    rpt->decIndex           = 0;
    rpt->userDataEnable     = cfg->userDataEnable;

    rpt->reportOpened = TRUE;

    return;
}

void CloseDecReport(
    DecHandle handle
    )
{
    Uint32 core_idx = handle->coreIdx;
    vpu_rpt_info_t *rpt = &handle->CodecInfo->decInfo.rpt_info;

    if (rpt->reportOpened == FALSE) {
        return;
    }

    if (rpt->fpPicDispInfoLogfile) {
        osal_fclose(rpt->fpPicDispInfoLogfile);
        rpt->fpPicDispInfoLogfile = NULL;
    }
    if (rpt->fpPicTypeLogfile) {
        osal_fclose(rpt->fpPicTypeLogfile);
        rpt->fpPicTypeLogfile = NULL;
    }
    if (rpt->fpSeqDispInfoLogfile) {
        osal_fclose(rpt->fpSeqDispInfoLogfile);
        rpt->fpSeqDispInfoLogfile = NULL;
    }

    if (rpt->fpUserDataLogfile) {
        osal_fclose(rpt->fpUserDataLogfile);
        rpt->fpUserDataLogfile= NULL;
    }

    if (rpt->fpSeqUserDataLogfile) {
        osal_fclose(rpt->fpSeqUserDataLogfile);
        rpt->fpSeqUserDataLogfile = NULL;
    }

    if (rpt->vb_rpt.base) {
        vdi_free_dma_memory(core_idx, &rpt->vb_rpt, DEC_ETC, handle->instIndex);
    }
    rpt->decIndex = 0;

    return;
}

static void SaveUserData(
    DecHandle handle,
    BYTE*   userDataBuf
    )
{
    vpu_rpt_info_t *rpt = &handle->CodecInfo->decInfo.rpt_info;
    Uint32          i;
    Uint32          UserDataType;
    Uint32          UserDataSize;
    Uint32          userDataNum;
    Uint32          TotalSize;
    BYTE*           tmpBuf;

    if (rpt->reportOpened == FALSE) {
        return;
    }

    if(rpt->fpUserDataLogfile == 0) {
        rpt->fpUserDataLogfile = osal_fopen(FN_USER_DATA, "w+");
    }

    tmpBuf      = userDataBuf;
    userDataNum = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
    TotalSize   = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));
    tmpBuf      = userDataBuf + 8;

    for(i=0; i<userDataNum; i++) {
        UserDataType = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
        UserDataSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

        osal_fprintf(rpt->fpUserDataLogfile, "\n[Idx Type Size] : [%4d %4d %4d]",i, UserDataType, UserDataSize);

        tmpBuf += 8;
    }
    osal_fprintf(rpt->fpUserDataLogfile, "\n");

    tmpBuf = userDataBuf + USER_DATA_INFO_OFFSET;

    for(i=0; i<TotalSize; i++) {
        osal_fprintf(rpt->fpUserDataLogfile, "%02x", tmpBuf[i]);
        if ((i&7) == 7) {
            osal_fprintf(rpt->fpUserDataLogfile, "\n");
        }
    }
    osal_fprintf(rpt->fpUserDataLogfile, "\n");

    osal_fflush(rpt->fpUserDataLogfile);
}


static void SaveUserDataINT(
    DecHandle           handle,
    BYTE*   userDataBuf,
    Int32   size,
    Int32   intIssued,
    Int32   decIdx,
    CodStd  bitstreamFormat
    )
{
    vpu_rpt_info_t *rpt = &handle->CodecInfo->decInfo.rpt_info;
    Int32           i;
    Int32           UserDataType = 0;
    Int32           UserDataSize = 0;
    Int32           userDataNum = 0;
    Int32           TotalSize;
    BYTE*           tmpBuf;
    BYTE*           backupBufTmp;
    static Int32    backupSize = 0;
    static BYTE*    backupBuf  = NULL;

    if (rpt->reportOpened == FALSE) {
        return;
    }

    if(rpt->fpUserDataLogfile == NULL) {
        rpt->fpUserDataLogfile = osal_fopen(FN_USER_DATA, "w+");
    }

    backupBufTmp = (BYTE *)osal_malloc(backupSize + size);

    if (backupBufTmp == 0) {
        VLOG( ERR, "Can't mem allock\n");
        return;
    }

    for (i=0; i<backupSize; i++) {
        backupBufTmp[i] = backupBuf[i];
    }
    if (backupBuf != NULL) {
        osal_free(backupBuf);
    }
    backupBuf = backupBufTmp;

    tmpBuf = userDataBuf + USER_DATA_INFO_OFFSET;
    size -= USER_DATA_INFO_OFFSET;

    for(i=0; i<size; i++) {
        backupBuf[backupSize + i] = tmpBuf[i];
    }

    backupSize += size;

    if (intIssued) {
        return;
    }

    tmpBuf = userDataBuf;
    userDataNum = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
    if(userDataNum == 0) {
        return;
    }

    tmpBuf = userDataBuf + 8;
    UserDataSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

    UserDataSize = (UserDataSize+7)/8*8;
    osal_fprintf(rpt->fpUserDataLogfile, "FRAME [%1d]\n", decIdx);

    for(i=0; i<backupSize; i++) {
        osal_fprintf(rpt->fpUserDataLogfile, "%02x", backupBuf[i]);
        if ((i&7) == 7) {
            osal_fprintf(rpt->fpUserDataLogfile, "\n");
        }

        if( (i%8==7) && (i==UserDataSize-1) && (UserDataSize != backupSize)) {
            osal_fprintf(rpt->fpUserDataLogfile, "\n");
            tmpBuf+=8;
            UserDataSize += (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));
            UserDataSize = (UserDataSize+7)/8*8;
        }
    }
    if (backupSize > 0) {
        osal_fprintf(rpt->fpUserDataLogfile, "\n");
    }

    tmpBuf = userDataBuf;
    userDataNum = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
    TotalSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

    osal_fprintf(rpt->fpUserDataLogfile, "User Data Num: [%d]\n", userDataNum);
    osal_fprintf(rpt->fpUserDataLogfile, "User Data Total Size: [%d]\n", TotalSize);

    tmpBuf = userDataBuf + 8;
    for(i=0; i<userDataNum; i++) {
        UserDataType = (short)((tmpBuf[0]<<8) | (tmpBuf[1]<<0));
        UserDataSize = (short)((tmpBuf[2]<<8) | (tmpBuf[3]<<0));

        if(bitstreamFormat == STD_VC1) {
            switch (UserDataType) {

            case BDU_SLICE_LEVEL_USER_DATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "BDU_SLICE_LEVEL_USER_DATA");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case BDU_FIELD_LEVEL_USER_DATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "BDU_FIELD_LEVEL_USER_DATA");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case BDU_FRAME_LEVEL_USER_DATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "BDU_FRAME_LEVEL_USER_DATA");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case BDU_ENTRYPOINT_LEVEL_USER_DATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "BDU_ENTRYPOINT_LEVEL_USER_DATA");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case BDU_SEQUENCE_LEVEL_USER_DATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "BDU_SEQUENCE_LEVEL_USER_DATA");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            }
        }
        else if(bitstreamFormat == STD_AVC) {
            switch (UserDataType) {
            case SEI_REGISTERED_ITUTT35_USERDATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "registered_itu_t_t35");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            case SEI_UNREGISTERED_USERDATA:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "unregistered");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;

            case SEI_MVC_SCALABLE_NESTING:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "mvc_scalable_nesting");
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
                break;
            }
        }
        else if(bitstreamFormat == STD_MPEG2) {

            switch (UserDataType) {
            case 0:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Seq]\n", i);
                break;
            case 1:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Gop]\n", i);
                break;
            case 2:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Pic]\n", i);
                break;
            default:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Error]\n", i);
                break;
            }
            osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
        }
        else if(bitstreamFormat == STD_AVS) {
            osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:%s]\n", i, "User Data");
            osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
        }
        else {
            switch (UserDataType) {
            case 0:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Vos]\n", i);
                break;
            case 1:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Vis]\n", i);
                break;
            case 2:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Vol]\n", i);
                break;
            case 3:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Gov]\n", i);
                break;
            default:
                osal_fprintf(rpt->fpUserDataLogfile, "User Data Type: [%d:Error]\n", i);
                break;
            }
            osal_fprintf(rpt->fpUserDataLogfile, "User Data Size: [%d]\n", UserDataSize);
        }

        tmpBuf += 8;
    }
    osal_fprintf(rpt->fpUserDataLogfile, "\n");
    osal_fflush(rpt->fpUserDataLogfile);

    backupSize = 0;
    if (backupBuf != NULL) {
        osal_free(backupBuf);
    }

    backupBuf = 0;
}

void ConfigDecReport(
    Uint32      core_idx,
    DecHandle   handle,
    CodStd      bitstreamFormat
    )
{
    vpu_rpt_info_t *rpt = &handle->CodecInfo->decInfo.rpt_info;

    if (rpt->reportOpened == FALSE) {
        return;
    }

    // Report Information
    if (!rpt->vb_rpt.base) {
        rpt->vb_rpt.size     = SIZE_REPORT_BUF;
        if (vdi_allocate_dma_memory(core_idx, &rpt->vb_rpt, DEC_ETC, handle->instIndex) < 0) {
            VLOG(ERR, "fail to allocate report  buffer\n" );
            return;
        }
    }

    VPU_DecGiveCommand(handle, SET_ADDR_REP_USERDATA,    &rpt->vb_rpt.phys_addr );
    VPU_DecGiveCommand(handle, SET_SIZE_REP_USERDATA,    &rpt->vb_rpt.size );

    if (rpt->userDataEnable == TRUE) {
        VPU_DecGiveCommand( handle, ENABLE_REP_USERDATA, 0 );
    }
    else {
        VPU_DecGiveCommand( handle, DISABLE_REP_USERDATA, 0 );
    }
}

void SaveDecReport(
    Uint32          core_idx,
    DecHandle       handle,
    DecOutputInfo*  pDecInfo,
    CodStd          bitstreamFormat,
    Uint32          mbNumX,
    Uint32          mbNumY
    )
{
    vpu_rpt_info_t *rpt = &handle->CodecInfo->decInfo.rpt_info;

    if (rpt->reportOpened == FALSE) {
        return ;
    }

    // Report Information

    // User Data
    if ((pDecInfo->indexFrameDecoded >= 0 || (bitstreamFormat == STD_VC1))  &&
        rpt->userDataEnable == TRUE &&
        pDecInfo->decOutputExtData.userDataSize > 0) {
        // Vc1 Frame user data follow picture. After last frame decoding, user data should be reported.
        Uint32 size        = 0;
        BYTE*  userDataBuf = NULL;

        if (pDecInfo->decOutputExtData.userDataBufFull == TRUE) {
            VLOG(ERR, "User Data Buffer is Full\n");
        }

        size = (pDecInfo->decOutputExtData.userDataSize+7)/8*8 + USER_DATA_INFO_OFFSET;
        userDataBuf = (BYTE*)osal_malloc(size);
        osal_memset(userDataBuf, 0, size);

        vdi_read_memory(core_idx, rpt->vb_rpt.phys_addr, userDataBuf, size, HOST_ENDIAN);
        if (pDecInfo->indexFrameDecoded >= 0) {
            SaveUserData(handle, userDataBuf);
        }
        osal_free(userDataBuf);
    }

    if (((pDecInfo->indexFrameDecoded >= 0 || (bitstreamFormat == STD_VC1 )) && rpt->userDataEnable) || // Vc1 Frame user data follow picture. After last frame decoding, user data should be reported.
        (pDecInfo->indexFrameDisplay >= 0 && rpt->userDataEnable) ) {
        Uint32 size        = 0;
        Uint32 dataSize    = 0;
        BYTE*  userDataBuf = NULL;

        if (pDecInfo->decOutputExtData.userDataBufFull) {
            VLOG(ERR, "User Data Buffer is Full\n");
        }

        dataSize = pDecInfo->decOutputExtData.userDataSize % rpt->vb_rpt.size;
        if (dataSize == 0 && pDecInfo->decOutputExtData.userDataSize != 0)	{
            dataSize = rpt->vb_rpt.size;
        }

        size = (dataSize+7)/8*8 + USER_DATA_INFO_OFFSET;
        userDataBuf = (BYTE*)osal_malloc(size);
        osal_memset(userDataBuf, 0, size);
        vdi_read_memory(core_idx, rpt->vb_rpt.phys_addr, userDataBuf, size, HOST_ENDIAN);
        if (pDecInfo->indexFrameDecoded >= 0 || (bitstreamFormat == STD_VC1)) {
            SaveUserDataINT(handle, userDataBuf, size, 0, rpt->decIndex, bitstreamFormat);
        }
        osal_free(userDataBuf);
    }

    if (pDecInfo->indexFrameDecoded >= 0) {
        if (rpt->fpPicTypeLogfile == NULL) {
            rpt->fpPicTypeLogfile = osal_fopen(FN_PIC_TYPE, "w+");
        }
        osal_fprintf(rpt->fpPicTypeLogfile, "FRAME [%1d]\n", rpt->decIndex);

        switch (bitstreamFormat) {
        case STD_AVC:
            if(pDecInfo->pictureStructure == 3) {	// FIELD_INTERLACED
                osal_fprintf(rpt->fpPicTypeLogfile, "Top Field Type: [%s]\n", pDecInfo->picTypeFirst == 0 ? "I_TYPE" :
                    (pDecInfo->picTypeFirst) == 1 ? "P_TYPE" :
                    (pDecInfo->picTypeFirst) == 2 ? "BI_TYPE" :
                    (pDecInfo->picTypeFirst) == 3 ? "B_TYPE" :
                    (pDecInfo->picTypeFirst) == 4 ? "SKIP_TYPE" :
                    (pDecInfo->picTypeFirst) == 5 ? "IDR_TYPE" :
                    "FORBIDDEN");

                osal_fprintf(rpt->fpPicTypeLogfile, "Bottom Field Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" :
                    (pDecInfo->picType) == 1 ? "P_TYPE" :
                    (pDecInfo->picType) == 2 ? "BI_TYPE" :
                    (pDecInfo->picType) == 3 ? "B_TYPE" :
                    (pDecInfo->picType) == 4 ? "SKIP_TYPE" :
                    (pDecInfo->picType) == 5 ? "IDR_TYPE" :
                    "FORBIDDEN");
            }
            else {
                osal_fprintf(rpt->fpPicTypeLogfile, "Picture Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" :
                    (pDecInfo->picType) == 1 ? "P_TYPE" :
                    (pDecInfo->picType) == 2 ? "BI_TYPE" :
                    (pDecInfo->picType) == 3 ? "B_TYPE" :
                    (pDecInfo->picType) == 4 ? "SKIP_TYPE" :
                    (pDecInfo->picType) == 5 ? "IDR_TYPE" :
                    "FORBIDDEN");
            }
            break;
        case STD_MPEG2 :
            osal_fprintf(rpt->fpPicTypeLogfile, "Picture Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" :
                pDecInfo->picType == 1 ? "P_TYPE" :
                pDecInfo->picType == 2 ? "B_TYPE" :
                "D_TYPE");
            break;
        case STD_MPEG4 :
            osal_fprintf(rpt->fpPicTypeLogfile, "Picture Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" :
                pDecInfo->picType == 1 ? "P_TYPE" :
                pDecInfo->picType == 2 ? "B_TYPE" :
                "S_TYPE");
            break;
        case STD_VC1:
            if(pDecInfo->pictureStructure == 3) {	// FIELD_INTERLACED
                osal_fprintf(rpt->fpPicTypeLogfile, "Top Field Type: [%s]\n", pDecInfo->picTypeFirst == 0 ? "I_TYPE" :
                    (pDecInfo->picTypeFirst) == 1 ? "P_TYPE" :
                    (pDecInfo->picTypeFirst) == 2 ? "BI_TYPE" :
                    (pDecInfo->picTypeFirst) == 3 ? "B_TYPE" :
                    (pDecInfo->picTypeFirst) == 4 ? "SKIP_TYPE" :
                    "FORBIDDEN");

            osal_fprintf(rpt->fpPicTypeLogfile, "Bottom Field Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" :
                (pDecInfo->picType) == 1 ? "P_TYPE" :
                (pDecInfo->picType) == 2 ? "BI_TYPE" :
                (pDecInfo->picType) == 3 ? "B_TYPE" :
                (pDecInfo->picType) == 4 ? "SKIP_TYPE" :
                "FORBIDDEN");
            }
            else {
                osal_fprintf(rpt->fpPicTypeLogfile, "Picture Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" :
                    (pDecInfo->picTypeFirst) == 1 ? "P_TYPE" :
                    (pDecInfo->picTypeFirst) == 2 ? "BI_TYPE" :
                    (pDecInfo->picTypeFirst) == 3 ? "B_TYPE" :
                    (pDecInfo->picTypeFirst) == 4 ? "SKIP_TYPE" :
                    "FORBIDDEN");
            }
            break;
        default:
            osal_fprintf(rpt->fpPicTypeLogfile, "Picture Type: [%s]\n", pDecInfo->picType == 0 ? "I_TYPE" :
                pDecInfo->picType == 1 ? "P_TYPE" :
                "B_TYPE");
            break;
        }
    }

    if (pDecInfo->indexFrameDecoded >= 0) {
        if (rpt->fpPicDispInfoLogfile == NULL) {
            rpt->fpPicDispInfoLogfile = osal_fopen(FN_PIC_INFO, "w+");
        }
        osal_fprintf(rpt->fpPicDispInfoLogfile, "FRAME [%1d]\n", rpt->decIndex);

        switch (bitstreamFormat) {
        case STD_MPEG2:
            osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n",
                pDecInfo->picType == 0 ? "I_TYPE" :
                pDecInfo->picType == 1 ? "P_TYPE" :
                pDecInfo->picType == 2 ? "B_TYPE" :
                "D_TYPE");
            break;
        case STD_MPEG4:
            osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n",
                pDecInfo->picType == 0 ? "I_TYPE" :
                pDecInfo->picType == 1 ? "P_TYPE" :
                pDecInfo->picType == 2 ? "B_TYPE" :
                "S_TYPE");
            break;
        case STD_VC1 :
            if(pDecInfo->pictureStructure == 3) {	// FIELD_INTERLACED
                osal_fprintf(rpt->fpPicDispInfoLogfile, "Top : %s\n", (pDecInfo->picType>>3) == 0 ? "I_TYPE" :
                    (pDecInfo->picType>>3) == 1 ? "P_TYPE" :
                    (pDecInfo->picType>>3) == 2 ? "BI_TYPE" :
                    (pDecInfo->picType>>3) == 3 ? "B_TYPE" :
                    (pDecInfo->picType>>3) == 4 ? "SKIP_TYPE" :
                    "FORBIDDEN");

                osal_fprintf(rpt->fpPicDispInfoLogfile, "Bottom : %s\n", (pDecInfo->picType&0x7) == 0 ? "I_TYPE" :
                    (pDecInfo->picType&0x7) == 1 ? "P_TYPE" :
                    (pDecInfo->picType&0x7) == 2 ? "BI_TYPE" :
                    (pDecInfo->picType&0x7) == 3 ? "B_TYPE" :
                    (pDecInfo->picType&0x7) == 4 ? "SKIP_TYPE" :
                    "FORBIDDEN");

                osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Interlaced Picture");
            }
            else {
                osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", (pDecInfo->picType>>3) == 0 ? "I_TYPE" :
                    (pDecInfo->picType>>3) == 1 ? "P_TYPE" :
                    (pDecInfo->picType>>3) == 2 ? "BI_TYPE" :
                    (pDecInfo->picType>>3) == 3 ? "B_TYPE" :
                    (pDecInfo->picType>>3) == 4 ? "SKIP_TYPE" :
                    "FORBIDDEN");

                osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Frame Picture");
            }
            break;
        default:
            osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n",
                         pDecInfo->picType == 0 ? "I_TYPE" :
                         pDecInfo->picType == 1 ? "P_TYPE" :
                         "B_TYPE");
            break;
        }

        if(bitstreamFormat != STD_VC1) {
            if (pDecInfo->interlacedFrame) {
                if(bitstreamFormat == STD_AVS) {
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Frame Picture");
                }
                else {
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Interlaced Picture");
                }
            }
            else {
                if(bitstreamFormat == STD_AVS) {
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Interlaced Picture");
                }
                else {
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Frame Picture");
                }
            }
        }

        if (bitstreamFormat != STD_RV) {
            if(bitstreamFormat == STD_VC1) {
                switch(pDecInfo->pictureStructure) {
                case 0:  osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "PROGRESSIVE");	break;
                case 2:  osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FRAME_INTERLACE"); break;
                case 3:  osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FIELD_INTERLACE");	break;
                default: osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FORBIDDEN"); break;
                }
            }
            else if(bitstreamFormat == STD_AVC) {
                if(!pDecInfo->interlacedFrame) {
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FRAME_PICTURE");
                }
                else {
                    if(pDecInfo->topFieldFirst) {
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Top Field First");
                    }
                    else {
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Bottom Field First");
                    }
                }
            }
            else if (bitstreamFormat != STD_MPEG4 && bitstreamFormat != STD_AVS) {
                switch (pDecInfo->pictureStructure) {
                case 1:  osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "TOP_FIELD");	break;
                case 2:  osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "BOTTOM_FIELD"); break;
                case 3:  osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FRAME_PICTURE");	break;
                default: osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "FORBIDDEN"); break;
                }
            }

            if(bitstreamFormat != STD_AVC) {
                if (pDecInfo->topFieldFirst) {
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Top Field First");
                }
                else {
                    osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Bottom Field First");
                }

                if (bitstreamFormat != STD_MPEG4) {
                    if (pDecInfo->repeatFirstField) {
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Repeat First Field");
                    }
                    else {
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Not Repeat First Field");
                    }

                    if (bitstreamFormat == STD_VC1) {
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "VC1 RPTFRM [%1d]\n", pDecInfo->progressiveFrame);
                    }
                    else if (pDecInfo->progressiveFrame) {
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Progressive Frame");
                    }
                    else {
                        osal_fprintf(rpt->fpPicDispInfoLogfile, "%s\n", "Interlaced Frame");
                    }
                }
            }
        }

        if (bitstreamFormat == STD_MPEG2) {
            osal_fprintf(rpt->fpPicDispInfoLogfile, "Field Sequence [%d]\n\n", pDecInfo->fieldSequence);
        }
        else {
            osal_fprintf(rpt->fpPicDispInfoLogfile, "\n");
        }

        osal_fflush(rpt->fpPicDispInfoLogfile);
    }

    if(pDecInfo->indexFrameDecoded >= 0) {
        rpt->decIndex ++;
    }

    return;
}

int setWaveEncOpenParam(EncOpenParam *pEncOP, TestEncConfig *pEncConfig, ENC_CFG *pCfg)
{
    Int32   i = 0;
    Int32   srcWidth;
    Int32   srcHeight;
    Int32   outputNum;
    Int32   bitrate;
    Int32   bitrateBL;

    EncWaveParam *param = &pEncOP->EncStdParam.waveParam;

    srcWidth  = (pEncConfig->picWidth > 0)  ? pEncConfig->picWidth  : pCfg->waveCfg.picX;
    srcHeight = (pEncConfig->picHeight > 0) ? pEncConfig->picHeight : pCfg->waveCfg.picY;
    if(pCfg->waveCfg.enStillPicture) {
        outputNum   = 1;
    }
    else {
        if ( pEncConfig->outNum != 0 ) {
            outputNum = pEncConfig->outNum;
        }
        else {
            outputNum = pCfg->NumFrame;
        }
    }
    bitrate   = (pEncConfig->kbps > 0)      ? pEncConfig->kbps*1024 : pCfg->RcBitRate;
    bitrateBL = (pEncConfig->kbps > 0)      ? pEncConfig->kbps*1024 : pCfg->RcBitRateBL;

    pEncConfig->outNum      = outputNum;
    pEncOP->picWidth        = srcWidth;
    pEncOP->picHeight       = srcHeight;
    pEncOP->frameRateInfo   = pCfg->waveCfg.frameRate;

    param->level            = 0;
    param->tier             = 0;
    pEncOP->srcBitDepth     = pCfg->SrcBitDepth;

    if (pCfg->waveCfg.internalBitDepth == 0)
        param->internalBitDepth = pCfg->SrcBitDepth;
    else
        param->internalBitDepth = pCfg->waveCfg.internalBitDepth;

    if ( param->internalBitDepth == 10 )
        pEncOP->outputFormat  = FORMAT_420_P10_16BIT_MSB;
    if ( param->internalBitDepth == 8 )
        pEncOP->outputFormat  = FORMAT_420;


    if (pEncOP->bitstreamFormat == STD_HEVC) {
        if(pCfg->waveCfg.enStillPicture) {
            if (param->internalBitDepth > 8)
                param->profile   = HEVC_PROFILE_MAIN10_STILLPICTURE;
            else
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
    pEncOP->rcEnable             = pCfg->RcEnable;
    pEncOP->vbvBufferSize        = pCfg->VbvBufferSize;
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
    pEncOP->bitRate          = bitrate;
    pEncOP->bitRateBL        = bitrateBL;

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
    param->numUnitsInTick       = pCfg->waveCfg.numUnitsInTick;
    param->timeScale            = pCfg->waveCfg.timeScale;
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
    param->entropyCodingMode    = pCfg->waveCfg.entropyCodingMode;;

    return 1;
}

/******************************************************************************
EncOpenParam Initialization
******************************************************************************/
/**
* To init EncOpenParam by runtime evaluation
* IN
*   EncConfigParam *pEncConfig
* OUT
*   EncOpenParam *pEncOP
*/
#define DEFAULT_ENC_OUTPUT_NUM      30
Int32 GetEncOpenParamDefault(EncOpenParam *pEncOP, TestEncConfig *pEncConfig)
{
    int bitFormat;
    Int32   productId;
    productId                       = VPU_GetProductId(pEncOP->coreIdx);
    pEncConfig->outNum              = pEncConfig->outNum == 0 ? DEFAULT_ENC_OUTPUT_NUM : pEncConfig->outNum;
    bitFormat                       = pEncOP->bitstreamFormat;

    pEncOP->picWidth                = pEncConfig->picWidth;
    pEncOP->picHeight               = pEncConfig->picHeight;
    pEncOP->frameRateInfo           = 30;
    pEncOP->bitRate                 = pEncConfig->kbps;
    pEncOP->vbvBufferSize           = 0;        // 0 = ignore
    pEncOP->meBlkMode               = 0;        // for compare with C-model ( C-model = only 0 )
    pEncConfig->picQpY              = 23;

    // Standard specific
    if( bitFormat == STD_HEVC || (bitFormat == STD_AVC && productId == PRODUCT_ID_521)) {
        EncWaveParam *param = &pEncOP->EncStdParam.waveParam;
        Int32   rcBitrate   = pEncConfig->kbps * 1000;
        Int32   i           = 0;

        pEncOP->bitRate         = rcBitrate;
        param->profile          = HEVC_PROFILE_MAIN;

        param->level            = 0;
        param->tier             = 0;
        param->internalBitDepth = 8;
        pEncOP->srcBitDepth     = 8;
        param->losslessEnable   = 0;
        param->constIntraPredFlag = 0;
        param->useLongTerm  = 0;

        /* for CMD_ENC_SEQ_GOP_PARAM */
        param->gopPresetIdx     = PRESET_IDX_IBBBP;

        /* for CMD_ENC_SEQ_INTRA_PARAM */
        param->decodingRefreshType   = ((param->gopPresetIdx == 1) && (pEncOP->rcEnable == 1) && (bitFormat == STD_HEVC)) ? 2 : 1;
        param->intraPeriod           = ((param->gopPresetIdx == 1) && (pEncOP->rcEnable == 1) && (bitFormat == STD_HEVC)) ? 1 : 28;
        param->intraQP               = 0;
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

        pEncConfig->roi_enable = 0;
        /* for CMD_ENC_PARAM */
        if (param->useRecommendEncParam != 1) {		// 0 : Custom,  2 : Boost mode (normal encoding speed, normal picture quality),  3 : Fast mode (high encoding speed, low picture quality)
            param->scalingListEnable        = 0;
            param->cuSizeMode               = 0x7;
            param->tmvpEnable               = 1;
            param->wppEnable                = 0;
            param->maxNumMerge              = 2;
            param->disableDeblk             = 0;
            param->lfCrossSliceBoundaryEnable   = 1;
            param->betaOffsetDiv2           = 0;
            param->tcOffsetDiv2             = 0;
            param->skipIntraTrans           = 1;
            param->saoEnable                = 1;
            param->intraNxNEnable           = 1;
        }

        /* for CMD_ENC_RC_PARAM */
        pEncOP->rcEnable             = rcBitrate == 0 ? FALSE : TRUE;
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
        param->timeScale            =  pEncOP->frameRateInfo * 1000;
        param->numUnitsInTick       =  1000;

        param->chromaCbQpOffset = 0;
        param->chromaCrQpOffset = 0;
        param->initialRcQp      = 63;       // 63 is meaningless.
        param->nrYEnable        = 0;
        param->nrCbEnable       = 0;
        param->nrCrEnable       = 0;
        param->nrNoiseEstEnable = 0;

        pEncConfig->roi_avg_qp             = 0;
        pEncConfig->lambda_map_enable      = 0;

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
        param->disableDeblk                = 0;
    }
    else {
        VLOG(ERR, "Invalid codec standard mode: bitFormat(%d) \n", bitFormat);
        return 0;
    }


    return 1;
}

/**
* To init EncOpenParam by CFG file
* IN
*   EncConfigParam *pEncConfig
* OUT
*   EncOpenParam *pEncOP
*   char *srcYuvFileName
*/
Int32 GetEncOpenParam(EncOpenParam *pEncOP, TestEncConfig *pEncConfig, ENC_CFG *pEncCfg)
{
    int bitFormat;
    ENC_CFG encCfgInst;
    ENC_CFG *pCfg;
    Int32   productId;

    productId   = VPU_GetProductId(pEncOP->coreIdx);

    // Source YUV Image File to load
    if (pEncCfg) {
        pCfg = pEncCfg;
    }
    else {
        osal_memset( &encCfgInst, 0x00, sizeof(ENC_CFG));
        pCfg = &encCfgInst;
    }
    bitFormat = pEncOP->bitstreamFormat;

    if ( PRODUCT_ID_W_SERIES(productId) == TRUE ) {
        // for WAVE
        switch(bitFormat)
        {
        case STD_HEVC:
        case STD_AVC:
            if (parseWaveEncCfgFile(pCfg, pEncConfig->cfgFileName, bitFormat) == 0)
                return 0;
            strcpy(pEncConfig->yuvFileName, pCfg->SrcFileName);

            if (pCfg->waveCfg.roiEnable) {
                strcpy(pEncConfig->roi_file_name, pCfg->waveCfg.roiFileName);
                if (!strcmp(pCfg->waveCfg.roiQpMapFile, "0") || pCfg->waveCfg.roiQpMapFile[0] == 0) {
                    //invalid value exist or not exist
                }
                else {
                    //valid value exist
                    strcpy(pEncConfig->roi_file_name, pCfg->waveCfg.roiQpMapFile);
                }
            }

            pEncConfig->roi_enable  = pCfg->waveCfg.roiEnable;

            if (pCfg->waveCfg.prefixSeiEnable)
                strcpy(pEncConfig->prefix_sei_nal_file_name, pCfg->waveCfg.prefixSeiDataFileName);
            pEncConfig->seiDataEnc.prefixSeiNalEnable       = pCfg->waveCfg.prefixSeiEnable;
            pEncConfig->seiDataEnc.prefixSeiDataSize        = pCfg->waveCfg.prefixSeiDataSize;

            if (pCfg->waveCfg.suffixSeiEnable)
                strcpy(pEncConfig->suffix_sei_nal_file_name, pCfg->waveCfg.suffixSeiDataFileName);
            pEncConfig->seiDataEnc.suffixSeiNalEnable       = pCfg->waveCfg.suffixSeiEnable;
            pEncConfig->seiDataEnc.suffixSeiDataSize        = pCfg->waveCfg.suffixSeiDataSize;

            if (pCfg->waveCfg.hrdInVPS)
                strcpy(pEncConfig->hrd_rbsp_file_name, pCfg->waveCfg.hrdDataFileName);
            if (pCfg->waveCfg.vuiDataEnable)
                strcpy(pEncConfig->vui_rbsp_file_name, pCfg->waveCfg.vuiDataFileName);


            pEncConfig->encAUD  = pCfg->waveCfg.encAUD;
            pEncConfig->encEOS  = pCfg->waveCfg.encEOS;
            pEncConfig->encEOB  = pCfg->waveCfg.encEOB;
            pEncConfig->useAsLongtermPeriod = pCfg->waveCfg.useAsLongtermPeriod;
            pEncConfig->refLongtermPeriod   = pCfg->waveCfg.refLongtermPeriod;

            pEncConfig->roi_avg_qp          = pCfg->waveCfg.roiAvgQp;
            pEncConfig->lambda_map_enable   = pCfg->waveCfg.customLambdaMapEnable;
            pEncConfig->mode_map_flag       = pCfg->waveCfg.customModeMapFlag;
            pEncConfig->wp_param_flag       = pCfg->waveCfg.weightPredEnable;
            pEncConfig->forceIdrPicIdx      = pCfg->waveCfg.forceIdrPicIdx;
            if (pCfg->waveCfg.scalingListEnable)
                strcpy(pEncConfig->scaling_list_fileName, pCfg->waveCfg.scalingListFileName);

            if (pCfg->waveCfg.customLambdaEnable)
                strcpy(pEncConfig->custom_lambda_fileName, pCfg->waveCfg.customLambdaFileName);

            // custom map
            if (pCfg->waveCfg.customLambdaMapEnable)
                strcpy(pEncConfig->lambda_map_fileName, pCfg->waveCfg.customLambdaMapFileName);

            if (pCfg->waveCfg.customModeMapFlag)
                strcpy(pEncConfig->mode_map_fileName, pCfg->waveCfg.customModeMapFileName);

            if (pCfg->waveCfg.weightPredEnable&1)
                strcpy(pEncConfig->wp_param_fileName, pCfg->waveCfg.WpParamFileName);

            pEncConfig->force_picskip_start =   pCfg->waveCfg.forcePicSkipStart;
            pEncConfig->force_picskip_end   =   pCfg->waveCfg.forcePicSkipEnd;
            pEncConfig->force_coefdrop_start=   pCfg->waveCfg.forceCoefDropStart;
            pEncConfig->force_coefdrop_end  =   pCfg->waveCfg.forceCoefDropEnd;

            break;
        default :
            break;
        }
    }

    if (bitFormat == STD_HEVC || (bitFormat == STD_AVC && productId == PRODUCT_ID_521)) {
        if (setWaveEncOpenParam(pEncOP, pEncConfig, pCfg) == 0)
            return 0;
    }

    return 1;
}
