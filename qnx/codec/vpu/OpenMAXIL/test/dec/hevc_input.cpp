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
#include <stdlib.h> 
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#include "config_parser.h"
#include "hevc_input.h"
#include "log.h"

#ifndef DIM
#  define DIM(a) (sizeof((a)) / sizeof((a)[0]))
#endif

#ifndef ALIGN4
#define ALIGN4(X)   (((X)+0x03) &~0x03)
#endif
#ifndef ALIGN32
#define ALIGN32(X)  (((X)+0x1F) &~0x1F)
#endif

enum {
    HEVC_NALU_TYPE_VIDEO_PARAM = 32,
    HEVC_NALU_TYPE_SEQ_PARAM = 33,
    HEVC_NALU_TYPE_PIC_PARAM = 34,
    HEVC_NALU_TYPE_PREFIX_SEI = 39
};

static inline bool CheckStartCode(
    const uint8_t *sc,
    uint32_t scsize,
    const uint8_t *inbuf,
    uint32_t isize,
    const uint32_t *byte_to_apply_mask,
    const uint8_t *start_code_mask )
{
    uint32_t sc_index = 0;

    if( isize < scsize ) {
        return false;
    }

    while( scsize ) {
        if( byte_to_apply_mask && start_code_mask && (sc_index == *byte_to_apply_mask) ) {
            if( *sc != (*inbuf & *start_code_mask) ) {
                return false;
            }
        } else {
            if( *sc != *inbuf ) {
                return false;
            }
        }
        scsize--;
        sc++;
        inbuf++;
        sc_index++;
    }

    return true;
}


static inline int FindStartCode(
    const uint8_t *sc,
    uint32_t scsize,
    const uint8_t *inbuf,
    uint32_t isize,
    const uint32_t *byte_to_apply_mask,
    const uint8_t *start_code_mask )
{
    int size = 0;

    while( isize ) {
        if( CheckStartCode( sc, scsize, inbuf, isize, byte_to_apply_mask, start_code_mask ) ) {
            break;
        }

        isize--;
        size++;
        inbuf++;
    }

    return size;
}

static int HEVCCheckNalType( const uint8_t *inbuf, int *config_data, int *frame_data )
{
    int nalType = 0;

    // detect SPS/PPS/Frame data
    nalType = (inbuf[0] & 0x7E) >> 1;
    if( (nalType == HEVC_NALU_TYPE_VIDEO_PARAM )
        || (nalType == HEVC_NALU_TYPE_PIC_PARAM)
        || (nalType == HEVC_NALU_TYPE_SEQ_PARAM) ) {
        *config_data = 1;
    }
    else if(nalType != HEVC_NALU_TYPE_PREFIX_SEI){
        *frame_data = 1;
    }

    return nalType;
}

