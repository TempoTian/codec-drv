
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
#ifndef AUDIO_CODEC_GPIO_IF_H
#define AUDIO_CODEC_GPIO_IF_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO mode
 */
typedef enum {
    AUDIO_GPIO_MODE_FLOAT,                /*!< Float */
    AUDIO_GPIO_MODE_PULL_UP = (1 << 0),   /*!< Internally pullup */
    AUDIO_GPIO_MODE_PULL_DOWN = (1 << 1), /*!< Internally pulldown */
} audio_gpio_mode_t;

/**
 * @brief GPIO drive type
 */
typedef enum {
    AUDIO_GPIO_DIR_OUT, /*!< Output GPIO */
    AUDIO_GPIO_DIR_IN,  /*!< input GPIO */
} audio_gpio_dir_t;

/**
 * @brief Codec GPIO interface structure
 */
typedef struct {
    int (*setup)(int16_t gpio, audio_gpio_dir_t dir, /*!< Setup GPIO */
                 audio_gpio_mode_t mode);
    int (*set)(int16_t gpio, bool high);             /*!< Set GPIO level */
    bool (*get)(int16_t gpio);                       /*!< Get GPIO level */
} audio_codec_gpio_if_t;

/**
 * @brief         Delete GPIO interface
 * @param         gpio_if: GPIO interface
 * @return        CODEC_DEV_OK: Delete success
 *                CODEC_DEV_INVALID_ARG: Input is NULL pointer         
 */
int audio_codec_delete_gpio_if(const audio_codec_gpio_if_t *gpio_if);

#ifdef __cplusplus
}
#endif

#endif
