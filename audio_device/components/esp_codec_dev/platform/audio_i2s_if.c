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

#include "audio_codec_data_if.h"
#include "stdlib.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "codec_dev_err.h"
#include <string.h>

#define TAG "I2S_IF"

typedef struct {
    audio_codec_data_if_t base;
    bool                  is_open;
    uint8_t               port;
    codec_sample_info_t   fs;
} i2s_data_t;

int _i2s_data_open(const audio_codec_data_if_t *h, void *data_cfg, int cfg_size)
{
    i2s_data_t *i2s_data = (i2s_data_t *) h;
    if (h == NULL || data_cfg == NULL || cfg_size != sizeof(codec_i2s_dev_cfg_t)) {
        return CODEC_DEV_INVALID_ARG;
    }
    codec_i2s_dev_cfg_t *i2s_cfg = (codec_i2s_dev_cfg_t *) data_cfg;
    i2s_data->is_open = true;
    i2s_data->port = i2s_cfg->port;
    return CODEC_DEV_OK;
}

bool _i2s_data_is_open(const audio_codec_data_if_t *h)
{
    i2s_data_t *i2s_data = (i2s_data_t *) h;
    if (i2s_data) {
        return i2s_data->is_open;
    }
    return false;
}

int _i2s_data_set_fmt(const audio_codec_data_if_t *h, codec_sample_info_t *fs)
{
    i2s_data_t *i2s_data = (i2s_data_t *) h;
    if (i2s_data == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (i2s_data->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    if (memcmp(&i2s_data->fs, fs, sizeof(codec_sample_info_t))) {
        ESP_LOGI(TAG, "I2S %d sample rate:%d channel:%d bits:%d", i2s_data->port, fs->sample_rate, fs->channel,
                 fs->bits_per_sample);
        i2s_set_clk(i2s_data->port, fs->sample_rate, fs->bits_per_sample, fs->channel);
        i2s_zero_dma_buffer(i2s_data->port);
        memcpy(&i2s_data->fs, fs, sizeof(codec_sample_info_t));
    }
    return -1;
}

int _i2s_data_read(const audio_codec_data_if_t *h, uint8_t *data, int size)
{
    i2s_data_t *i2s_data = (i2s_data_t *) h;
    if (i2s_data == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (i2s_data->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    size_t bytes_read = 0;
    int ret = i2s_read(i2s_data->port, data, size, &bytes_read, portMAX_DELAY);
    return ret == 0 ? CODEC_DEV_OK : CODEC_DEV_DRV_ERR;
}

int _i2s_data_write(const audio_codec_data_if_t *h, uint8_t *data, int size)
{
    i2s_data_t *i2s_data = (i2s_data_t *) h;
    if (i2s_data == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (i2s_data->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    size_t bytes_written = 0;
    int ret = i2s_write(i2s_data->port, data, size, &bytes_written, portMAX_DELAY);
    return ret == 0 ? CODEC_DEV_OK : CODEC_DEV_DRV_ERR;
}

int _i2s_data_close(const audio_codec_data_if_t *h)
{
    i2s_data_t *i2s_data = (i2s_data_t *) h;
    if (i2s_data == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    memset(&i2s_data->fs, 0, sizeof(codec_sample_info_t));
    i2s_data->is_open = false;
    return CODEC_DEV_OK;
}

const audio_codec_data_if_t *audio_codec_new_i2s_data_if(codec_i2s_dev_cfg_t *i2s_cfg)
{
    i2s_data_t *i2s_data = calloc(1, sizeof(i2s_data_t));
    if (i2s_data == NULL) {
        return NULL;
    }
    i2s_data->base.open = _i2s_data_open;
    i2s_data->base.is_open = _i2s_data_is_open;
    i2s_data->base.read = _i2s_data_read;
    i2s_data->base.write = _i2s_data_write;
    i2s_data->base.set_fmt = _i2s_data_set_fmt;
    i2s_data->base.close = _i2s_data_close;
    int ret = _i2s_data_open(&i2s_data->base, i2s_cfg, sizeof(codec_i2s_dev_cfg_t));
    if (ret != 0) {
        free(i2s_data);
        return NULL;
    }
    return &i2s_data->base;
}
