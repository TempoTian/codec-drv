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

#include "audio_codec_gpio_if.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "codec_dev_err.h"
#include <string.h>

static int _gpio_cfg(int16_t gpio, audio_gpio_dir_t dir, audio_gpio_mode_t mode)
{
    if (gpio == -1) {
        return CODEC_DEV_INVALID_ARG;
    }
    gpio_config_t io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.mode = (dir == AUDIO_GPIO_DIR_OUT ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
    io_conf.pin_bit_mask = BIT64(gpio);
    io_conf.pull_down_en = ((mode & AUDIO_GPIO_MODE_PULL_DOWN) != 0);
    io_conf.pull_up_en = ((mode & AUDIO_GPIO_MODE_PULL_UP) != 0);
    esp_err_t ret = 0;
    ret |= gpio_config(&io_conf);
    return ret == 0 ? CODEC_DEV_OK : CODEC_DEV_DRV_ERR;
}
static int _gpio_set(int16_t gpio, bool high)
{
    if (gpio == -1) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = gpio_set_level((gpio_num_t) gpio, high ? 1 : 0);
    return ret == 0 ? CODEC_DEV_OK : CODEC_DEV_DRV_ERR;
}

static bool _gpio_get(int16_t gpio)
{
    if (gpio == -1) {
        return false;
    }
    return (bool) gpio_get_level((gpio_num_t) gpio);
}

const audio_codec_gpio_if_t *audio_codec_new_gpio_if()
{
    audio_codec_gpio_if_t *gpio_if = (audio_codec_gpio_if_t *) calloc(1, sizeof(audio_codec_gpio_if_t));
    if (gpio_if == NULL) {
        return NULL;
    }
    gpio_if->setup = _gpio_cfg;
    gpio_if->set = _gpio_set;
    gpio_if->get = _gpio_get;
    return gpio_if;
}
