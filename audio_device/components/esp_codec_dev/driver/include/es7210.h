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
#ifndef __ES7210_H__
#define __ES7210_H__

#include "audio_codec_if.h"
#include "audio_codec_ctrl_if.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ES7210_CODEC_DEFAULT_ADDR (0x80)

#define ES7120_SEL_MIC1           (uint8_t)(1 << 0)
#define ES7120_SEL_MIC2           (uint8_t)(1 << 1)
#define ES7120_SEL_MIC3           (uint8_t)(1 << 2)
#define ES7120_SEL_MIC4           (uint8_t)(1 << 3)

/**
 * @brief ES7210 codec configuration, only support ADC feature
 */
typedef struct {
    const audio_codec_ctrl_if_t *ctrl_if;      /*!< Codec Control interface */
    bool                         master_mode;  /*!< Whether codec works as I2S master or not */
    uint8_t                      mic_selected; /*!< Selected microphone */
} es7210_codec_cfg_t;

/**
 * @brief         New ES7210 codec interface
 * @param         codec_cfg: ES7210 codec configuration
 * @return        NULL: Fail to new ES7210 codec interface
 *                -Others: ES7210 codec interface
 */
const audio_codec_if_t *es7210_codec_new(es7210_codec_cfg_t *codec_cfg);

#ifdef __cplusplus
}
#endif

#endif
