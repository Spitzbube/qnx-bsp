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
#include <stdio.h>
#include <stdlib.h> // calloc
#include <errno.h>
#include <sys/types.h>
#include <string.h>


#include "config_parser.h"
#include "log.h"

#define SECOND_BYTE      0x8000000
#define BS_PART_READ( buffptr, retval, bytesleft ) do {             \
    int32_t remainShift = (4 - bytesleft) * 8; \
	while (bytesleft > 0) { \
		retval <<= 8; \
		retval |= *buffptr; \
		buffptr++; \
		bytesleft--; \
	} \
	retval <<= remainShift; \
} while( 0 )

static inline uint32_t BS_BYTE_READ(const uint8_t *x)
{
    return (uint32_t)((((x))[0] << 24) | (((x))[1] << 16) | (((x))[2] <<  8) | ((x))[3]);
}

static inline int32_t mmf_log2(uint32_t v)
{
	int32_t log2 = 0;
	int32_t i;

	for (i = 31; i >= 0; i--) {
		if (v >> i) {
			log2 = i;
			break;
		}
	}
	return log2;
}

// The following values are used to parse the HEVC/H265  decoder config.
enum { HEVC_NALU_TYPE_VIDEO_PARAM = 32,  HEVC_NALU_TYPE_SEQ_PARAM = 33, HEVC_NALU_TYPE_PIC_PARAM = 34, HEVC_NALU_TYPE_PREFIX_SEI = 39 };

// The following tables are required to parse the AVCC SPS block to retrieve height and
// width for H.264 streams.
static const uint8_t vlc_len[512]={
14,13,12,12,11,11,11,11,10,10,10,10,10,10,10,10,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,7,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1};

static const uint8_t golomb_vals[512]={
31,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
7,7,7,7,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,
6,6,6,6,6,6,6,6,6,6,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0};

static const int8_t signed_golomb_vals[512]={
16,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,8,-8,9,-9,10,-10,11,-11,12,-12,13,-13,14,-14,15,
-15,4,4,4,4,-4,-4,-4,-4,5,5,5,5,-5,-5,-5,-5,6,6,6,6,-6,-6,-6,-6,7,7,7,7,-7,-7,-7,-7,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0};

typedef struct {
	const uint8_t       *buff;
	uint32_t            buff_size;
	int                 bit_offset;
} psbitstream_t;


static inline uint32_t mmf_bitstremgetbit(psbitstream_t *s){
	int32_t byte    = s->bit_offset / 8;
	uint8_t result;

	if (byte >= (int32_t)s->buff_size) {
		return 0;
	}
	result  = s->buff[ byte ];
	result <<= (s->bit_offset & 0x07);
	result >>= 7;
	s->bit_offset++;

	return result;
}

// This is lazy, but we're not worried about performance.
static inline uint32_t mmf_bitstremgetbits(psbitstream_t *s, int n){
	int32_t  i;
	uint32_t retval = 0;

	if (s->bit_offset/8 >= (int32_t)s->buff_size) {
		return 0;
	}

	n--;
	for (i = 0; i < n ; i++) {
		retval |= mmf_bitstremgetbit(s);
		retval <<= 1;
	}
	retval |= mmf_bitstremgetbit(s);

	return retval;
}

//
// These are exp golomb code reading funcs. Required for AVC/ H.264
//
static uint32_t mmf_golombunsigned(psbitstream_t *psb) {
	uint32_t retval = 0;
	int32_t  byte = psb->bit_offset / 8;
	int32_t  log;
	int32_t  bytesleft = psb->buff_size - byte;

	if (psb->bit_offset/8 >= (int32_t)psb->buff_size) {
		return 0;
	}

	if (bytesleft >= 4) {
		retval = BS_BYTE_READ( &psb->buff[byte] );
	} else {
		uint8_t *buffptr = (uint8_t*)&psb->buff[byte];
		BS_PART_READ( buffptr, retval, bytesleft );
	}

	retval <<= (psb->bit_offset & 0x07);

	if( retval >= SECOND_BYTE ) {
		retval >>= 23;
		psb->bit_offset += vlc_len[retval];

		return golomb_vals[retval];
	}
	log = 2 * mmf_log2(retval) - 31;
	psb->bit_offset += 32 - log;

	retval >>= log;
	retval--;
	return retval;
}

static int32_t mmf_golombconstrained(psbitstream_t *psb) {
	uint32_t retval = 0;
	int32_t  byte = psb->bit_offset / 8;
	int32_t  bytesleft = psb->buff_size - byte;

	if (psb->bit_offset/8 >= (int32_t)psb->buff_size) {
		return 0;
	}

	if (bytesleft >= 4) {
		retval = BS_BYTE_READ( &psb->buff[byte] );
	} else {
		uint8_t *buffptr = (uint8_t*)&psb->buff[byte];
		BS_PART_READ( buffptr, retval, bytesleft );
	}
	retval <<= (psb->bit_offset & 0x07);
	retval >>= 23;
	psb->bit_offset += vlc_len[retval];

	return golomb_vals[retval];
}

