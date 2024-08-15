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

#include "vdi_debug.h"
#include "vdi_osal.h"
#include "vpuapifunc.h"
#include "wave5_regdefine.h"

static void make_log(unsigned long instIdx, const char *str, int step)
{ //lint !e578
    if (step == 1)
        codec_sloginfo("\n**%s start(%lu)\n", str, instIdx);
    else if (step == 2) //
        codec_sloginfo("\n**%s timeout(%lu)\n", str, instIdx);
    else
        codec_sloginfo("\n**%s end(%lu)\n", str, instIdx);
}

void vdi_log(unsigned long coreIdx, unsigned long instIdx, int cmd, int step)
{ //lint !e578
    int i;
    int productId;

    codec_slogtrace("%s", __func__);
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return ;

    productId = VPU_GetProductId(coreIdx);

    if (PRODUCT_ID_W_SERIES(productId))
    {
        switch(cmd)
        {
        case W5_INIT_VPU:
            make_log(instIdx, "INIT_VPU", step);
            break;
        case W5_ENC_SET_PARAM:
            make_log(instIdx, "ENC_SET_PARAM", step);
            break;
        case W5_INIT_SEQ:
            make_log(instIdx, "DEC INIT_SEQ", step);
            break;
        case W5_DESTROY_INSTANCE:
            make_log(instIdx, "DESTROY_INSTANCE", step);
            break;
        case W5_DEC_PIC://ENC_PIC for ENC
            make_log(instIdx, "DEC_PIC(ENC_PIC)", step);
            break;
        case W5_SET_FB:
            make_log(instIdx, "SET_FRAMEBUF", step);
            break;
        case W5_FLUSH_INSTANCE:
            make_log(instIdx, "FLUSH INSTANCE", step);
            break;
        case W5_QUERY:
            make_log(instIdx, "QUERY", step);
            break;
        case W5_SLEEP_VPU:
            make_log(instIdx, "SLEEP_VPU", step);
            break;
        case W5_WAKEUP_VPU:
            make_log(instIdx, "WAKEUP_VPU", step);
            break;
        case W5_UPDATE_BS:
            make_log(instIdx, "UPDATE_BS", step);
            break;
        case W5_CREATE_INSTANCE:
            make_log(instIdx, "CREATE_INSTANCE", step);
            break;
        default:
            make_log(instIdx, "ANY_CMD", step);
            break;
        }
    }
    else {
        codec_slogerr("Unknown product id : %08x\n", productId);
        return;
    }

    for (i=0x0; i<0x200; i=i+16) { // host IF register 0x100 ~ 0x200
        codec_sloginfo("0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
            vdi_read_register(coreIdx, i), vdi_read_register(coreIdx, i+4),
            vdi_read_register(coreIdx, i+8), vdi_read_register(coreIdx, i+0xc));
    }
}