int32_t OmxilVideoDecInputHEVC::readFrame(uint8_t *oBuf, uint32_t *oSize)
{
    *oSize = read(m_fd, oBuf, INPUT_BUFFER_SIZE);
    if(*oSize <= 0) {
        LOG(LOG_INFO,"%s=> reach the end of data", __func__);
        m_EOF = true;
        return EOK;
    }

    return EOK;
#if 0
    static const uint8_t sc[] = {0x00, 0x00, 0x01};  /* start code */
    int initial_sc_offset = 0;
    int cfg_data_found = 0;
    int frame_found = 0;
    int nalSize;
    uint8_t *buf;
    uint32_t      size;
    const uint8_t *inbuf = m_currPtr;
    int32_t err = EOK;

    size = m_inputDataSize;
    buf = m_currPtr;
    *oSize = 0;

    LOG(LOG_DEBUG2,"%s (m_currPtr=%p, m_inputDataSize=%d)", __func__, m_currPtr, m_inputDataSize);

    if(m_inputDataSize < (int32_t)(DIM(sc)+ 2)) {
        if(m_EOF) {
            LOG(LOG_INFO,"%s=> reach the end of data", __func__);
            return EOK;
        }
        else {
            uint8_t *rbuf = m_inputBuffer;
            if(m_inputDataSize > 0) {
                memcpy(rbuf, buf, m_inputDataSize);
                rbuf += m_inputDataSize;
            }
            m_inputDataSize = read(m_fd, rbuf, DEFAULT_BUFFER_SIZE);
            if(m_inputDataSize <= 0) {
                LOG(LOG_INFO,"%s=> reach the end of data", __func__);
                m_EOF = true;
                return EOK;
            }
        }
    }
    // check if the buffer starts with the start code, else skip the bytes
    initial_sc_offset = FindStartCode( sc, DIM(sc), inbuf, m_inputDataSize, NULL, NULL );
    LOG(LOG_DEBUG2,"%s=> start code was found at initial_sc_offset %d (skipping these bytes)", __func__, initial_sc_offset );
    if( initial_sc_offset > m_inputDataSize) {
        LOG(LOG_ERROR,"%s=> Wrong input stream initial_sc_offset=%d, m_inputDataSize=%d", __func__, initial_sc_offset, m_inputDataSize);
        return EINVAL;
    }
    buf += initial_sc_offset;
    size -= initial_sc_offset;

    if (size < DIM(sc)+ 2) {
        LOG(LOG_INFO,"%s=> Only frame header found(size=%d), skip", __func__, size);
        return EOK;
    }

    while( ( size >= DIM(sc)+ 2 ) ) {
        // detect SPS/PPS/Frame data
        HEVCCheckNalType( (buf + DIM(sc)), &cfg_data_found, &frame_found );

        /* if we've already started pushing NALU to the buffer, check
         * if this next NALU starts a new access unit
         */
        nalSize = DIM(sc) + FindStartCode( sc, DIM(sc), buf+DIM(sc), size-DIM(sc), NULL, NULL );

        if( nalSize > (int)size ) {
            LOG(LOG_ERROR,"%s=> Wrong input stream nal_size=%d, size=%d", __func__, nalSize, size );
            err = EINVAL;
            break;
        }

        memcpy(oBuf + *oSize, buf, nalSize);
        buf += nalSize;
        size -= nalSize;
        *oSize += nalSize;

        //no access unit delimiter return once frame data found
        if( frame_found ) {
            break;
        }
    }

    if(size == 0 && !m_EOF) {
        m_inputDataSize = read(m_fd, m_inputBuffer, DEFAULT_BUFFER_SIZE);
        if(m_inputDataSize >= 0) {
            LOG(LOG_DEBUG2,"%s:%d=> read more data=%d", __func__, __LINE__, m_inputDataSize);
            buf = m_inputBuffer;
            size = m_inputDataSize;
            nalSize = FindStartCode( sc, DIM(sc), buf, size, NULL, NULL );
            if( nalSize > (int)size ) {
                LOG(LOG_ERROR,"%s:%d=> Wrong input stream nal_size=%d, size=%d", __func__, __LINE__, nalSize, size );
                err = EINVAL;
            }
            else {
                memcpy(oBuf + *oSize, buf, nalSize);
                *oSize += nalSize;
                buf += nalSize;
                size -= nalSize;
            }
            if(m_inputDataSize < DEFAULT_BUFFER_SIZE) {
                LOG(LOG_DEBUG2,"%s:%d=> reach end of file %d", __func__, __LINE__, m_inputDataSize);
                m_EOF = true;
            }
        }
        else {
            LOG(LOG_ERROR,"%s=> failed to read data=%d", __func__, m_inputDataSize);
            err = EINVAL;
        }

    }

    m_inputDataSize = size;
    m_currPtr = buf;

    LOG(LOG_DEBUG2,"%s get one frame(%u) input buffer(m_currPtr=%p, m_inputDataSize=%d)", __func__,
            *oSize, m_currPtr, m_inputDataSize);

    return err;
#endif
}