static int32_t mmf_golombsigned(psbitstream_t *psb) {
	int32_t  retval = 0;
	uint32_t codenum = 0;
	int32_t  byte = psb->bit_offset / 8;
	int32_t  log;
	int32_t  bytesleft = psb->buff_size - byte;

	if (psb->bit_offset/8 >= (int32_t)psb->buff_size) {
		return 0;
	}

	if (bytesleft >= 4) {
		codenum = BS_BYTE_READ( &psb->buff[byte] );
	} else {
		uint8_t *buffptr = (uint8_t*)&psb->buff[byte];
		BS_PART_READ( buffptr, codenum, bytesleft );
	}
	codenum <<= (psb->bit_offset & 0x07);

	if(codenum >= SECOND_BYTE){
		codenum >>= 23;
		psb->bit_offset += vlc_len[codenum];

		return signed_golomb_vals[codenum];
	}
	log = 2 * mmf_log2(codenum) - 31;
	psb->bit_offset += 32 - log;

	codenum >>= log;
	if (codenum & 0x00001) {
		retval = -((int32_t)(codenum>>1));
	} else {
		retval =  (codenum>>1);
	}

	return retval;

}


static void skip_list(psbitstream_t *psb, int size) {
	// We only worry about it if the presence bit says it's there.
	if(mmf_bitstremgetbit(psb)) {
		int32_t       val;
		int32_t       i;

		val = 8;
		for(i = 0; i < size; i++) {
			if (val) {
				val = (val + mmf_golombsigned(psb)) & 0xff;
			}
			// If it's not there
			if(!i && !val){
				break;
			}
		}
	}
}


//
// H.264/AVC Config Parsing.
//
#define H264_CUSTOM_ASPECT_RATIO     255
static const uint8_t h264_aspec_ratio_from_idc[14][2] = {
		{1, 1},
		{1, 1},
		{12, 11},
		{10, 11},
		{16, 11},
		{40, 33},
		{24, 11},
		{20, 11},
		{32, 11},
		{80, 33},
		{18, 11},
		{15, 11},
		{64, 33},
		{160, 99}
};


