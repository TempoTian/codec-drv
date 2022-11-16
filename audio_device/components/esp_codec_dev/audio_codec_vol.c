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
#include "codec_dev_types.h"
#include "audio_codec_vol.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GAIN_0DB_SHIFT (15)

typedef struct {
    codec_sample_info_t fs;
    uint16_t            gain;
    int                 cur;
    int                 step;
    int                 block_size;
    int                 duration;
} audio_vol_t;

void audio_codec_sw_vol_close(audio_codec_vol_handle_t h)
{
    if (h) {
        free(h);
    }
}

audio_codec_vol_handle_t audio_codec_sw_vol_open(codec_sample_info_t *fs, int duration)
{
    audio_vol_t *vol = calloc(1, sizeof(audio_vol_t));
    if (vol == NULL) {
        return NULL;
    }
    vol->fs = *fs;
    vol->block_size = (vol->fs.bits_per_sample * vol->fs.channel) >> 3;
    vol->cur = vol->gain = (1 << GAIN_0DB_SHIFT);
    vol->duration = duration;
    return (audio_codec_vol_handle_t) vol;
}

int audio_codec_sw_vol_process(audio_codec_vol_handle_t h, uint8_t *in, int len, uint8_t *out, int out_len)
{
    audio_vol_t *vol = (audio_vol_t *) h;
    if (vol == NULL) {
        return -1;
    }
    int sample = len / vol->block_size;
    if (vol->fs.bits_per_sample == 16) {
        int16_t *v_in = (int16_t *) in;
        int16_t *v_out = (int16_t *) out;
        if (vol->cur == vol->gain) {
            if (vol->gain == 0) {
                memset(out, 0, len);
                return 0;
            } else {
                for (int i = 0; i < sample; i++) {
                    for (int j = 0; j < vol->fs.channel; j++) {
                        *(v_out++) = ((*v_in++) * vol->cur) >> GAIN_0DB_SHIFT;
                    }
                }
                return 0;
            }
        }
        for (int i = 0; i < sample; i++) {
            for (int j = 0; j < vol->fs.channel; j++) {
                *(v_out++) = ((*v_in++) * vol->cur) >> GAIN_0DB_SHIFT;
            }
            if (vol->step) {
                vol->cur += vol->step;
                if (vol->step > 0) {
                    if (vol->cur > vol->gain) {
                        vol->cur = vol->gain;
                        vol->step = 0;
                    }
                } else {
                    if (vol->cur < vol->gain) {
                        vol->cur = vol->gain;
                        vol->step = 0;
                    }
                }
            }
        }
    }
    return 0;
}

int audio_codec_sw_vol_set(audio_codec_vol_handle_t h, float db_value)
{
    audio_vol_t *vol = (audio_vol_t *) h;
    if (vol == NULL) {
        return -1;
    }
    int gain;
    if (db_value <= -96.0) {
        gain = 0;
    } else {
        gain = (int) (exp(db_value / 20 * log(10)) * (1 << GAIN_0DB_SHIFT));
    }
    vol->gain = gain;
    float step = (float) (vol->gain - vol->cur) * 1000 / vol->duration / vol->fs.sample_rate;
    vol->step = (int) step;
    if (step == 0) {
        vol->cur = vol->gain;
    }
    return 0;
}
