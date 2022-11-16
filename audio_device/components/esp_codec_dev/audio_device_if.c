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

#include <stdlib.h>
#include <string.h>
#include "audio_codec_if.h"
#include "audio_codec_ctrl_if.h"
#include "audio_codec_data_if.h"
#include "audio_codec_gpio_if.h"
#include "codec_dev_err.h"

static const audio_codec_gpio_if_t *render_gpio_if;

int audio_codec_delete_codec_if(const audio_codec_if_t *h)
{
    if (h) {
        int ret = 0;
        if (h->close) {
            ret = h->close(h);
        }
        free((void *) h);
        return ret;
    }
    return CODEC_DEV_INVALID_ARG;
}

int audio_codec_delete_ctrl_if(const audio_codec_ctrl_if_t *h)
{
    if (h) {
        int ret = 0;
        if (h->close) {
            ret = h->close(h);
        }
        free((void *) h);
        return ret;
    }
    return CODEC_DEV_INVALID_ARG;
}

int audio_codec_delete_data_if(const audio_codec_data_if_t *h)
{
    if (h) {
        int ret = 0;
        if (h->close) {
            ret = h->close(h);
        }
        free((void *) h);
        return ret;
    }
    return CODEC_DEV_INVALID_ARG;
}

int audio_codec_delete_gpio_if(const audio_codec_gpio_if_t *gpio_if)
{
    if (gpio_if) {
        free((void *) gpio_if);
        return CODEC_DEV_OK;
    }
    return CODEC_DEV_INVALID_ARG;
}

void audio_codec_set_gpio_if(const audio_codec_gpio_if_t *gpio_if)
{
    render_gpio_if = gpio_if;
}

const audio_codec_gpio_if_t *audio_codec_get_gpio_if()
{
    return render_gpio_if;
}
