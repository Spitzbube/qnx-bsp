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
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "tivpu_codec.h"

int32_t tivpu_codec_init(uint32_t coreIdx, char* path, int op_polling)
{
    Int32       nread;
    Uint32      totalRead, allocSize, readSize = WAVE5_MAX_CODE_BUF_SIZE;
    Uint8*      firmware = NULL;
    Uint32      firmwareSize = 0;
    osal_file_t fp;
    ProductId   productId;
    int32_t     retval;

    productId  = (ProductId)VPU_GetProductId(coreIdx);
    codec_sloginfo("%s:%d product id: %d\n", __FUNCTION__, __LINE__, productId);
    if (!PRODUCT_ID_W_SERIES(productId)) {
        codec_slogerr("%s:%d Unknown product id: %d\n", __FUNCTION__, __LINE__, productId);
        return -1;
    }

    if ((fp = osal_fopen(path, "rb")) == NULL) {
        VLOG(ERR, "Failed to open %s\n", path);
        return -1;
    }

    totalRead = 0;
    firmware = (Uint8*)osal_malloc(readSize);
    if (firmware == NULL) {
        VLOG(ERR, "Failed to allocate space for firmware read, size: %d\n", (int32_t)readSize);
        return -1;
    }
    allocSize = readSize;
    nread = 0;
    while (TRUE) {
        if (allocSize < (totalRead+readSize)) {
            allocSize += 2 * nread;
            firmware = (Uint8*)osal_realloc(firmware, allocSize);
        }
        if (firmware) {
            nread = osal_fread((void*)&firmware[totalRead], 1, readSize, fp);//lint !e613
        }
        else {
            VLOG(ERR, "Failed to allocate space for firmware read, size: %d\n", (int32_t)allocSize);
            return -1;
        }
        totalRead += nread;
        if (nread < (Int32)readSize)
            break;
    }
    firmwareSize = (totalRead+1) / 2;
    osal_fclose(fp);

    if (op_polling)
        VPU_SetPollingMode();

    retval = VPU_InitWithBitcode(coreIdx, (Uint16 *)firmware, firmwareSize);
    if (retval != RETCODE_SUCCESS && retval != RETCODE_CALLED_BEFORE) {
        codec_slogerr("%s:%d Failed to VPU_InitiWithBitcode, ret(%08x)\n", __FUNCTION__, __LINE__, retval);
    }

    osal_free(firmware);
    return retval;
}

int32_t tivpu_codec_get_product_info(VpuAttr* attr, uint32_t coreIdx, Uint32 *cyclePerTick)
{
    int32_t retval;

    retval = VPU_GetProductInfo(coreIdx, attr);
    if (retval != RETCODE_SUCCESS) {
        codec_slogerr("%s:%d Failure in VPU_GetProductInfo, ret(%08x)\n", __FUNCTION__, __LINE__, retval);
        return retval;
    }

    /* Set cyclePerTick value based on attributes obtained from the VPU */
    *cyclePerTick = 32768;
    if (TRUE == attr->supportNewTimer)
        *cyclePerTick = 256;

    /* Print VPU Product Info */
    codec_sloginfo("VPU coreNum : [%d]\n", coreIdx);
    codec_sloginfo("Firmware : CustomerCode: %04x | version : rev.%d\n", attr->customerId, attr->fwVersion);
    codec_sloginfo("Hardware : %04x\n", attr->productId);
    codec_sloginfo("API      : %d.%d.%d\n\n", API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    codec_sloginfo("fwVersion       : %08x(r%d)\n", attr->fwVersion, attr->fwVersion);
    codec_sloginfo("productName     : %s%4x\n", attr->productName, attr->productVersion);

    return retval;
}

int32_t tivpu_cleanup_buffers(tivpu_context_t *vpu_ctx)
{
    int i = 0;
    BOOL ret = EOK;

    if((vpu_ctx == NULL)) {
        return -1;
    }


        // output buffers
        vpu_buffer_t    *bufs = vpu_ctx->output_bufs;
        uint32_t        buf_num = vpu_ctx->output_buf_num;

        for(i = 0;i < buf_num; i++) {
            ret = munmap((void *)(bufs[i].virt_addr), bufs[i].usr_info.alloc_size);
            if (ret != EOK)
                codec_slogerr("failed munmap call for buf phys_addr = 0x%lx size = 0x%lx",
                                    bufs[i].phys_addr, bufs[i].usr_info.alloc_size);
        }
        osal_free(bufs);

        // input buffers
        bufs = vpu_ctx->input_bufs;
        buf_num = vpu_ctx->input_buf_num;

        for(i = 0;i < buf_num; i++) {
            ret = munmap((void *)(bufs[i].virt_addr), bufs[i].usr_info.alloc_size);
            if (ret != EOK)
                codec_slogerr("failed munmap call for buf phys_addr = 0x%lx size = 0x%lx",
                                    bufs[i].phys_addr, bufs[i].usr_info.alloc_size);
        }

        osal_free(bufs);

        vpu_ctx->input_bufs = vpu_ctx->output_bufs = NULL;

        //Move this to a different function specific to the encoder
        //tivpu_release_fb_mem(vpu_ctx->codec_ctx);
    return ret;
}