static int h264_parse_sps(const uint8_t *rawps, uint32_t pslen, decoder_info_t *info, avc1_decoder_specific_t *avc1) {
	psbitstream_t   psb;
	int32_t         profile;
	int32_t         poc_type;
	int32_t         i;
	int32_t         ar_idc;

	// Just in case there is no aspect ratio information in the SPS.
	info->par_x = 1;
	info->par_y = 1;
	info->width = 0;
	info->height = 0;

	// Turn our buffer into a bitstream.
	psb.bit_offset = 0;
	psb.buff_size = pslen;
	psb.buff = rawps;

	profile = mmf_bitstremgetbits(&psb, 8);
	avc1->profile_indication    = profile;
	avc1->profile_compatibility = mmf_bitstremgetbits(&psb, 8);
	avc1->level_indication      = mmf_bitstremgetbits(&psb, 8);

	if(mmf_golombconstrained(&psb) >= 32) {
		return -1;
	}

	// More to do for high profile or better.
    if( profile == 100 || profile == 110 ||
            profile == 122 || profile == 244 || profile == 44 ||
            profile == 83 || profile == 86 || profile == 118 ||
            profile == 128 || profile == 138 || profile == 139 ||
            profile == 134 || profile == 135 ) {

		avc1->chroma_format_idc = mmf_golombconstrained(&psb);
		if (avc1->chroma_format_idc == 3)
			mmf_bitstremgetbit(&psb);

		avc1->bit_depth_luma = mmf_golombunsigned(&psb) + 8;
		avc1->bit_depth_chroma = mmf_golombunsigned(&psb) + 8;
		mmf_bitstremgetbit(&psb);

		if (mmf_bitstremgetbit(&psb)) {
			for (i = 0; i < 6; i++) {
				skip_list(&psb, 16);
			}
			for (i = 0; i < 2; i++) {
				skip_list(&psb, 64);
			}
		}
	}
    else {
        //set default value if they are absent
        avc1->chroma_format_idc = 1;
        avc1->bit_depth_luma = 8;
        avc1->bit_depth_chroma = 8;
    }

	mmf_golombunsigned(&psb);
	poc_type = mmf_golombconstrained(&psb);

	if (poc_type == 0) {
		mmf_golombunsigned(&psb);
	} else if (poc_type == 1) {
		int32_t cycle_len;

		mmf_bitstremgetbit(&psb);
		mmf_golombsigned(&psb);
		mmf_golombsigned(&psb);
		cycle_len = mmf_golombunsigned(&psb);

		for(i = 0; i < cycle_len; i++)
			mmf_golombsigned(&psb);
	}

	avc1->num_ref_frames =  mmf_golombconstrained(&psb);
	mmf_bitstremgetbit(&psb);

	// Store the width and height
	info->width = (mmf_golombunsigned(&psb) + 1) * 16;
    int32_t pic_height_in_map_units_minus1 = mmf_golombunsigned(&psb);

	// Mine for aspect ratio info.
	// frame mbs only flag
	if(mmf_bitstremgetbit(&psb) == 0) {
		avc1->frame_mbs_only_flag = 0;
		//mb_adaptive_frame_field_flag
		mmf_bitstremgetbit(&psb);
	} else {
		avc1->frame_mbs_only_flag = 1;
	}

	info->height = (pic_height_in_map_units_minus1 + 1) * (avc1->frame_mbs_only_flag ? 1 : 2) * 16;
        
	//direct_8x8_inference_flag
	mmf_bitstremgetbit(&psb);
	//frame_cropping_flag
	if(mmf_bitstremgetbit(&psb)) {
		int vsub   = (avc1->chroma_format_idc == 1) ? 1 : 0;
		int hsub   = (avc1->chroma_format_idc == 1 || avc1->chroma_format_idc == 2) ? 1 : 0;
		int step_x = 1 << hsub;
		int step_y = (2 - avc1->frame_mbs_only_flag) << vsub;

		// frame_crop_left_offset
		unsigned int crop_left = mmf_golombunsigned(&psb);
		//frame_crop_right_offset
		unsigned int crop_right = mmf_golombunsigned(&psb);
		//frame_crop_top_offset
		unsigned int crop_top = mmf_golombunsigned(&psb);
		//frame_crop_bottom_offset
		unsigned int crop_bottom = mmf_golombunsigned(&psb);

		if (crop_left  > (unsigned)INT32_MAX / 4 / step_x ||
				crop_right > (unsigned)INT32_MAX / 4 / step_x ||
				crop_top   > (unsigned)INT32_MAX / 4 / step_y ||
				crop_bottom> (unsigned)INT32_MAX / 4 / step_y ||
				(crop_left + crop_right ) * step_x >= (uint32_t)info->width ||
				(crop_top  + crop_bottom) * step_y >= (uint32_t)info->height
				) {
			LOG( LOG_WARNING,"crop values invalid %d %d %d %d / %d %d\n", crop_left, crop_right, crop_top, crop_bottom, info->width, info->height);
			avc1->cropping.crop_left = 0;
			avc1->cropping.crop_right = 0;
			avc1->cropping.crop_top = 0;
			avc1->cropping.crop_bottom = 0;
		} else {
			avc1->cropping.crop_left   = crop_left   * step_x;
			avc1->cropping.crop_right  = crop_right  * step_x;
			avc1->cropping.crop_top   = crop_top    * step_y;
			avc1->cropping.crop_bottom= crop_bottom * step_y;
		}
	}

	// VUI Parameters Flags -- Aspect ratio is here.
	if(mmf_bitstremgetbit(&psb)) {
		LOG( LOG_DEBUG2,"VUI Parameters present");
		//aspect_ratio_info_present_flag
		if (mmf_bitstremgetbit(&psb)) {
			LOG( LOG_DEBUG2,"aspect ratio present");
			ar_idc = mmf_bitstremgetbits(&psb, 8);
			LOG( LOG_DEBUG2,"    ar_idc = %d", ar_idc);
			if (ar_idc == H264_CUSTOM_ASPECT_RATIO) {
				info->par_x = mmf_bitstremgetbits(&psb, 16);
				info->par_y = mmf_bitstremgetbits(&psb, 16);
				LOG( LOG_DEBUG2,"    sar x = %d, sar y = %d.", info->par_x, info->par_y);
			} else {
				if (ar_idc < 14) {
					// Interpret the pre-defined field.
					info->par_x = h264_aspec_ratio_from_idc[ar_idc][0];
					info->par_y = h264_aspec_ratio_from_idc[ar_idc][1];
				}
			}
			// By spec. if par x or par y are 0, par is unspecified.  So back to 1:1
			if (info->par_x == 0 || info->par_y == 0) {
				LOG( LOG_DEBUG2,"    Undefined aspect ratio.");
				info->par_x = 1;
				info->par_y = 1;
			}
		}

		//overscan_info_present_flag
		if (mmf_bitstremgetbit(&psb)) {
			mmf_bitstremgetbit(&psb);
		}

		//video_signal_type_present_flag
		if (mmf_bitstremgetbit(&psb)) {
			mmf_bitstremgetbits(&psb, 3);
			avc1->video_full_range_flag = mmf_bitstremgetbit(&psb);
			//colour_description_present_flag
			if (mmf_bitstremgetbit(&psb)) {
				mmf_bitstremgetbits(&psb, 24);
			}
		}

		//chroma_loc_info_present_flag
		if (mmf_bitstremgetbit(&psb)) {
			mmf_golombunsigned(&psb);
			mmf_golombunsigned(&psb);
		}

		//timing_info_present_flag
		if (mmf_bitstremgetbit(&psb)) {
			info->num_units_in_tick = mmf_bitstremgetbits(&psb, 32);
			info->time_scale = mmf_bitstremgetbits(&psb, 32);
			info->fixed_frame_rate_flag = mmf_bitstremgetbit(&psb);
		}


	}

	return EOK;
}

