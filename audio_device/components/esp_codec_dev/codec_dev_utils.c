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

#include <math.h>
#include "codec_dev_utils.h"

int audio_codec_calc_vol_reg(const codec_dev_vol_range_t *vol_range, float db)
{
    if (vol_range->max_vol.db_value == vol_range->min_vol.db_value) {
        return vol_range->max_vol.vol;
    }
    if (db >= vol_range->max_vol.db_value) {
        return vol_range->max_vol.vol;
    }
    if (db <= vol_range->min_vol.db_value) {
        return vol_range->min_vol.vol;
    }
    float ratio =
        (vol_range->max_vol.vol - vol_range->min_vol.vol) / (vol_range->max_vol.db_value - vol_range->min_vol.db_value);
    return (int) ((db - vol_range->min_vol.db_value) * ratio + vol_range->min_vol.vol);
}

float audio_codec_calc_vol_db(const codec_dev_vol_range_t *vol_range, int vol)
{
    if (vol_range->max_vol.vol == vol_range->min_vol.vol) {
        return vol_range->max_vol.db_value;
    }
    if (vol_range->max_vol.vol > vol_range->min_vol.vol) {
        if (vol >= vol_range->max_vol.vol) {
            return vol_range->max_vol.db_value;
        }
        if (vol <= vol_range->min_vol.vol) {
            return vol_range->min_vol.db_value;
        }
    } else {
        if (vol <= vol_range->max_vol.vol) {
            return vol_range->max_vol.db_value;
        }
        if (vol >= vol_range->min_vol.vol) {
            return vol_range->min_vol.db_value;
        }
    }
    float ratio =
        (vol_range->max_vol.db_value - vol_range->min_vol.db_value) / (vol_range->max_vol.vol - vol_range->min_vol.vol);
    return ((vol - vol_range->min_vol.vol) * ratio + vol_range->min_vol.db_value);
}