int32_t OmxilVideoDecInputHEVC::loadConfig()
{
    static const uint8_t sc[] = {0x00, 0x00, 0x01};  /* start code */
    int initial_sc_offset = 0;
    int cfg_data_found = 0;
    int frame_found = 0;
    int nalSize;
    uint8_t *buf;
    uint32_t      size;
    int32_t err = EOK;

    size = m_inputDataSize;
    buf = m_currPtr;
    mConfigSize = 0;

    if(m_inputDataSize < (int32_t)(DIM(sc)+ 2)) {
        LOG(LOG_INFO,"%s=> reach the end of data", __func__);
        return EINVAL;
    }
    // check if the buffer starts with the start code, else skip the bytes
    initial_sc_offset = FindStartCode( sc, DIM(sc), buf, m_inputDataSize, NULL, NULL );
    LOG(LOG_DEBUG2,"%s=> start code was found at initial_sc_offset %d (skipping these bytes)", __func__, initial_sc_offset );
    if( initial_sc_offset > m_inputDataSize) {
        LOG(LOG_ERROR,"%s=> Wrong input stream initial_sc_offset=%d, m_inputDataSize=%d", __func__, initial_sc_offset, m_inputDataSize);
        return EINVAL;
    }
    buf += initial_sc_offset;
    size -= initial_sc_offset;

    if (size < DIM(sc)+ 2) {
        LOG(LOG_ERROR,"%s=> Only frame header found(size=%d), skip", __func__, size);
        return EINVAL;
    }

    while( ( size >= DIM(sc)+ 2 ) ) {
        // detect SPS/PPS/Frame data
        cfg_data_found = 0;
        HEVCCheckNalType( (buf + DIM(sc)), &cfg_data_found, &frame_found );

        /* if we've already started pushing NALU to the buffer, check
         * if this next NALU starts a new access unit
         */
        nalSize = DIM(sc) + FindStartCode( sc, DIM(sc), buf+DIM(sc), size-DIM(sc), NULL, NULL );
        LOG(LOG_DEBUG2,"%s=> nalSize=%d", __func__, nalSize);

        // detect SPS/PPS/Frame data
        if( nalSize > (int)size ) {
            LOG(LOG_ERROR,"%s=> Wrong input stream nal_size=%d, size=%d", __func__, nalSize, size );
            err = EINVAL;
            break;
        }

        if(cfg_data_found) {
            if(nalSize + mConfigSize < CONFIG_DATA_BUFFER_SIZE) {
                memcpy(mpConfig + mConfigSize, buf, nalSize);
                mConfigSize += nalSize;
            }
            else {
                LOG(LOG_ERROR,"%s=> config data buffer too small( nal_size=%d, size=%d)", __func__, nalSize, size );
                err = ENOMEM;
                break;
            }
        }
        else if(mConfigSize > 0) {
            break;
        }

        buf += nalSize;
        size -= nalSize;
    }

    m_inputDataSize = size;
    m_currPtr = buf;

    decoder_info_t decoder_info;
    hevc_decoder_specific_t *hevc_info;
    if(hevc_parse_decoder_config( mpConfig, mConfigSize, &decoder_info, &hevc_info) == EOK ) {
        mWidth = decoder_info.width;
        mHeight = decoder_info.height;
        mFrameRate = 30;
        mLumaDepth = hevc_info->bit_depth_luma;
        mChromaDepth = hevc_info->bit_depth_chroma;
        mChromaFmt = hevc_info->chroma_format;
        mAlignedWidth = ALIGN32(decoder_info.width);
        mAlignedHeight = ALIGN4(decoder_info.height);
        hevc_free_decoder_specific(hevc_info);
    }
    else {
        LOG(LOG_ERROR,"%s=> Failed to get config data(%d) ", __func__, mConfigSize);
        err = EINVAL;
    }

    LOG(LOG_DEBUG2,"%s get config(%u) input buffer(m_currPtr=%p, m_inputDataSize=%d)", __func__,
            mConfigSize, m_currPtr, m_inputDataSize);

    /* Return the read pointer back to beginning of the file */
    lseek(m_fd, 0, SEEK_SET);

    return err;
}

