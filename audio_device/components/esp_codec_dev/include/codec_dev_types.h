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
#ifndef CODEC_DEV_TYPES_H
#define CODEC_DEV_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Codec Device type
 */
typedef enum {
    CODEC_DEV_TYPE_NONE,
    CODEC_DEV_TYPE_IN = (1 << 0),                                     /*!< Codec input device like microphone */
    CODEC_DEV_TYPE_OUT = (1 << 1),                                    /*!< Codec input device like speaker */
    CODEC_DEV_TYPE_IN_OUT = (CODEC_DEV_TYPE_IN | CODEC_DEV_TYPE_OUT), /*!< Codec input and output device */
} codec_dev_type_t;

/**
 * @brief Codec audio sample information
 */
typedef struct {
    uint8_t  bits_per_sample; /*!< Bit lengths of one channel data */
    uint8_t  channel;         /*!< Channels of sample */
    uint32_t sample_rate;     /*!< Sample rate of sample */
} codec_sample_info_t;

/**
 * @brief Codec volume map to decibel
 */
typedef struct {
    int   vol;      /*!< Volume value */
    float db_value; /*!< Volume decibel value */
} codec_dev_vol_map_t;

/**
 * @brief Codec volume range setting
 */
typedef struct {
    codec_dev_vol_map_t min_vol; /*!< Minimum volume setting */
    codec_dev_vol_map_t max_vol; /*!< Maximum volume setting */
} codec_dev_vol_range_t;

/**
 * @brief Codec I2C configuration
 */
typedef struct {
    uint8_t port;  /*!< I2C port, this port need pre-installed by other modules */
    uint8_t addr;  /*!< I2C address, default address can be gotten from codec head files */
} codec_i2c_dev_cfg_t;

/**
 * @brief Codec I2S configuration
 */
typedef struct {
    uint8_t port; /*!< I2S port, this port need pre-installed by other modules */
} codec_i2s_dev_cfg_t;

/**
 * @brief Codec SPI configuration
 */
typedef struct {
    uint8_t spi_port; /*!< SPI port, this port need pre-installed by other modules */
    int16_t cs_pin;   /*!< SPI CS GPIO pin setting */
} codec_spi_dev_cfg_t;

/**
 * @brief Codec working mode
 */
typedef enum {
    CODEC_WORK_MODE_NONE,
    CODEC_WORK_MODE_ADC = (1 << 0), /*!< Enable ADC, only support input */
    CODEC_WORK_MODE_DAC = (1 << 1), /*!< Enable DAC, only support output */
    CODEC_WORK_MODE_BOTH = (CODEC_WORK_MODE_ADC | CODEC_WORK_MODE_DAC), /*!< Support both DAC and ADC */
    CODEC_WORK_MODE_LINE = (1 << 2),  /*!< Line mode */
} codec_work_mode_t;

#ifdef __cplusplus
}
#endif

#endif