// cleanup the avc configuration object
void h264_free_decoder_specific( avc1_decoder_specific_t* avcc )
{
	int32_t i;
	if (avcc) {
		for( i = 0; i < avcc->sps_count; i++ ) {
			free(avcc->sps[i].string);
			free(avcc->sps[i].nalu);
		}
		free(avcc->sps);
		free(avcc->sps_info);
		for( i = 0; i < avcc->pps_count; i++ ) {
			free(avcc->pps[i].string);
			free(avcc->pps[i].nalu);
		}
		free(avcc->pps);
		free(avcc);
	}
}


int32_t h264_parse_decoder_config(const uint8_t* raw_avcc, uint32_t avcc_size, decoder_info_t *info_out, avc1_decoder_specific_t** avcc_out)
{
    int32_t i;
    int32_t bytepos = 1;
    avc1_decoder_specific_t *avcc;
    uint8_t *buf;
    int32_t offsetSps;
    int32_t offsetPps;
    uint8_t nalType;
    uint32_t size;

    if (avcc_size < 9) {
        return ENOTSUP;
    }

    // Search for SPS and PPS
    offsetSps = offsetPps = -1;
    for (i = 0; i < (int32_t)avcc_size - 4; i++) {
        // First byte is a don't care, but must be present.
        if (raw_avcc[i] == 0 && raw_avcc[i+1] == 0 && raw_avcc[i+2] == 1) {
            nalType = raw_avcc[i+3] & 0x1F;
            if (nalType == 7) {
                LOG( LOG_DEBUG1,"Found SPS at offset %d", i);
                offsetSps = i;
            }
            if (nalType == 8) {
                LOG( LOG_DEBUG1,"Found PPS at offset %d", i);
                offsetPps = i;
            }
        }
        if ((offsetSps != -1) && (offsetPps != -1)) {
            break;
        }
    }

    if (offsetSps < 0) {
        LOG( LOG_DEBUG1, "No SPS" );
        return ENOTSUP;
    }
    if (offsetPps < 0) {
        LOG( LOG_DEBUG1, "No PPS" );
        return ENOTSUP;
    }

    if ((avcc = (avc1_decoder_specific_t *)calloc(1, sizeof(avc1_decoder_specific_t))) == NULL) {
        return ENOMEM;
    }

    avcc->sps_count             = 1;
    avcc->sps                   = (avc1_param_set_t*)calloc(1, sizeof(avc1_param_set_t));
    avcc->sps_info              = (decoder_info_t*)calloc(1, sizeof(decoder_info_t));
    if (!avcc->sps || !avcc->sps_info )
    {
        h264_free_decoder_specific(avcc);
        return ENOMEM;
    }

    // Parse required info out of the SPS.
    // Start parsing SPS header
    buf =  (uint8_t *)raw_avcc + offsetSps + 4;
    size = avcc_size - (offsetSps + 4);
    if (size < 2) {
        h264_free_decoder_specific(avcc);
        return ENOTSUP;
    }

    h264_parse_sps(buf, size, &avcc->sps_info[0], avcc);

    if ((avcc_size - bytepos) < 1) {
        h264_free_decoder_specific(avcc);
        return ENOTSUP;
    }
    avcc->pps_count             = 1;
    avcc->pps                   = (avc1_param_set_t*)calloc(1, sizeof(avc1_param_set_t)*avcc->pps_count);

    // For h.264 there can be multiple param sets, for now take the first.
    // we are returning all of them, so a parser can choose the appropriate one.
    if (info_out) {
        *info_out = avcc->sps_info[0];
    }

    if (avcc_out) {
        *avcc_out = avcc;
    } else {
        h264_free_decoder_specific(avcc);
    }
    return EOK;
}

