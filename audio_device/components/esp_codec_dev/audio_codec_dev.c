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
#include <math.h>
#include <string.h>
#include "esp_codec_dev.h"
#include "audio_codec_if.h"
#include "audio_codec_data_if.h"
#include "codec_dev_err.h"
#include "audio_codec_vol.h"
#include "esp_log.h"

#define TAG                 "Adev_Codec"

#define VOL_TRANSITION_TIME (50)

typedef struct {
    const audio_codec_if_t      *codec_if;
    const audio_codec_data_if_t *data_if;
    codec_dev_type_t             dev_caps;
    bool                         input_opened;
    bool                         output_opened;
    int                          volume;
    float                        mic_gain;
    bool                         muted;
    bool                         mic_muted;
    float                        hw_gain_db;
    audio_codec_vol_handle_t     sw_vol;
    esp_codec_dev_vol_curve_t    vol_curve;
} codec_dev_t;

static bool _verify_codec_ready(codec_dev_t *dev)
{
    if (dev->codec_if && dev->codec_if->is_open) {
        if (dev->codec_if->is_open(dev->codec_if) == false) {
            return false;
        }
    }
    return true;
}

static bool _verify_drv_ready(codec_dev_t *dev, bool playback)
{
    if (_verify_codec_ready(dev) == false) {
        ESP_LOGE(TAG, "Codec is not open yet");
        return false;
    }
    if (dev->data_if->is_open && dev->data_if->is_open(dev->data_if) == false) {
        ESP_LOGE(TAG, "Codec data interface not open");
        return false;
    }
    if (playback && dev->data_if->write == NULL) {
        ESP_LOGE(TAG, "Need provide write API");
        return false;
    }
    if (playback == false && dev->data_if->read == NULL) {
        ESP_LOGE(TAG, "Need provide read API");
        return false;
    }
    return true;
}

static int _verify_codec_setting(codec_dev_t *dev, bool playback)
{
    if ((playback && (dev->dev_caps & CODEC_DEV_TYPE_OUT) == 0) ||
        (!playback && (dev->dev_caps & CODEC_DEV_TYPE_IN) == 0)) {
        return CODEC_DEV_NOT_SUPPORT;
    }
    if (_verify_codec_ready(dev) == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return CODEC_DEV_OK;
}

static int _get_default_vol_curve(esp_codec_dev_vol_curve_t *curve)
{
    curve->vol_map = (codec_dev_vol_map_t *) malloc(2 * sizeof(codec_dev_vol_map_t));
    if (curve->vol_map) {
        curve->count = 2;
        curve->vol_map[0].vol = 0;
        curve->vol_map[0].db_value = -50.0;
        curve->vol_map[1].vol = 100;
        curve->vol_map[1].db_value = 0.0;
    }
    return CODEC_DEV_OK;
}

static float _get_vol_db(esp_codec_dev_vol_curve_t *curve, int vol)
{
    if (vol == 0) {
        return -96.0;
    }
    int n = curve->count;
    if (n == 0) {
        return 0.0;
    }
    if (vol >= curve->vol_map[n - 1].vol) {
        return curve->vol_map[n - 1].db_value;
    }
    for (int i = 0; i < n - 1; i++) {
        if (vol < curve->vol_map[i + 1].vol) {
            if (curve->vol_map[i].vol != curve->vol_map[i + 1].vol) {
                float ratio = (curve->vol_map[i + 1].db_value - curve->vol_map[i].db_value) /
                              (curve->vol_map[i + 1].vol - curve->vol_map[i].vol);
                return curve->vol_map[i].db_value + (vol - curve->vol_map[i].vol) * ratio;
            }
            break;
        }
    }
    return 0.0;
}

esp_codec_dev_handle_t esp_codec_dev_new(esp_codec_dev_cfg_t *cfg)
{
    if (cfg == NULL || cfg->data_if == NULL || cfg->dev_type == CODEC_DEV_TYPE_NONE) {
        return NULL;
    }
    codec_dev_t *dev = (codec_dev_t *) calloc(1, sizeof(codec_dev_t));
    if (dev == NULL) {
        return NULL;
    }
    dev->dev_caps = cfg->dev_type;
    dev->codec_if = cfg->codec_if;
    dev->data_if = cfg->data_if;
    if (cfg->dev_type & CODEC_DEV_TYPE_OUT) {
        _get_default_vol_curve(&dev->vol_curve);
    }
    return (esp_codec_dev_handle_t) dev;
}

int esp_codec_dev_open(esp_codec_dev_handle_t handle, codec_sample_info_t *fs)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL || fs == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (dev->input_opened || dev->output_opened) {
        ESP_LOGI(TAG, "Input already open");
        return CODEC_DEV_OK;
    }
    if ((dev->dev_caps & CODEC_DEV_TYPE_IN)) {
        // check record
        if (_verify_drv_ready(dev, false) == false) {
            ESP_LOGE(TAG, "Codec not support input");
        } else {
            dev->input_opened = true;
        }
    }
    if ((dev->dev_caps & CODEC_DEV_TYPE_OUT)) {
        // check record
        if (_verify_drv_ready(dev, true) == false) {
            ESP_LOGE(TAG, "Codec not support output");
        } else {
            dev->output_opened = true;
        }
    }
    // if open failed
    if (dev->input_opened == false && dev->output_opened == false) {
        return CODEC_DEV_NOT_SUPPORT;
    }
    const audio_codec_if_t *codec = dev->codec_if;
    if (codec) {
        // TODO not set codec fs
        if (0 && codec->set_fs) {
            if (codec->set_fs(codec, fs) != 0) {
                return CODEC_DEV_NOT_SUPPORT;
            }
        }
        if (codec->enable) {
            codec->enable(codec, true);
        }
    }
    const audio_codec_data_if_t *data_if = dev->data_if;
    if (data_if->set_fmt) {
        data_if->set_fmt(data_if, fs);
    }
    if (dev->output_opened) {
        if (codec == NULL || codec->set_vol == NULL) {
            dev->sw_vol = audio_codec_sw_vol_open(fs, VOL_TRANSITION_TIME);
        }
    }
    ESP_LOGI(TAG, "open audio_device_codec OK\n");
    return CODEC_DEV_OK;
}

