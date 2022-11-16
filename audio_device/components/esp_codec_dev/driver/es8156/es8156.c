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

#include <string.h>
#include "audio_codec_ctrl_if.h"
#include "es8156.h"
#include "es8156_reg.h"
#include "esp_log.h"
#include "codec_dev_gpio.h"
#include "codec_dev_os.h"
#include "codec_dev_defaults.h"
#include "codec_dev_err.h"
#include "codec_dev_utils.h"

#define BIT(b) (1 << b)

#define TAG    "ES8156"

typedef struct {
    audio_codec_if_t             base;
    const audio_codec_ctrl_if_t *ctrl_if;
    int16_t                      pa_pin;
    bool                         is_open;
} audio_codec_es8156_t;

const codec_dev_vol_range_t vol_range = {
    .min_vol ={
        .vol = 0x0,
        .db_value = -95.5,
    },
    .max_vol = {
        .vol = 0xFF,
        .db_value = 32.0,
    },
};

static int es8156_write_reg(audio_codec_es8156_t *codec, int reg, int value)
{
    return codec->ctrl_if->write_addr(codec->ctrl_if, reg, 1, &value, 1);
}

static int es8156_read_reg(audio_codec_es8156_t *codec, int reg, int *value)
{
    *value = 0;
    return codec->ctrl_if->read_addr(codec->ctrl_if, reg, 1, value, 1);
}

static int es8156_stop(audio_codec_es8156_t *codec)
{
    int ret = 0;
    ret = es8156_write_reg(codec, 0x14, 0x00);
    ret |= es8156_write_reg(codec, 0x19, 0x02);
    ret |= es8156_write_reg(codec, 0x21, 0x1F);
    ret |= es8156_write_reg(codec, 0x22, 0x02);
    ret |= es8156_write_reg(codec, 0x25, 0x21);
    ret |= es8156_write_reg(codec, 0x25, 0xA1);
    ret |= es8156_write_reg(codec, 0x18, 0x01);
    ret |= es8156_write_reg(codec, 0x09, 0x02);
    ret |= es8156_write_reg(codec, 0x09, 0x01);
    ret |= es8156_write_reg(codec, 0x08, 0x00);
    return ret;
}

static int es8156_start(audio_codec_es8156_t *codec)
{
    int ret = 0;
    ret |= es8156_write_reg(codec, 0x08, 0x3F);
    ret |= es8156_write_reg(codec, 0x09, 0x00);
    ret |= es8156_write_reg(codec, 0x18, 0x00);

    ret |= es8156_write_reg(codec, 0x25, 0x20);
    ret |= es8156_write_reg(codec, 0x22, 0x00);
    ret |= es8156_write_reg(codec, 0x21, 0x3C);
    ret |= es8156_write_reg(codec, 0x19, 0x20);
    ret |= es8156_write_reg(codec, 0x14, 179);
    return ret;
}

int es8156_set_mute(const audio_codec_if_t *h, bool mute)
{
    audio_codec_es8156_t *codec = (audio_codec_es8156_t *) h;
    if (codec == NULL || codec->is_open == false) {
        return CODEC_DEV_INVALID_ARG;
    }
    ESP_LOGI(TAG, "Enter into es8156_mute(), mute = %d\n", mute);
    int regv;
    int ret = es8156_read_reg(codec, ES8156_DAC_MUTE_REG13, &regv);
    if (ret < 0) {
        return CODEC_DEV_READ_FAIL;
    }
    if (mute) {
        regv = regv | BIT(1) | BIT(2);
    } else {
        regv = regv & (~(BIT(1) | BIT(2)));
    }
    return es8156_write_reg(codec, ES8156_DAC_MUTE_REG13, (uint8_t) regv);
}