static const uint8_t* hvcc_next_annexb_nalu( const uint8_t *ibuf, uint32_t isize, int32_t *iremain,  uint32_t *nal_size )
{
  // annexb format
  uint32_t       cmp   = 0xFFFFFFFF;
  uint32_t       i_end = isize;
  const uint8_t  *sptr = ibuf;
  uint32_t       tmp;

  if( !nal_size )
    nal_size = &tmp;
  // look for start code 0X00000001 */
  while( i_end ) {
    cmp = (cmp << 8) | *sptr;
    if( (cmp ^ UINT32_C(0x100)) <= UINT32_C(0xFF) )
      break;
    sptr++;
    i_end--;
  }

  if( iremain )
    *iremain = i_end;
  *nal_size = sptr - ibuf;
  if( i_end )
  {
    *nal_size -= 4; // remove the startcode size
    LOG(LOG_DEBUG2, "%s: found nalu at offset=%zu nal_type=%d", __func__, (sptr-ibuf), ((sptr[0] & 0x7E) >> 1));
    return sptr;
  }
  return NULL;
}

static uint8_t* nal_unit_extract_rbsp( const uint8_t *ibuf, uint32_t isize, uint32_t *dst_len )
{
  uint32_t i   = 0;
  uint32_t len = 0;
  uint8_t  *dst;

  if( (dst = (uint8_t*)malloc(isize)) ) {
    while( i < 2 && i < isize )
      dst[len++] = ibuf[i++];
    while( (i + 2) < isize ) {
      if( !ibuf[i] && !ibuf[i + 1] && ibuf[i + 2] == 3 ) {
        dst[len++] = ibuf[i++];
        dst[len++] = ibuf[i++];
        i++; // remove emulation_prevention_three_byte
      } else {
        dst[len++] = ibuf[i++];
      }
    }
    while( i < isize )
      dst[len++] = ibuf[i++];
  }
  *dst_len = len;
  return dst;
}

static int32_t hvcc_read_parameter_set( psbitstream_t *bs, parameter_set_t *ps, uint32_t type, uint32_t *set_count )
{
  uint32_t count     = mmf_bitstremgetbits(bs, 16);
  uint32_t count_max = sizeof(ps->buf) / sizeof(ps->buf[0]);
  uint32_t available = bs->buff_size - (bs->bit_offset / 8);
  uint32_t i         = 0;
  int32_t  err       = EINVAL;

  *set_count = count;
  ps->count  = count > count_max ? count_max : count;
  while( i < ps->count ) {
    uint32_t rbsp_size;
    uint32_t rsize = mmf_bitstremgetbits(bs, 16);
    uint8_t  *rbsp = (uint8_t*)&bs->buff[bs->bit_offset/8];

    if( rsize > available ) {
      err = EINVAL;
      break;
    }

    if( (rbsp = nal_unit_extract_rbsp(rbsp, rsize, &rbsp_size)) == NULL ) {
      err = ENOMEM;
      LOG(LOG_ERROR, "%s Couldn't allocate %uytes", __func__, rsize);
      ps->count = 0;
    } else {
      err = 0;
      ps->buf[i].iov_len  = rbsp_size;
      ps->buf[i].iov_base = rbsp;
      bs->bit_offset     += rsize * 8;
    }
    i++;
  }
  return err;
}