int esp_codec_dev_read(esp_codec_dev_handle_t handle, void *data, int len)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL || data == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (dev->input_opened == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    const audio_codec_data_if_t *data_if = dev->data_if;
    if (data_if->read) {
        return data_if->read(data_if, (uint8_t *) data, len);
    }
    return CODEC_DEV_NOT_SUPPORT;
}

int esp_codec_dev_write(esp_codec_dev_handle_t handle, void *data, int len)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL || data == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (dev->output_opened == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    const audio_codec_data_if_t *data_if = dev->data_if;
    if (data_if->write) {
        // soft volume process firstly
        if (dev->sw_vol) {
            audio_codec_sw_vol_process(dev->sw_vol, (uint8_t *) data, len, (uint8_t *) data, len);
        }
        return data_if->write(data_if, (uint8_t *) data, len);
    }
    return CODEC_DEV_NOT_SUPPORT;
}

int esp_codec_dev_set_hw_gain(esp_codec_dev_handle_t handle, esp_codec_dev_hw_gain_t *hw_gain)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (hw_gain->pa_voltage == 0.0) {
        hw_gain->pa_voltage = 5.0;
    }
    if (hw_gain->codec_dac_voltage == 0.0) {
        hw_gain->codec_dac_voltage = 3.3;
    }
    dev->hw_gain_db = 20 * log10(hw_gain->codec_dac_voltage / hw_gain->pa_voltage) + hw_gain->pa_gain;
    ESP_LOGI(TAG, "Calc hw_gain %f\n", dev->hw_gain_db);
    return CODEC_DEV_OK;
}

int esp_codec_dev_set_vol_curve(esp_codec_dev_handle_t handle, esp_codec_dev_vol_curve_t *curve)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL || curve == NULL || curve->vol_map == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = _verify_codec_setting(dev, true);
    if (ret != CODEC_DEV_OK) {
        return ret;
    }
    int size = curve->count * sizeof(codec_dev_vol_map_t);
    codec_dev_vol_map_t *new_map = (codec_dev_vol_map_t *) realloc(dev->vol_curve.vol_map, size);
    if (new_map == NULL) {
        return CODEC_DEV_NO_MEM;
    }
    dev->vol_curve.vol_map = new_map;
    memcpy(dev->vol_curve.vol_map, curve->vol_map, size);
    dev->vol_curve.count = curve->count;
    return CODEC_DEV_OK;
}

