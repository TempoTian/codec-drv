
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
#ifndef AUDIO_CODEC_DATA_IF_H
#define AUDIO_CODEC_DATA_IF_H

#include "codec_dev_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audio_codec_data_if_t audio_codec_data_if_t;

/**
 * @brief Audio codec data interface structure
 */
struct audio_codec_data_if_t {
    int (*open)(const audio_codec_data_if_t *h, void *data_cfg, int cfg_size); /*!< Open data interface */
    bool (*is_open)(const audio_codec_data_if_t *h);                           /*!< Check whether data interface is opened */
    int (*set_fmt)(const audio_codec_data_if_t *h, codec_sample_info_t *fs);   /*!< Set audio format to data interface */
    int (*read)(const audio_codec_data_if_t *h, uint8_t *data, int size);      /*!< Read data from data interface */
    int (*write)(const audio_codec_data_if_t *h, uint8_t *data, int size);     /*!< Write data to data interface */
    int (*close)(const audio_codec_data_if_t *h);                              /*!< Close data interface */
};

/**
 * @brief         Delete codec data interface
 * @param         data_if: Codec data interface
 * @return        CODEC_DEV_OK: Delete success
 *                CODEC_DEV_INVALID_ARG: Input is NULL pointer        
 */
int audio_codec_delete_data_if(const audio_codec_data_if_t *data_if);

#ifdef __cplusplus
}
#endif

#endif
