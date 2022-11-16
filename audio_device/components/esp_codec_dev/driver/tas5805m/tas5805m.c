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
#include "esp_log.h"
#include "tas5805m.h"
#include "tas5805m_reg.h"
#include "tas5805m_reg_cfg.h"
#include "codec_dev_defaults.h"
#include "codec_dev_err.h"
#include "codec_dev_gpio.h"
#include "codec_dev_os.h"
#include "codec_dev_utils.h"

#define TAG "TAS5805M"

typedef struct {
    audio_codec_if_t     base;
    tas5805m_codec_cfg_t cfg;
    bool                 is_open;
} audio_codec_tas5805m_t;

static const codec_dev_vol_range_t vol_range = {
    .min_vol = {
        .vol = 0xFE,
        .db_value = -103.0,
    },
    .max_vol = {
        .vol = 0,
        .db_value = 24.0,
    },
};

static int tas5805m_write_reg(audio_codec_tas5805m_t *codec, int reg, int value)
{
    return codec->cfg.ctrl_if->write_addr(codec->cfg.ctrl_if, reg, 1, &value, 1);
}

static int tas5805m_read_reg(audio_codec_tas5805m_t *codec, int reg, int *value)
{
    *value = 0;
    return codec->cfg.ctrl_if->read_addr(codec->cfg.ctrl_if, reg, 1, value, 1);
}

static int tas5805m_write_data(audio_codec_tas5805m_t *codec, uint8_t reg_addr, uint8_t *data, int size)
{
    return codec->cfg.ctrl_if->write_addr(codec->cfg.ctrl_if, reg_addr, 1, data, size);
}

static int tas5805m_transmit_registers(audio_codec_tas5805m_t *codec, const tas5805m_cfg_reg_t *conf_buf, int size)
{
    int i = 0;
    int ret = 0;
    while (i < size) {
        switch (conf_buf[i].offset) {
            case CFG_META_SWITCH:
                // Used in legacy applications.  Ignored here.
                break;
            case CFG_META_DELAY:
                codec_dev_sleep(conf_buf[i].value);
                break;
            case CFG_META_BURST:
                ret = tas5805m_write_data(codec, conf_buf[i + 1].offset, (uint8_t *) (&conf_buf[i + 1].value),
                                          conf_buf[i].value);
                i += (conf_buf[i].value / 2) + 1;
                break;
            case CFG_END_1:
                if (CFG_END_2 == conf_buf[i + 1].offset && CFG_END_3 == conf_buf[i + 2].offset) {
                    ESP_LOGI(TAG, "End of tms5805m reg: %d\n", i);
                }
                break;
            default:
                ret = tas5805m_write_reg(codec, conf_buf[i].offset, conf_buf[i].value);
                break;
        }
        i++;
    }
    if (ret != CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Fail to load configuration to tas5805m");
        return ret;
    }
    return ret;
}

static int tas5805m_set_mute_fade(audio_codec_tas5805m_t *codec, int value)
{
    int ret = 0;
    uint8_t fade_reg = 0;
    /* Time for register value
     *   000: 11.5 ms
     *   001: 53 ms
     *   010: 106.5 ms
     *   011: 266.5 ms
     *   100: 0.535 sec
     *   101: 1.065 sec
     *   110: 2.665 sec
     *   111: 5.33 sec
     */
    if (value <= 12) {
        fade_reg = 0;
    } else if (value <= 53) {
        fade_reg = 1;
    } else if (value <= 107) {
        fade_reg = 2;
    } else if (value <= 267) {
        fade_reg = 3;
    } else if (value <= 535) {
        fade_reg = 4;
    } else if (value <= 1065) {
        fade_reg = 5;
    } else if (value <= 2665) {
        fade_reg = 6;
    } else {
        fade_reg = 7;
    }
    fade_reg |= (fade_reg << 4);
    ret |= tas5805m_write_reg(codec, MUTE_TIME_REG_ADDR, fade_reg);
    ESP_LOGI(TAG, "Set mute fade, value:%d, 0x%x", value, fade_reg);
    return ret == 0 ? CODEC_DEV_OK : CODEC_DEV_WRITE_FAIL;
}

static void tas5805m_reset(audio_codec_tas5805m_t *codec, int16_t reset_pin)
{
    const audio_codec_gpio_if_t *gpio_if = audio_codec_get_gpio_if();
    if (reset_pin <= 0 || gpio_if == NULL) {
        return;
    }
    gpio_if->setup(reset_pin, AUDIO_GPIO_DIR_OUT, AUDIO_GPIO_MODE_FLOAT);
    gpio_if->set(reset_pin, 0);
    codec_dev_sleep(20);
    gpio_if->set(reset_pin, 1);
    codec_dev_sleep(200);
}