int esp_codec_dev_set_out_vol(esp_codec_dev_handle_t handle, int volume)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = _verify_codec_setting(dev, true);
    if (ret != CODEC_DEV_OK) {
        return ret;
    }
    const audio_codec_if_t *codec = dev->codec_if;
    float db_value = _get_vol_db(&dev->vol_curve, volume);
    db_value -= dev->hw_gain_db;
    if (codec && codec->set_vol) {
        codec->set_vol(codec, db_value);
        dev->volume = volume;
        return CODEC_DEV_OK;
    } else if (dev->sw_vol) {
        audio_codec_sw_vol_set(dev->sw_vol, db_value);
        return CODEC_DEV_OK;
    }
    return CODEC_DEV_NOT_SUPPORT;
}

int esp_codec_dev_get_out_vol(esp_codec_dev_handle_t handle, int *volume)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = _verify_codec_setting(dev, true);
    if (ret != CODEC_DEV_OK) {
        return ret;
    }
    *volume = dev->volume;
    return CODEC_DEV_OK;
}

int esp_codec_dev_set_out_mute(esp_codec_dev_handle_t handle, bool mute)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = _verify_codec_setting(dev, true);
    if (ret != CODEC_DEV_OK) {
        return ret;
    }
    const audio_codec_if_t *codec = dev->codec_if;
    if (codec && codec->mute) {
        codec->mute(codec, mute);
        dev->muted = mute;
        return CODEC_DEV_OK;
    }
    return CODEC_DEV_NOT_SUPPORT;
}

int esp_codec_dev_get_out_mute(esp_codec_dev_handle_t handle, bool *muted)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = _verify_codec_setting(dev, true);
    if (ret != CODEC_DEV_OK) {
        return ret;
    }
    *muted = dev->muted;
    return CODEC_DEV_OK;
}

int esp_codec_dev_set_in_gain(esp_codec_dev_handle_t handle, float db)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = _verify_codec_setting(dev, false);
    if (ret != CODEC_DEV_OK) {
        return ret;
    }
    const audio_codec_if_t *codec = dev->codec_if;
    if (codec && codec->set_mic_gain) {
        codec->set_mic_gain(codec, (int) db);
        dev->mic_gain = db;
        return CODEC_DEV_OK;
    }
    return CODEC_DEV_NOT_SUPPORT;
}

int esp_codec_dev_get_in_gain(esp_codec_dev_handle_t handle, float *db_value)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = _verify_codec_setting(dev, false);
    if (ret != CODEC_DEV_OK) {
        return ret;
    }
    *db_value = dev->mic_gain;
    return CODEC_DEV_OK;
}

int esp_codec_dev_set_in_mute(esp_codec_dev_handle_t handle, bool mute)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = _verify_codec_setting(dev, false);
    if (ret != CODEC_DEV_OK) {
        return ret;
    }
    const audio_codec_if_t *codec = dev->codec_if;
    if (codec && codec->mute_mic) {
        codec->mute_mic(codec, mute);
        dev->mic_muted = mute;
        return CODEC_DEV_OK;
    }
    return CODEC_DEV_NOT_SUPPORT;
}

int esp_codec_dev_get_in_mute(esp_codec_dev_handle_t handle, bool *muted)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = _verify_codec_setting(dev, false);
    if (ret != CODEC_DEV_OK) {
        return ret;
    }
    *muted = dev->mic_muted;
    return CODEC_DEV_OK;
}

int esp_codec_dev_close(esp_codec_dev_handle_t handle)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (dev->output_opened == false && dev->input_opened == false) {
        return CODEC_DEV_OK;
    }
    const audio_codec_if_t *codec = dev->codec_if;
    if (codec) {
        if (codec->enable) {
            codec->enable(codec, false);
        }
    }
    if (dev->sw_vol) {
        audio_codec_sw_vol_close(dev->sw_vol);
        dev->sw_vol = NULL;
    }
    dev->output_opened = dev->input_opened = false;
    return CODEC_DEV_OK;
}

void esp_codec_dev_delete(esp_codec_dev_handle_t handle)
{
    codec_dev_t *dev = (codec_dev_t *) handle;
    if (dev) {
        esp_codec_dev_close(handle);
        if (dev->vol_curve.vol_map) {
            free(dev->vol_curve.vol_map);
        }
        free(dev);
    }
}
