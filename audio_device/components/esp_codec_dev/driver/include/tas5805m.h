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

#ifndef _TAS5805M_H_
#define _TAS5805M_H_

#define TAS5805M_CODEC_DEFAULT_ADDR (0x5c)

#include "audio_codec_if.h"
#include "audio_codec_ctrl_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TAS5805M codec configuration
 */
typedef struct {
    const audio_codec_ctrl_if_t *ctrl_if;     /*!< Codec Control interface */
    codec_work_mode_t            codec_mode;  /*!< Codec work mode: ADC or DAC */
    bool                         master_mode; /*!< Whether codec works as I2S master or not */
    int16_t                      reset_pin;   /*!< Reset pin */
} tas5805m_codec_cfg_t;

/**
 * @brief         New TAS5805M codec interface
 * @param         codec_cfg: TAS5805M codec configuration
 * @return        NULL: Fail to new TAS5805M codec interface
 *                -Others: TAS5805M codec interface
 */
const audio_codec_if_t *tas5805m_codec_new(tas5805m_codec_cfg_t *codec_cfg);

#ifdef __cplusplus
}
#endif

#endif