int es8156_set_vol(const audio_codec_if_t *h, float volume)
{
    audio_codec_es8156_t *codec = (audio_codec_es8156_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    int reg = audio_codec_calc_vol_reg(&vol_range, volume);
    ESP_LOGI(TAG, "SET: volume:%f, reg:%d", volume, reg);
    int ret = es8156_write_reg(codec, ES8156_VOLUME_CONTROL_REG14, reg);
    return (ret == 0) ? CODEC_DEV_OK : CODEC_DEV_WRITE_FAIL;
}

int es8156_codec_get_voice_volume(audio_codec_es8156_t *codec, float *volume)
{
    *volume = 0;
    int reg = 0;
    int ret = es8156_read_reg(codec, ES8156_VOLUME_CONTROL_REG14, &reg);
    if (ret != CODEC_DEV_OK) {
        return CODEC_DEV_READ_FAIL;
    }
    *volume = audio_codec_calc_vol_db(&vol_range, reg);
    ESP_LOGD(TAG, "GET: reg:%d, volume:%fdb", reg, *volume);
    return CODEC_DEV_OK;
}

/*
 * enable pa power
 */
void es8156_pa_power(audio_codec_es8156_t *codec, bool enable)
{
    int16_t pa_pin = codec->pa_pin;
    const audio_codec_gpio_if_t *gpio_if = audio_codec_get_gpio_if();
    if (pa_pin == -1) {
        return;
    }
    gpio_if->setup(pa_pin, AUDIO_GPIO_DIR_OUT, AUDIO_GPIO_MODE_FLOAT);
    gpio_if->set(pa_pin, enable);
    ESP_LOGI(TAG, "PA gpio %d enable %d", pa_pin, enable);
}

int es8156_open(const audio_codec_if_t *h, void *cfg, int cfg_size)
{
    audio_codec_es8156_t *codec = (audio_codec_es8156_t *) h;
    es8156_codec_cfg_t *codec_cfg = (es8156_codec_cfg_t *) cfg;
    if (codec == NULL || codec_cfg == NULL || cfg_size != sizeof(es8156_codec_cfg_t) || codec_cfg->ctrl_if == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = CODEC_DEV_OK;
    codec->ctrl_if = codec_cfg->ctrl_if;
    codec->pa_pin = codec_cfg->pa_pin;

    ret |= es8156_write_reg(codec, 0x02, 0x04);
    ret |= es8156_write_reg(codec, 0x20, 0x2A);
    ret |= es8156_write_reg(codec, 0x21, 0x3C);
    ret |= es8156_write_reg(codec, 0x22, 0x00);
    ret |= es8156_write_reg(codec, 0x24, 0x07);
    ret |= es8156_write_reg(codec, 0x23, 0x00);

    ret |= es8156_write_reg(codec, 0x0A, 0x01);
    ret |= es8156_write_reg(codec, 0x0B, 0x01);
    ret |= es8156_write_reg(codec, 0x11, 0x00);
    ret |= es8156_write_reg(codec, 0x14, 179); // volume 70%

    ret |= es8156_write_reg(codec, 0x0D, 0x14);
    ret |= es8156_write_reg(codec, 0x18, 0x00);
    ret |= es8156_write_reg(codec, 0x08, 0x3F);
    ret |= es8156_write_reg(codec, 0x00, 0x02);
    ret |= es8156_write_reg(codec, 0x00, 0x03);
    ret |= es8156_write_reg(codec, 0x25, 0x20);
    if (ret != 0) {
        return CODEC_DEV_WRITE_FAIL;
    }
    es8156_pa_power(codec, true);
    codec->is_open = true;
    return CODEC_DEV_OK;
}

int es8156_close(const audio_codec_if_t *h)
{
    audio_codec_es8156_t *codec = (audio_codec_es8156_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open) {
        es8156_stop(codec);
        es8156_pa_power(codec, false);
        codec->is_open = false;
    }
    return CODEC_DEV_OK;
}

int es8156_enable(const audio_codec_if_t *h, bool enable)
{
    int ret = CODEC_DEV_OK;
    audio_codec_es8156_t *codec = (audio_codec_es8156_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    if (enable) {
        ret = es8156_start(codec);
        ESP_LOGI(TAG, "Start ret %d", ret);
    } else {
        ESP_LOGW(TAG, "The codec is about to stop");
        ret = es8156_stop(codec);
    }
    return ret;
}

static int es8156_set_reg(const audio_codec_if_t *h, int reg, int value)
{
    audio_codec_es8156_t *codec = (audio_codec_es8156_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return es8156_write_reg(codec, reg, value);
}

static int es8156_get_reg(const audio_codec_if_t *h, int reg, int *value)
{
    audio_codec_es8156_t *codec = (audio_codec_es8156_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return es8156_read_reg(codec, reg, value);
}

const audio_codec_if_t *es8156_codec_new(es8156_codec_cfg_t *codec_cfg)
{
    // verify param
    if (codec_cfg == NULL || codec_cfg->ctrl_if == NULL) {
        ESP_LOGE(TAG, "wrong codec config");
        return NULL;
    }
    if (codec_cfg->ctrl_if->is_open(codec_cfg->ctrl_if) == false) {
        ESP_LOGE(TAG, "ctrl interface not open yet");
        return NULL;
    }
    audio_codec_es8156_t *codec = (audio_codec_es8156_t *) calloc(1, sizeof(audio_codec_es8156_t));
    if (codec == NULL) {
        return NULL;
    }
    // overwrite functions
    codec->ctrl_if = codec_cfg->ctrl_if;
    codec->base.open = es8156_open;
    codec->base.enable = es8156_enable;
    codec->base.set_vol = es8156_set_vol;
    codec->base.mute = es8156_set_mute;
    codec->base.set_reg = es8156_set_reg;
    codec->base.get_reg = es8156_get_reg;
    codec->base.close = es8156_close;
    do {
        int ret = codec->base.open(&codec->base, codec_cfg, sizeof(es8156_codec_cfg_t));
        if (ret != 0) {
            ESP_LOGE(TAG, "Fail to open");
            break;
        }
        return &codec->base;
    } while (0);
    if (codec) {
        free(codec);
    }
    return NULL;
}
