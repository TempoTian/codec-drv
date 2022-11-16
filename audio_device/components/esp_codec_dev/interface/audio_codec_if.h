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
#ifndef AUDIO_CODEC_IF_H
#define AUDIO_CODEC_IF_H

#include "audio_codec_ctrl_if.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audio_codec_if_t audio_codec_if_t;

/**
 * @brief Structure for codec interface
 */
struct audio_codec_if_t {
    int (*open)(const audio_codec_if_t *h, void *cfg, int cfg_size);   /*!< Open codec */
    bool (*is_open)(const audio_codec_if_t *h);                        /*!< Check whether codec is opened */
    int (*enable)(const audio_codec_if_t *h, bool enable);             /*!< Enable codec, when codec disabled it can use less power if provided */
    int (*set_fs)(const audio_codec_if_t *h, codec_sample_info_t *fs); /*!< Set audio format to codec */
    int (*mute)(const audio_codec_if_t *h, bool mute);                 /*!< Mute and un-mute DAC output */
    int (*set_vol)(const audio_codec_if_t *h, float db);               /*!< Set DAC volume in decibel */
    int (*set_mic_gain)(const audio_codec_if_t *h, float db);          /*!< Set microphone gain in decibel */
    int (*mute_mic)(const audio_codec_if_t *h, bool mute);             /*!< Mute and un-mute microphone */
    int (*set_reg)(const audio_codec_if_t *h, int reg, int value);     /*!< Set register value to codec */
    int (*get_reg)(const audio_codec_if_t *h, int reg, int *value);    /*!< Get register value from codec */
    int (*close)(const audio_codec_if_t *h);                           /*!< Close codec */
};

/**
 * @brief         Delete codec interface
 * @param         codec_if: Codec interface
 * @return        CODEC_DEV_OK: Delete success
 *                CODEC_DEV_INVALID_ARG: Input is NULL pointer         
 */
int audio_codec_delete_codec_if(const audio_codec_if_t *codec_if);

#ifdef __cplusplus
}
#endif

#endif
