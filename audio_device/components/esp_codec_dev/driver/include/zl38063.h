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

#ifndef __ZL38063_H__
#define __ZL38063_H__

#include "audio_codec_if.h"
#include "audio_codec_ctrl_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ZL38063 codec configuration
 *        Notes: ZL38063 codec driver provide default configuration of I2S settings in firmware.
 *               Defaults are 48khz, 16 bits, 2 channels
 *               To playback other sample rate need do resampling firstly
 */
typedef struct {
    const audio_codec_ctrl_if_t *ctrl_if;    /*!< Codec Control interface */
    codec_work_mode_t            codec_mode; /*!< Codec work mode: ADC or DAC */
    int16_t                      pa_pin;     /*!< PA chip power pin */
    int16_t                      reset_pin;  /*!< Reset pin */
} zl38063_codec_cfg_t;

/**
 * @brief         New ZL38063 codec interface
 * @param         codec_cfg: ZL38063 codec configuration
 * @return        NULL: Fail to new ZL38063 codec interface
 *                -Others: ZL38063 codec interface
 */
const audio_codec_if_t *zl38063_codec_new(zl38063_codec_cfg_t *codec_cfg);

#ifdef __cplusplus
}
#endif

#endif
