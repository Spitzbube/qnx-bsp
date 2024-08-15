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

#ifndef DECODER_CONFIG_H_
#define DECODER_CONFIG_H_

#include <sys/types.h>

#define AOIFOURCC4( C0, C1, C2, C3 ) ( ((uint32_t)C0<<24) | ((uint8_t)C1<<16) | ((uint8_t)C2<<8) | ((uint8_t)C3) )
#define AOIFOURCC( FCC ) AOIFOURCC4( FCC[0], FCC[1], FCC[2], FCC[3] )


__BEGIN_DECLS
// Common config data returned for all formats.
typedef struct {
    int32_t width;
    int32_t height;
    uint32_t par_x;
    uint32_t par_y;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    uint32_t fixed_frame_rate_flag;
    int32_t  config_len;
} decoder_info_t;

// H.264 AVC-specific return data. (fourcc "AVC1")
typedef struct avc_param_set
{
	uint8_t*   string;
	uint8_t*   nalu;
	int16_t    len;
	int16_t    nalu_length;
	int16_t    mmbuffer_offset;
} avc1_param_set_t;
typedef struct avc_crop_info
{
	uint32_t               crop_left;
	uint32_t               crop_right;
	uint32_t               crop_top;
	uint32_t               crop_bottom;
} avc1_crop_info_t;
typedef struct avc_config
{
	uint8_t                profile_indication;
	uint8_t                profile_compatibility;
	uint8_t                level_indication;
	int8_t                 size_length;
	int8_t                 sps_count;
	int8_t                 pps_count;
	decoder_info_t*        sps_info;
	avc1_param_set_t*      sps;
	avc1_param_set_t*      pps;
	avc1_crop_info_t       cropping;
	int32_t                num_ref_frames;
	uint8_t                frame_mbs_only_flag;
	uint8_t                video_full_range_flag;
	int32_t                chroma_format_idc;
} avc1_decoder_specific_t;

typedef struct mp2v_config
{
	uint32_t   frame_rate_num;
	uint32_t   frame_rate_den;
	uint32_t   bitrate;
} mp2v_decoder_specific_t;

/* HEVC/H265 decoder specific data structures */
typedef struct parameter_set
{
	iov_t   buf[16];
	uint8_t count;
} parameter_set_t;

typedef struct hevc_decoder_specific
{
	uint8_t         version;                             /* 8bits:  version                                               */
	uint8_t         general_profile_space;               /* 2bits:  the profile context                                   */
	uint8_t         general_tier_flag;                   /* 1bit:   profile context tier flag                             */
	uint8_t         general_profile_idc;                 /* 5bits:  the profile of the bitstream                          */
	uint32_t        general_profile_compatibility_flag;  /* 32bits: profile compatibility flag                            */
	uint8_t         progressive_source_flag;             /* 1bit:   the source is progressive                             */
	uint8_t         interlace_source_flag;               /* 1bit:   the source is interlaced                              */
	uint8_t         nonpacked_constraint_flag;           /* 1bit:   1 == no frame packing arrangement SEI messages        */
	uint8_t         frame_only_constraint_flag;          /* 1bit:   1 == no fields                                        */
	uint8_t         general_level_idc;                   /* 8bits:  the level of the bitstream                            */
	uint8_t         reserve1;
	uint16_t        min_spatial_segmentation;            /* 12bits: maximum possible size of distinct coded spatial segmentation regions*/
	uint8_t         parallelism_type;                    /* 2bits:  parallelism_type: 0=unknown, 1=slices, 2=tiles, 3=WPP */
	uint8_t         chroma_format;                       /* 2bits:  HEVC chroma_format, See table 6-1                     */
	uint8_t         bit_depth_luma;                      /* 3bits:  bit depth luma                                        */
	uint8_t         bit_depth_chroma;                    /* 3bits:  bit depth chroma                                      */
	uint16_t        avg_frame_rate;                      /* 16bits  reserved field: average  frame rate field             */
	uint8_t         cst_frame_rate;                      /* 2bits:  reserved field: constant frame rate field             */
	uint8_t         max_sub_layers;                      /* 3bits:  maximum number of temporal sub-layers                 */
	uint8_t         temporal_id_nesting_flag;            /* 1bit:   1 == inter prediction is additionally restricted      */
	uint8_t         size_nalu;                           /* 2bits:  size of NALU Length field                             */
	parameter_set_t vps;                                 /* HEVC    Video    Paramerers Set                               */
	parameter_set_t pps;                                 /* HEVC    Picture  Parameters Set                               */
	parameter_set_t sps;                                 /* HEVC    Sequence Parameter  Set                               */
	parameter_set_t sei;                                 /* HEVC    Supplemental enhancement information nalu             */
} hevc_decoder_specific_t;

typedef enum {
	DCFLAG_NO_FLAGS,
	DCFLAG_IGNORE_PPS,
} decoder_config_flags_t;

/* Use this function to free the dynamic decoder specific struct. */
extern void MmFreeDecoderSpecific( void* decoder_specific, uint32_t fourcc );
/* This function will take a raw decoder config block and parse out width, height, and
 * pixel aspect ratio.  Other decoder specific parsing depends on the fourcc passed in.
 * fourcc            decoder_specific type
 * AVC1              avc_config_t
 * MP2V              mp2v_decoder_specific_t
 */
extern int32_t MmParseDecoderConfig(const void *config_data, uint32_t data_size, uint32_t fourcc,
		decoder_info_t *info_out, void **decoder_specific, decoder_config_flags_t flag);

__END_DECLS
#endif /* DECODER_CONFIG_H_ */




