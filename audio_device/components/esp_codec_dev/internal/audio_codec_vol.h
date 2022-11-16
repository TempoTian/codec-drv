/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef AUDIO_CODEC_VOL_H
#define AUDIO_CODEC_VOL_H

#include "codec_dev_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *audio_codec_vol_handle_t;

/**
 * @brief         Open software volume module
 *                Notes: currently only support 16bits input
 * @param         fs: Audio sample information
 * @param         duration: Fade in and fade out duration if volume change too quick
 * @return        NULL: Memory not enough
 *                -Others: Handle for software volume setting        
 */
audio_codec_vol_handle_t audio_codec_sw_vol_open(codec_sample_info_t *fs, int duration);

/**
 * @brief         Set volume to software volume module
 * @param         h: Software volume handle
 * @param         db_value: Volume in decibel
 * @return        0: On success
 *                -1: Wrong handle   
 */
int audio_codec_sw_vol_set(audio_codec_vol_handle_t h, float db_value);

/**
 * @brief         Do volume process
 * @param         h: Software volume handle
 * @param         in: Input audio sample need aligned to sample
 * @param         len: Input sample length
 * @param         out: Output audio sample can be same as `in`
 * @param         out_len: Output sample length
 * @return        0: On success
 *                -1: Wrong handle   
 */
int audio_codec_sw_vol_process(audio_codec_vol_handle_t h, uint8_t *in, int len, uint8_t *out, int out_len);

/**
 * @brief         Close software volume module
 * @param         h: Software volume handle 
 */
void audio_codec_sw_vol_close(audio_codec_vol_handle_t h);

#ifdef __cplusplus
}
#endif

#endif