static int32_t parse_hevc_decoder_cfg( const uint8_t *cfg, uint32_t cfg_len, hevc_decoder_specific_t *hvcc )
{
  uint32_t nal_type;
  uint32_t set_count;
  int32_t  err = ENOTSUP;

  if( cfg_len > 3 ) {
    if( cfg[0] || cfg[1] || cfg[2] > 1 ) {
      // its seems the cfg is in the hvcC format
      psbitstream_t bs  = {.buff = cfg, .buff_size = cfg_len, .bit_offset = 0};
      int32_t       num_ps;

      if( (hvcc->version = mmf_bitstremgetbits(&bs, 8)) > 1 )
        LOG(LOG_ERROR, "%s: seeing unsupported version=%d", __func__, hvcc->version);
      else {
        hvcc->general_profile_space              = mmf_bitstremgetbits(&bs, 2);     // the profile context
        hvcc->general_tier_flag                  = mmf_bitstremgetbits(&bs, 1);     // profile context flag
        hvcc->general_profile_idc                = mmf_bitstremgetbits(&bs, 5);     // the profile of the bitstream
        hvcc->general_profile_compatibility_flag = mmf_bitstremgetbits(&bs, 32);    // profile compatibility
        hvcc->progressive_source_flag            = mmf_bitstremgetbits(&bs, 1);     // Source is progressive
        hvcc->interlace_source_flag              = mmf_bitstremgetbits(&bs, 1);     // Source is interlaced
        hvcc->nonpacked_constraint_flag          = mmf_bitstremgetbits(&bs, 1);     //  1 == no frame packing arrangement SEI messages
        hvcc->frame_only_constraint_flag         = mmf_bitstremgetbits(&bs, 1);     //  1 == no fields
        // 44 bits Reserved field
        mmf_bitstremgetbits(&bs, 32);
        mmf_bitstremgetbits(&bs, 12);
        hvcc->general_level_idc                  = mmf_bitstremgetbits(&bs, 8);     // the level of the bitstream
        // 4 bits Reserved field
        mmf_bitstremgetbits(&bs, 4);
        hvcc->min_spatial_segmentation           = mmf_bitstremgetbits(&bs, 12);    // Maximum possible size of distinct coded spatial segmentation regions in the pictures of the CVS
        // 6 bits Reserved field
        mmf_bitstremgetbits(&bs, 6);
        hvcc->parallelism_type                   = mmf_bitstremgetbits(&bs, 2);     // parallelism_type: 0=unknown, 1=slices, 2=tiles, 3=WPP
        // 6 bits Reserved field
        mmf_bitstremgetbits(&bs, 6);
        hvcc->chroma_format                      = mmf_bitstremgetbits(&bs, 2);     // HEVC chroma_format, See table 6-1
        // 5 bits Reserved field
        mmf_bitstremgetbits(&bs, 5);
        hvcc->bit_depth_luma                     = mmf_bitstremgetbits(&bs, 3) + 8; // Bit depth luma
        // 5 bits Reserved field
        mmf_bitstremgetbits(&bs, 5);
        hvcc->bit_depth_chroma                   = mmf_bitstremgetbits(&bs, 3) + 8; // Bit depth chroma
        // 16 bits Reserved field: average  frame rate field
        hvcc->avg_frame_rate                     = mmf_bitstremgetbits(&bs, 16);
        // 2 bits Reserved field:  cosntant frame rate field
        hvcc->cst_frame_rate                     = mmf_bitstremgetbits(&bs, 2);
        hvcc->max_sub_layers                     = mmf_bitstremgetbits(&bs, 3);     // Maximum number of temporal sub-layers
        hvcc->temporal_id_nesting_flag           = mmf_bitstremgetbits(&bs, 1);     // Specifies whether inter prediction is additionally restricted
        hvcc->size_nalu                          = mmf_bitstremgetbits(&bs, 2) + 1; // Size of field NALU Length

        if( (num_ps = mmf_bitstremgetbits(&bs, 8)) ) {
          while( num_ps > 0 ) {
            parameter_set_t *ps;
            err = EINVAL;
            mmf_bitstremgetbits(&bs, 1); // array_completeness
            mmf_bitstremgetbits(&bs, 1); // reserved
            nal_type = mmf_bitstremgetbits(&bs, 6);

            if( nal_type == HEVC_NALU_TYPE_VIDEO_PARAM )
              ps = &hvcc->vps;
            else if( nal_type == HEVC_NALU_TYPE_SEQ_PARAM )
              ps = &hvcc->sps;
            else if(  nal_type == HEVC_NALU_TYPE_PIC_PARAM )
              ps = &hvcc->pps;
            else if( nal_type == HEVC_NALU_TYPE_PREFIX_SEI )
              ps = &hvcc->sei;
            else {
              LOG(LOG_ERROR, "%s: hvcc_read_parameter_set() seeing invalid nal_type=0x%x", __func__, nal_type);
              break;
            }

            if( (err = hvcc_read_parameter_set(&bs, ps, nal_type, &set_count)) ) {
              LOG(LOG_ERROR, "%s: hvcc_read_parameter_set() for nal_type=0x%x failed err = %d '%s'", __func__, nal_type, err, strerror(err));
              break;
            }
            num_ps -= set_count;
          }
        }
      }
    } else {
      // annexb format
      uint32_t      nal_size;
      int32_t       i_end = cfg_len;
      const uint8_t *p_cfg = cfg;
      while( i_end > 0 ) {
        if( (p_cfg = hvcc_next_annexb_nalu(p_cfg, i_end, &i_end, NULL)) == NULL )
           break;
        hvcc_next_annexb_nalu(p_cfg, i_end, NULL, &nal_size);
        nal_type = (p_cfg[0] & 0x7E) >> 1;
        if( nal_type != HEVC_NALU_TYPE_SEQ_PARAM ) {
          // ignore the nal, we are looking for the SPS to extact width and height
          p_cfg += nal_size;
          i_end -= nal_size;
        } else {
          uint32_t rbsp_size;
          uint8_t  *rbsp;
          /* skip the nal header:
          *  forbidden_zero_bit    u(1)
          *  nal_type              u(6)
          *  nuh_layer_id          u(6)
          *  nuh_temporal_id_plus1 u(3)
          */
          err = EINVAL;
          if( nal_size > 2 ) {
            p_cfg    += 2;
            nal_size -= 2;
            if( (rbsp = nal_unit_extract_rbsp(p_cfg, nal_size, &rbsp_size)) == NULL ) {
              err = ENOMEM;
              LOG(LOG_ERROR, "%s Couldn't allocate %uytes", __func__, nal_size);
            } else {
              err = EOK;
              hvcc->sps.buf[0].iov_len  = rbsp_size;
              hvcc->sps.buf[0].iov_base = rbsp;
            }
          }
          return err;
        }
      }
    }
  }
  return err;
}

