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
#ifndef CODEC_DEV_UTILS_H
#define CODEC_DEV_UTILS_H

#include "codec_dev_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief         Convert decibel value to register settings
 * @param         vol_range: Volume range
 * @param         db: Volume decibel
 * @return        Codec register value
 */
int audio_codec_calc_vol_reg(const codec_dev_vol_range_t *vol_range, float db);

/**
 * @brief         Convert codec register setting to decibel value
 * @param         vol_range: Volume range
 * @param         vol: Volume register setting
 * @return        Codec volume in decibel unit
 */
float audio_codec_calc_vol_db(const codec_dev_vol_range_t *vol_range, int vol);

#ifdef __cplusplus
}
#endif

#endif
