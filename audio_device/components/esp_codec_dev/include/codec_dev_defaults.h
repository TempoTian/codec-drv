
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
#ifndef CODEC_DEV_DEFAULTS_H
#define CODEC_DEV_DEFAULTS_H

#include "audio_codec_if.h"
#include "audio_codec_ctrl_if.h"
#include "audio_codec_data_if.h"
#include "audio_codec_gpio_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief         Get default codec GPIO interface
 * @return        Codec GPIO interface
 */
const audio_codec_gpio_if_t *audio_codec_new_gpio_if();

/**
 * @brief         Get default SPI control interface
 * @return        SPI control interface
 */
const audio_codec_ctrl_if_t *audio_codec_new_spi_ctrl_if(codec_spi_dev_cfg_t *spi_cfg);

/**
 * @brief         Get default I2C control interface
 * @return        I2C control interface
 */
const audio_codec_ctrl_if_t *audio_codec_new_i2c_ctrl_if(codec_i2c_dev_cfg_t *i2c_cfg);

/**
 * @brief         Get default I2S data interface
 * @return        I2S data interface
 */
const audio_codec_data_if_t *audio_codec_new_i2s_data_if(codec_i2s_dev_cfg_t *i2s_cfg);

#ifdef __cplusplus
}
#endif

#endif