static void hvcc_parse_profile_tier_level( psbitstream_t *bs, hevc_decoder_specific_t *hvcc, uint32_t max_sub_layers_minus1 )
{
  uint8_t  level_idc;
  uint8_t  tier_flag;
  uint8_t  profile_idc;
  uint32_t profile_compatibility_flag;
  uint8_t  sub_layer_profile_present_flag[8];
  uint8_t  sub_layer_level_present_flag[8];
  int32_t  i;

  hvcc->general_profile_space = mmf_bitstremgetbits(bs, 2);
  tier_flag                   = mmf_bitstremgetbits(bs, 1);
  profile_idc                 = mmf_bitstremgetbits(bs, 5);
  profile_compatibility_flag  = mmf_bitstremgetbits(bs, 32);
  // skip constraint_indicator_flags 48bits
  mmf_bitstremgetbits(bs, 32);
  mmf_bitstremgetbits(bs, 16);
  level_idc                   =  mmf_bitstremgetbits(bs, 8);

  if( tier_flag > hvcc->general_tier_flag )
    hvcc->general_level_idc = level_idc;
  else if( level_idc > hvcc->general_level_idc )
    hvcc->general_level_idc = level_idc;
  if( tier_flag > hvcc->general_tier_flag )
    hvcc->general_tier_flag = tier_flag;
  if( profile_idc > hvcc->general_profile_idc )
    hvcc->general_profile_idc = profile_idc;
  hvcc->general_profile_compatibility_flag &= profile_compatibility_flag;
  // skip sub_layer_profile_present_flags
  for( i = 0; i < (int32_t)max_sub_layers_minus1; i++ ) {
    sub_layer_profile_present_flag[i] = mmf_bitstremgetbits(bs, 1);
    sub_layer_level_present_flag[i]   = mmf_bitstremgetbits(bs, 1);
  }

  if( max_sub_layers_minus1 > 0 ) {
    for(i = max_sub_layers_minus1; i < 8; i++ )
      mmf_bitstremgetbits(bs, 2); // skip reserved_zero_2bits[i]
  }

  for( i = 0; i < (int32_t)max_sub_layers_minus1; i++ ) {
    // skip sub_layer profile data
    if( sub_layer_profile_present_flag[i] ) {
      // skip sub_layer profile data
      mmf_bitstremgetbits(bs, 32);
      mmf_bitstremgetbits(bs, 32);
      mmf_bitstremgetbits(bs, 24);
    }
    // skip sub_layer level data
    if( sub_layer_level_present_flag[i] )
       mmf_bitstremgetbits(bs, 8);
  }
}