static int tas5805m_open(const audio_codec_if_t *h, void *cfg, int cfg_size)
{
    audio_codec_tas5805m_t *codec = (audio_codec_tas5805m_t *) h;
    tas5805m_codec_cfg_t *codec_cfg = (tas5805m_codec_cfg_t *) cfg;
    if (codec == NULL || codec_cfg == NULL || codec_cfg->ctrl_if == NULL || cfg_size != sizeof(tas5805m_codec_cfg_t)) {
        return CODEC_DEV_INVALID_ARG;
    }
    memcpy(&codec->cfg, codec_cfg, sizeof(tas5805m_codec_cfg_t));
    tas5805m_reset(codec, codec_cfg->reset_pin);
    int ret = tas5805m_transmit_registers(codec, tas5805m_registers,
                                          sizeof(tas5805m_registers) / sizeof(tas5805m_registers[0]));
    if (ret != CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Fail write register group");
    } else {
        codec->is_open = true;
        tas5805m_set_mute_fade(codec, 50);
    }
    return ret;
}

static int tas5805m_set_volume(const audio_codec_if_t *h, float db_value)
{
    audio_codec_tas5805m_t *codec = (audio_codec_tas5805m_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    int volume = audio_codec_calc_vol_reg(&vol_range, db_value);
    ESP_LOGI(TAG, "SET: volume:%x db:%f", volume, db_value);
    return tas5805m_write_reg(codec, MASTER_VOL_REG_ADDR, volume);
}

int tas5805m_get_volume(audio_codec_tas5805m_t *codec, float *value)
{
    /// FIXME: Got the digit volume is not right.
    int vol_idx = 0;
    int ret = tas5805m_read_reg(codec, MASTER_VOL_REG_ADDR, &vol_idx);
    if (ret == CODEC_DEV_OK) {
        *value = audio_codec_calc_vol_db(&vol_range, vol_idx);
        ESP_LOGI(TAG, "Volume is %fdb", *value);
        return 0;
    }
    return CODEC_DEV_READ_FAIL;
}

static int tas5805m_set_mute(const audio_codec_if_t *h, bool enable)
{
    audio_codec_tas5805m_t *codec = (audio_codec_tas5805m_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    int mute_reg = 0;
    tas5805m_read_reg(codec, TAS5805M_REG_03, &mute_reg);
    if (enable) {
        mute_reg |= 0x8;
    } else {
        mute_reg &= (~0x08);
    }
    int ret = tas5805m_write_reg(codec, TAS5805M_REG_03, mute_reg);
    return ret == 0 ? CODEC_DEV_OK : CODEC_DEV_WRITE_FAIL;
}

static int tas5805m_set_reg(const audio_codec_if_t *h, int reg, int value)
{
    audio_codec_tas5805m_t *codec = (audio_codec_tas5805m_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return tas5805m_write_reg(codec, reg, value);
}

static int tas5805m_get_reg(const audio_codec_if_t *h, int reg, int *value)
{
    audio_codec_tas5805m_t *codec = (audio_codec_tas5805m_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return tas5805m_read_reg(codec, reg, value);
}

static int tas5805m_close(const audio_codec_if_t *h)
{
    audio_codec_tas5805m_t *codec = (audio_codec_tas5805m_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    codec->is_open = false;
    return 0;
}

const audio_codec_if_t *tas5805m_codec_new(tas5805m_codec_cfg_t *codec_cfg)
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
    audio_codec_tas5805m_t *codec = (audio_codec_tas5805m_t *) calloc(1, sizeof(audio_codec_tas5805m_t));
    if (codec == NULL) {
        return NULL;
    }
    codec->base.open = tas5805m_open;
    codec->base.set_vol = tas5805m_set_volume;
    codec->base.mute = tas5805m_set_mute;
    codec->base.set_reg = tas5805m_set_reg;
    codec->base.get_reg = tas5805m_get_reg;
    codec->base.close = tas5805m_close;
    do {
        int ret = codec->base.open(&codec->base, codec_cfg, sizeof(tas5805m_codec_cfg_t));
        if (ret != 0) {
            ESP_LOGE(TAG, "Fail to open");
            break;
        }
        ESP_LOGI(TAG, "open ok");
        return &codec->base;
    } while (0);
    if (codec) {
        free(codec);
    }
    return NULL;
}
