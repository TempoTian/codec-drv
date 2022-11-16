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
#ifndef __ES8311_H__
#define __ES8311_H__

#include "audio_codec_if.h"
#include "audio_codec_ctrl_if.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ES8311_CODEC_DEFAULT_ADDR (0x30)

/**
 * @brief ES8311 codec configuration
 */
typedef struct {
    const audio_codec_ctrl_if_t *ctrl_if;     /*!< Codec Control interface */
    codec_work_mode_t            codec_mode;  /*!< Codec work mode: ADC or DAC */
    int16_t                      pa_pin;      /*!< PA chip power pin */
    bool                         master_mode; /*!< Whether codec works as I2S master or not */
    bool                         use_mclk;    /*!< Whether use MCLK clock */
    bool                         digital_mic; /*!< Whether use digital microphone */
    bool                         invert_mclk; /*!< MCLK clock signal inverted or not */
    bool                         invert_sclk; /*!< SCLK clock signal inverted or not */
} es8311_codec_cfg_t;

/**
 * @brief         New ES8311 codec interface
 * @param         codec_cfg: ES8311 codec configuration
 * @return        NULL: Fail to new ES8311 codec interface
 *                -Others: ES8311 codec interface
 */
const audio_codec_if_t *es8311_codec_new(es8311_codec_cfg_t *codec_cfg);

#ifdef __cplusplus
}
#endif

#endif