static int32_t hvcc_parse_sps( hevc_decoder_specific_t *hvcc, decoder_info_t *info_out )
{
  // we parse the first available sps in the list
  parameter_set_t *sps = &hvcc->sps;
  psbitstream_t   bs   = {.buff = (const uint8_t*)sps->buf[0].iov_base, .buff_size = (uint32_t)sps->buf[0].iov_len, .bit_offset = 0};
  uint8_t         sps_max_sub_layers_minus1;
  uint32_t        pic_width_in_luma_samples;
  uint32_t        pic_height_in_luma_samples;
  uint32_t        sps_sub_layer_ordering_info_present_flag;
  uint32_t        log2_min_luma_coding_block_size_minus3;
  uint32_t        log2_diff_max_min_luma_coding_block_size;

  // skip 4bits: sps_video_parameter_set_id
  mmf_bitstremgetbits(&bs, 4);
  sps_max_sub_layers_minus1 = mmf_bitstremgetbits(&bs, 3);
  if( hvcc->max_sub_layers < (sps_max_sub_layers_minus1 + 1) )
     hvcc->max_sub_layers = sps_max_sub_layers_minus1 + 1;
  hvcc->temporal_id_nesting_flag =  mmf_bitstremgetbits(&bs, 1);
  hvcc_parse_profile_tier_level(&bs, hvcc, sps_max_sub_layers_minus1);
  // skip sps_seq_parameter_set_id
  mmf_golombunsigned(&bs);
  if( (hvcc->chroma_format = mmf_golombunsigned(&bs)) == 3 )
    mmf_bitstremgetbits(&bs, 1); //skip separate_colour_plane_flag

  pic_width_in_luma_samples  = mmf_golombunsigned(&bs);
  pic_height_in_luma_samples = mmf_golombunsigned(&bs);

  info_out->width  = pic_width_in_luma_samples;
  info_out->height = pic_height_in_luma_samples;

  // check the conformance_window_flag
  if( mmf_bitstremgetbits(&bs, 1) ) {
    uint32_t left_off   = mmf_golombunsigned(&bs); // conf_win_left_offset
    uint32_t right_off  = mmf_golombunsigned(&bs); // onf_win_right_offset
    uint32_t top_off    = mmf_golombunsigned(&bs); // conf_win_top_offset
    uint32_t bottom_off = mmf_golombunsigned(&bs); // conf_win_bottom_offset
    LOG(LOG_INFO, "%s: left_offset=%u right_offset=%u top_offset=%u bottom_offset=%u", __func__, left_off, right_off, top_off, bottom_off);
  }

  hvcc->bit_depth_luma   = mmf_golombunsigned(&bs) + 8; // Bit depth luma
  hvcc->bit_depth_chroma = mmf_golombunsigned(&bs) + 8; // Bit depth chroma
  mmf_golombunsigned(&bs); //log2_max_pic_order_cnt_lsb_minus4
  sps_sub_layer_ordering_info_present_flag = mmf_bitstremgetbits(&bs, 1);
  for(int i = (sps_sub_layer_ordering_info_present_flag ?
              0 : sps_max_sub_layers_minus1);
          i <= sps_max_sub_layers_minus1; ++i ) {
      mmf_golombunsigned(&bs); //sps_max_dec_pic_buffering_minus1
      mmf_golombunsigned(&bs); //sps_max_num_reorder_pics
      mmf_golombunsigned(&bs); //sps_max_latency_increase_plus1
  }
  log2_min_luma_coding_block_size_minus3 = mmf_golombunsigned(&bs);
  log2_diff_max_min_luma_coding_block_size = mmf_golombunsigned(&bs);

  uint32_t min_cblog2_size_y = log2_min_luma_coding_block_size_minus3 + 3;
  uint32_t ctb_log2size_y =
      min_cblog2_size_y + log2_diff_max_min_luma_coding_block_size;
  uint32_t ctb_size_y = 1 << ctb_log2size_y;

  if(ctb_size_y > 0) {
      hvcc->aligned_width  = (pic_width_in_luma_samples + ctb_size_y - 1) / ctb_size_y * ctb_size_y;
      hvcc->aligned_height = (pic_height_in_luma_samples + ctb_size_y - 1) / ctb_size_y * ctb_size_y;
  }
  else {
      return -1;
  }


  return 0;
}

void hevc_free_decoder_specific( hevc_decoder_specific_t *hvcc )
{
  if( hvcc ) {
    uint32_t i;
    uint32_t max = sizeof(hvcc->vps.buf) / sizeof(hvcc->vps.buf[0]);
    for( i = 0; i < max; i++ ) {
      free(hvcc->vps.buf[i].iov_base);
      free(hvcc->sps.buf[i].iov_base);
      free(hvcc->pps.buf[i].iov_base);
      free(hvcc->sei.buf[i].iov_base);
    }
    free(hvcc);
  }
}

int32_t hevc_parse_decoder_config( const uint8_t *cfg, uint32_t cfg_len, decoder_info_t *info_out, hevc_decoder_specific_t **parsed_hvcc )
{
  hevc_decoder_specific_t *hvcc;
  int32_t                 err = ENOMEM;

  if( (hvcc = (hevc_decoder_specific_t*)calloc(1, sizeof(*hvcc))) == NULL )
    LOG(LOG_ERROR, "%s Couldn't allocate %zuytes", __func__, sizeof(*hvcc));
  else {

    if( (err = parse_hevc_decoder_cfg(cfg, cfg_len, hvcc)) )
      LOG(LOG_ERROR, "%s: parse_hevc_decoder_cfg() failed err=%d '%s'", __func__, err, strerror(err));
    else if( (err = hvcc_parse_sps(hvcc, info_out)) )
      LOG(LOG_ERROR, "%s: hvcc_parse_sps() failed err=%d '%s'", __func__, err, strerror(err));
    else {
      // success
       LOG(LOG_DEBUG2, "%s: w=%d h=%d par_x=%d par_y=%d", __func__, info_out->width, info_out->height, info_out->par_x, info_out->par_y);
       if(parsed_hvcc == NULL)
           hevc_free_decoder_specific(hvcc);
       else
           *parsed_hvcc = hvcc;
      return err;
    }
    hevc_free_decoder_specific(hvcc);
  }
  return err;
}

