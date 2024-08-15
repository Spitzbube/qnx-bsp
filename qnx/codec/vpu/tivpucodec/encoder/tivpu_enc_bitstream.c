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
#include <string.h>
#include <sys/mman.h>

#include "tivpu_enc.h"

BOOL tivpu_enc_bitstream_prepare(EncoderContext_t* ctx)
{
    Uint32               i;
    vpu_buffer_t*        bsBuffer;
    Uint32               num = ctx->bsBuf.num;

    bsBuffer = &ctx->bsBuf.bs[0];
    for (i = 0; i < num; i++) {
        bsBuffer[i].size = ctx->encOpenParam.streamBufSize;
        if (vdi_attach_dma_memory(ctx->encOpenParam.coreIdx, &bsBuffer[i]) < 0) {
            VLOG(ERR, "%s:%d failed to vdi_attach to bitstream buffer\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
    }

    return TRUE;
}
