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
#include "es8311.h"
#include "es8311_reg.h"
#include "esp_log.h"
#include "esxxx_common.h"
#include "codec_dev_defaults.h"
#include "codec_dev_err.h"
#include "codec_dev_gpio.h"
#include "codec_dev_utils.h"

/* ES8311 address
 * 0x32:CE=1;0x30:CE=0
 */
#define BIT(n)       ((uint64_t) 1 << n)
#define MCLK_DIV_FRE 256
#define TAG          "ES8311"

typedef struct {
    audio_codec_if_t   base;
    es8311_codec_cfg_t cfg;
    bool               is_open;
    bool               enabled;
} audio_codec_es8311_t;

/*
 * Clock coefficient structer
 */
struct _coeff_div {
    uint32_t mclk;      /* mclk frequency */
    uint32_t rate;      /* sample rate */
    uint8_t  pre_div;   /* the pre divider with range from 1 to 8 */
    uint8_t  pre_multi; /* the pre multiplier with x1, x2, x4 and x8 selection */
    uint8_t  adc_div;   /* adcclk divider */
    uint8_t  dac_div;   /* dacclk divider */
    uint8_t  fs_mode;   /* double speed or single speed, =0, ss, =1, ds */
    uint8_t  lrck_h;    /* adclrck divider and daclrck divider */
    uint8_t  lrck_l;
    uint8_t  bclk_div; /* sclk divider */
    uint8_t  adc_osr;  /* adc osr */
    uint8_t  dac_osr;  /* dac osr */
};

/* codec hifi mclk clock divider coefficients */
static const struct _coeff_div coeff_div[] = {
  //  mclk     rate   pre_div  mult  adc_div dac_div fs_mode lrch  lrcl  bckdiv osr
  /* 8k */
    {12288000, 8000,  0x06, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {18432000, 8000,  0x03, 0x02, 0x03, 0x03, 0x00, 0x05, 0xff, 0x18, 0x10, 0x20},
    {16384000, 8000,  0x08, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {8192000,  8000,  0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {6144000,  8000,  0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {4096000,  8000,  0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {3072000,  8000,  0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {2048000,  8000,  0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {1536000,  8000,  0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {1024000,  8000,  0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},

 /* 11.025k */
    {11289600, 11025, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {5644800,  11025, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {2822400,  11025, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {1411200,  11025, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},

 /* 12k */
    {12288000, 12000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {6144000,  12000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {3072000,  12000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {1536000,  12000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},

 /* 16k */
    {12288000, 16000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {18432000, 16000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x20},
    {16384000, 16000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {8192000,  16000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {6144000,  16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {4096000,  16000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {3072000,  16000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {2048000,  16000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {1536000,  16000, 0x03, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
    {1024000,  16000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},

 /* 22.05k */
    {11289600, 22050, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800,  22050, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400,  22050, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200,  22050, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

 /* 24k */
    {12288000, 24000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 24000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000,  24000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000,  24000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000,  24000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

 /* 32k */
    {12288000, 32000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 32000, 0x03, 0x04, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
    {16384000, 32000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000,  32000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000,  32000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000,  32000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000,  32000, 0x03, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000,  32000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000,  32000, 0x03, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
    {1024000,  32000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

 /* 44.1k */
    {11289600, 44100, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800,  44100, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400,  44100, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200,  44100, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

 /* 48k */
    {12288000, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 48000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000,  48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000,  48000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000,  48000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

 /* 64k */
    {12288000, 64000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 64000, 0x03, 0x04, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {16384000, 64000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000,  64000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000,  64000, 0x01, 0x04, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {4096000,  64000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000,  64000, 0x01, 0x08, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {2048000,  64000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000,  64000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0xbf, 0x03, 0x18, 0x18},
    {1024000,  64000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

 /* 88.2k */
    {11289600, 88200, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800,  88200, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400,  88200, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200,  88200, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

 /* 96k */
    {12288000, 96000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 96000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000,  96000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000,  96000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000,  96000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
};

static const codec_dev_vol_range_t vol_range = {
    .min_vol = {
        .vol = 0x0,
        .db_value = -95.5,
    },
    .max_vol = {
        .vol = 0xFF,
        .db_value = 32.0,
    },
};

static int es8311_write_reg(audio_codec_es8311_t *codec, int reg, int value)
{
    return codec->cfg.ctrl_if->write_addr(codec->cfg.ctrl_if, reg, 1, &value, 1);
}

static int es8311_read_reg(audio_codec_es8311_t *codec, int reg, int *value)
{
    *value = 0;
    return codec->cfg.ctrl_if->read_addr(codec->cfg.ctrl_if, reg, 1, value, 1);
}

int es8311_config_fmt(audio_codec_es8311_t *codec, es_i2s_fmt_t fmt)
{
    int ret = CODEC_DEV_OK;
    int adc_iface = 0, dac_iface = 0;
    ret = es8311_read_reg(codec, ES8311_SDPIN_REG09, &dac_iface);
    ret |= es8311_read_reg(codec, ES8311_SDPOUT_REG0A, &adc_iface);
    switch (fmt) {
        case ES_I2S_NORMAL:
            ESP_LOGD(TAG, "ES8311 in I2S Format");
            dac_iface &= 0xFC;
            adc_iface &= 0xFC;
            break;
        case ES_I2S_LEFT:
        case ES_I2S_RIGHT:
            ESP_LOGD(TAG, "ES8311 in LJ Format");
            adc_iface &= 0xFC;
            dac_iface &= 0xFC;
            adc_iface |= 0x01;
            dac_iface |= 0x01;
            break;
        case ES_I2S_DSP:
            ESP_LOGD(TAG, "ES8311 in DSP-A Format");
            adc_iface &= 0xDC;
            dac_iface &= 0xDC;
            adc_iface |= 0x03;
            dac_iface |= 0x03;
            break;
        default:
            dac_iface &= 0xFC;
            adc_iface &= 0xFC;
            break;
    }
    ret |= es8311_write_reg(codec, ES8311_SDPIN_REG09, dac_iface);
    ret |= es8311_write_reg(codec, ES8311_SDPOUT_REG0A, adc_iface);
    return ret;
}

int es8311_set_bits_per_sample(audio_codec_es8311_t *codec, int bits)
{
    int ret = CODEC_DEV_OK;
    int adc_iface = 0, dac_iface = 0;
    ret |= es8311_read_reg(codec, ES8311_SDPIN_REG09, &dac_iface);
    ret |= es8311_read_reg(codec, ES8311_SDPOUT_REG0A, &adc_iface);
    switch (bits) {
        case 16:
        default:
            dac_iface |= 0x0c;
            adc_iface |= 0x0c;
            break;
        case 24:
            break;
        case 32:
            dac_iface |= 0x10;
            adc_iface |= 0x10;
            break;
    }
    ret |= es8311_write_reg(codec, ES8311_SDPIN_REG09, dac_iface);
    ret |= es8311_write_reg(codec, ES8311_SDPOUT_REG0A, adc_iface);

    return ret;
}

/*
 * look for the coefficient in coeff_div[] table
 */
static int get_coeff(uint32_t mclk, uint32_t rate)
{
    for (int i = 0; i < (sizeof(coeff_div) / sizeof(coeff_div[0])); i++) {
        if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk)
            return i;
    }
    return CODEC_DEV_NOT_FOUND;
}

/*
 * set es8311 into suspend mode
 */
static int es8311_suspend(audio_codec_es8311_t *codec)
{
    ESP_LOGI(TAG, "Enter into es8311_suspend()");
    es8311_write_reg(codec, ES8311_DAC_REG32, 0x00);
    es8311_write_reg(codec, ES8311_ADC_REG17, 0x00);
    es8311_write_reg(codec, ES8311_SYSTEM_REG0E, 0xFF);
    es8311_write_reg(codec, ES8311_SYSTEM_REG12, 0x02);
    es8311_write_reg(codec, ES8311_SYSTEM_REG14, 0x00);
    es8311_write_reg(codec, ES8311_SYSTEM_REG0D, 0xFA);
    es8311_write_reg(codec, ES8311_ADC_REG15, 0x00);
    es8311_write_reg(codec, ES8311_DAC_REG37, 0x08);
    return es8311_write_reg(codec, ES8311_GP_REG45, 0x01);
}

static int es8311_start(audio_codec_es8311_t *codec)
{
    int ret = CODEC_DEV_OK;
    int adc_iface = 0, dac_iface = 0;

    ret = es8311_read_reg(codec, ES8311_SDPIN_REG09, &dac_iface);
    ret |= es8311_read_reg(codec, ES8311_SDPOUT_REG0A, &adc_iface);
    if (ret != CODEC_DEV_OK) {
        return ret;
    }
    dac_iface &= 0xBF;
    adc_iface &= 0xBF;
    adc_iface |= BIT(6);
    dac_iface |= BIT(6);
    int codec_mode = codec->cfg.codec_mode;
    if (codec_mode == CODEC_WORK_MODE_LINE) {
        ESP_LOGE(TAG, "The codec es8311 doesn't support ES_MODULE_LINE mode");
        return CODEC_DEV_NOT_SUPPORT;
    }
    if (codec_mode == CODEC_WORK_MODE_ADC || codec_mode == CODEC_WORK_MODE_BOTH) {
        adc_iface &= ~(BIT(6));
    }
    if (codec_mode == CODEC_WORK_MODE_DAC || codec_mode == CODEC_WORK_MODE_BOTH) {
        dac_iface &= ~(BIT(6));
    }

    ret |= es8311_write_reg(codec, ES8311_SDPIN_REG09, dac_iface);
    ret |= es8311_write_reg(codec, ES8311_SDPOUT_REG0A, adc_iface);

    ret |= es8311_write_reg(codec, ES8311_ADC_REG17, 0xBF);
    ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG0E, 0x02);
    ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG12, 0x00);
    ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG14, 0x1A);

    /*
     * pdm dmic enable or disable
     */
    int regv = 0;
    if (codec->cfg.digital_mic) {
        ret |= es8311_read_reg(codec, ES8311_SYSTEM_REG14, &regv);
        regv |= 0x40;
        ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG14, regv);
    } else {
        ret |= es8311_read_reg(codec, ES8311_SYSTEM_REG14, &regv);
        regv &= ~(0x40);
        ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG14, regv);
    }

    ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG0D, 0x01);
    ret |= es8311_write_reg(codec, ES8311_ADC_REG15, 0x40);
    ret |= es8311_write_reg(codec, ES8311_DAC_REG37, 0x48);
    ret |= es8311_write_reg(codec, ES8311_GP_REG45, 0x00);

    return ret;
}

int es8311_codec_get_voice_volume(audio_codec_es8311_t *codec, float *volume)
{
    int regv = 0;
    int ret = es8311_read_reg(codec, ES8311_DAC_REG32, &regv);
    *volume = audio_codec_calc_vol_db(&vol_range, regv);
    ESP_LOGD(TAG, "GET: res:%d, volume:%f", regv, *volume);
    return ret;
}

/*
 * set es8311 dac mute or not
 * if mute = 0, dac un-mute
 * if mute = 1, dac mute
 */
int es8311_set_mute(const audio_codec_if_t *h, bool mute)
{
    audio_codec_es8311_t *codec = (audio_codec_es8311_t *) h;
    if (codec == NULL || codec->is_open == false) {
        return CODEC_DEV_INVALID_ARG;
    }
    int regv;
    ESP_LOGI(TAG, "Enter into es8311_mute(), mute = %d\n", mute);
    int ret = es8311_read_reg(codec, ES8311_DAC_REG31, &regv);
    regv &= 0x9f;
    if (mute) {
        es8311_write_reg(codec, ES8311_SYSTEM_REG12, 0x02);
        es8311_write_reg(codec, ES8311_DAC_REG31, regv | 0x60);
        es8311_write_reg(codec, ES8311_DAC_REG32, 0x00);
        es8311_write_reg(codec, ES8311_DAC_REG37, 0x08);
    } else {
        es8311_write_reg(codec, ES8311_DAC_REG31, regv);
        es8311_write_reg(codec, ES8311_SYSTEM_REG12, 0x00);
    }
    return ret;
}

int es8311_get_voice_mute(audio_codec_es8311_t *codec, int *mute)
{
    int reg = 0;
    int ret = es8311_read_reg(codec, ES8311_DAC_REG31, &reg);
    if (ret >= 0) {
        reg = (reg & 0x20) >> 5;
    }
    *mute = reg;
    return ret;
}

int es8311_set_vol(const audio_codec_if_t *h, float db_value)
{
    audio_codec_es8311_t *codec = (audio_codec_es8311_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    int reg = audio_codec_calc_vol_reg(&vol_range, db_value);
    ESP_LOGI(TAG, "SET: volume:%x db:%d", reg, (int) db_value);
    return es8311_write_reg(codec, ES8311_DAC_REG32, (uint8_t) reg);
}

int es8311_set_mic_gain(const audio_codec_if_t *h, float db)
{
    audio_codec_es8311_t *codec = (audio_codec_es8311_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    es8311_mic_gain_t gain_db = ES8311_MIC_GAIN_0DB;
    if (db < 6) {
    } else if (db < 12) {
        gain_db = ES8311_MIC_GAIN_6DB;
    } else if (db < 18) {
        gain_db = ES8311_MIC_GAIN_12DB;
    } else if (db < 24) {
        gain_db = ES8311_MIC_GAIN_18DB;
    } else if (db < 30) {
        gain_db = ES8311_MIC_GAIN_24DB;
    } else if (db < 36) {
        gain_db = ES8311_MIC_GAIN_30DB;
    } else if (db < 42) {
        gain_db = ES8311_MIC_GAIN_36DB;
    } else {
        gain_db = ES8311_MIC_GAIN_42DB;
    }
    int ret = es8311_write_reg(codec, ES8311_ADC_REG16, gain_db); // MIC gain scale
    return ret == 0 ? CODEC_DEV_OK : CODEC_DEV_WRITE_FAIL;
}

/*
 * enable pa power
 */
void es8311_pa_power(audio_codec_es8311_t *codec, bool enable)
{
    int16_t pa_pin = codec->cfg.pa_pin;
    const audio_codec_gpio_if_t *gpio_if = audio_codec_get_gpio_if();
    if (pa_pin == -1 || gpio_if == NULL) {
        return;
    }
    gpio_if->setup(pa_pin, AUDIO_GPIO_DIR_OUT, AUDIO_GPIO_MODE_FLOAT);
    gpio_if->set(pa_pin, enable);
    ESP_LOGI(TAG, "PA gpio %d enable %d", pa_pin, enable);
}

int es8311_config_sample(audio_codec_es8311_t *codec, int sample_rate)
{
    int datmp, regv;
    int mclk_fre = sample_rate * MCLK_DIV_FRE;
    int coeff = get_coeff(mclk_fre, sample_rate);
    if (coeff < 0) {
        ESP_LOGE(TAG, "Unable to configure sample rate %dHz with %dHz MCLK", sample_rate, mclk_fre);
        return CODEC_DEV_NOT_SUPPORT;
    }
    int ret = 0;
    /*
     * Set clock parammeters
     */
    ret = es8311_read_reg(codec, ES8311_CLK_MANAGER_REG02, &regv);
    regv &= 0x7;
    regv |= (coeff_div[coeff].pre_div - 1) << 5;
    datmp = 0;
    switch (coeff_div[coeff].pre_multi) {
        case 1:
            datmp = 0;
            break;
        case 2:
            datmp = 1;
            break;
        case 4:
            datmp = 2;
            break;
        case 8:
            datmp = 3;
            break;
        default:
            break;
    }

    if (codec->cfg.master_mode == false) {
        datmp = 3; /* DIG_MCLK = LRCK * 256 = BCLK * 8 */
    }
    regv |= (datmp) << 3;
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG02, regv);
    // TODO check this logic
    ret |= es8311_read_reg(codec, ES8311_CLK_MANAGER_REG05, &regv);
    regv &= 0x00;
    regv |= (coeff_div[coeff].adc_div - 1) << 4;
    regv |= (coeff_div[coeff].dac_div - 1) << 0;
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG05, regv);

    ret |= es8311_read_reg(codec, ES8311_CLK_MANAGER_REG03, &regv);
    regv &= 0x80;
    regv |= coeff_div[coeff].fs_mode << 6;
    regv |= coeff_div[coeff].adc_osr << 0;
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG03, regv);

    ret |= es8311_read_reg(codec, ES8311_CLK_MANAGER_REG04, &regv);
    regv &= 0x80;
    regv |= coeff_div[coeff].dac_osr << 0;
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG04, regv);

    ret |= es8311_read_reg(codec, ES8311_CLK_MANAGER_REG07, &regv);
    regv &= 0xC0;
    regv |= coeff_div[coeff].lrck_h << 0;
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG07, regv);
    // TODO check this logic
    ret |= es8311_read_reg(codec, ES8311_CLK_MANAGER_REG08, &regv);
    regv &= 0x00;
    regv |= coeff_div[coeff].lrck_l << 0;
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG08, regv);

    ret = es8311_read_reg(codec, ES8311_CLK_MANAGER_REG06, &regv);
    regv &= 0xE0;
    if (coeff_div[coeff].bclk_div < 19) {
        regv |= (coeff_div[coeff].bclk_div - 1) << 0;
    } else {
        regv |= (coeff_div[coeff].bclk_div) << 0;
    }
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG06, regv);
    return ret == 0 ? CODEC_DEV_OK : CODEC_DEV_WRITE_FAIL;
}

int es8311_open(const audio_codec_if_t *h, void *cfg, int cfg_size)
{
    audio_codec_es8311_t *codec = (audio_codec_es8311_t *) h;
    es8311_codec_cfg_t *codec_cfg = (es8311_codec_cfg_t *) cfg;
    if (codec == NULL || codec_cfg == NULL || codec_cfg->ctrl_if == NULL || cfg_size != sizeof(es8311_codec_cfg_t)) {
        return CODEC_DEV_INVALID_ARG;
    }
    memcpy(&codec->cfg, cfg, sizeof(es8311_codec_cfg_t));
    int regv;
    int ret = CODEC_DEV_OK;
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG01, 0x30);
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG02, 0x00);
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG03, 0x10);
    ret |= es8311_write_reg(codec, ES8311_ADC_REG16, 0x24);
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG04, 0x10);
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG05, 0x00);
    ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG0B, 0x00);
    ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG0C, 0x00);
    ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG10, 0x1F);
    ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG11, 0x7F);
    ret |= es8311_write_reg(codec, ES8311_RESET_REG00, 0x80);
    /*
     * Set Codec into Master or Slave mode
     */
    ret = es8311_read_reg(codec, ES8311_RESET_REG00, &regv);
    /*
     * Set master/slave audio interface
     */
    if (codec_cfg->master_mode) {
        ESP_LOGI(TAG, "ES8311 in Master mode");
        regv |= 0x40;
    } else {
        ESP_LOGI(TAG, "ES8311 in Slave mode");
        regv &= 0xBF;
    }
    ret |= es8311_write_reg(codec, ES8311_RESET_REG00, regv);
    ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG01, 0x3F);
    /*
     * Select clock source for internal mclk
     */
    ret |= es8311_read_reg(codec, ES8311_CLK_MANAGER_REG01, &regv);
    if (codec_cfg->use_mclk) {
        regv &= 0x7F;
    } else {
        regv |= 0x80;
    }
    /*
     * mclk inverted or not
     */
    if (codec_cfg->invert_mclk) {
        ret |= es8311_read_reg(codec, ES8311_CLK_MANAGER_REG01, &regv);
        regv |= 0x40;
        ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG01, regv);
    } else {
        ret |= es8311_read_reg(codec, ES8311_CLK_MANAGER_REG01, &regv);
        regv &= ~(0x40);
        ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG01, regv);
    }
    /*
     * sclk inverted or not
     */
    if (codec_cfg->invert_sclk) {
        ret |= es8311_read_reg(codec, ES8311_CLK_MANAGER_REG06, &regv);
        regv |= 0x20;
        ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG06, regv);
    } else {
        ret |= es8311_read_reg(codec, ES8311_CLK_MANAGER_REG06, &regv);
        regv &= ~(0x20);
        ret |= es8311_write_reg(codec, ES8311_CLK_MANAGER_REG06, regv);
    }

    ret |= es8311_write_reg(codec, ES8311_SYSTEM_REG13, 0x10);
    ret |= es8311_write_reg(codec, ES8311_ADC_REG1B, 0x0A);
    ret |= es8311_write_reg(codec, ES8311_ADC_REG1C, 0x6A);
    if (ret != 0) {
        return CODEC_DEV_WRITE_FAIL;
    }
    es8311_pa_power(codec, true);
    codec->is_open = true;
    return CODEC_DEV_OK;
}

int es8311_close(const audio_codec_if_t *h)
{
    audio_codec_es8311_t *codec = (audio_codec_es8311_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open) {
        es8311_suspend(codec);
        es8311_pa_power(codec, false);
        codec->is_open = false;
    }
    return CODEC_DEV_OK;
}

int es8311_set_fs(const audio_codec_if_t *h, codec_sample_info_t *fs)
{
    audio_codec_es8311_t *codec = (audio_codec_es8311_t *) h;
    if (codec == NULL || codec->is_open == false) {
        return CODEC_DEV_INVALID_ARG;
    }
    es8311_set_bits_per_sample(codec, fs->bits_per_sample);
    if (fs->channel == 1) {
        es8311_config_fmt(codec, ES_I2S_LEFT);
    } else {
        es8311_config_fmt(codec, ES_I2S_NORMAL);
    }
    es8311_config_sample(codec, fs->sample_rate);
    return CODEC_DEV_OK;
}

int es8311_enable(const audio_codec_if_t *h, bool enable)
{
    int ret = CODEC_DEV_OK;
    audio_codec_es8311_t *codec = (audio_codec_es8311_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    if (enable == codec->enabled) {
        return CODEC_DEV_OK;
    }
    if (enable) {
        ret = es8311_start(codec);
        ESP_LOGI(TAG, "Start ret %d", ret);
    } else {
        ESP_LOGW(TAG, "The codec is about to stop");
        ret = es8311_suspend(codec);
    }
    if (ret == CODEC_DEV_OK) {
        codec->enabled = enable;
    }
    return ret;
}

static int es8311_set_reg(const audio_codec_if_t *h, int reg, int value)
{
    audio_codec_es8311_t *codec = (audio_codec_es8311_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return es8311_write_reg(codec, reg, value);
}

static int es8311_get_reg(const audio_codec_if_t *h, int reg, int *value)
{
    audio_codec_es8311_t *codec = (audio_codec_es8311_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return es8311_read_reg(codec, reg, value);
}

const audio_codec_if_t *es8311_codec_new(es8311_codec_cfg_t *codec_cfg)
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
    audio_codec_es8311_t *codec = (audio_codec_es8311_t *) calloc(1, sizeof(audio_codec_es8311_t));
    if (codec == NULL) {
        return NULL;
    }
    codec->base.open = es8311_open;
    codec->base.enable = es8311_enable;
    codec->base.set_fs = es8311_set_fs;
    codec->base.set_vol = es8311_set_vol;
    codec->base.set_mic_gain = es8311_set_mic_gain;
    codec->base.mute = es8311_set_mute;
    codec->base.set_reg = es8311_set_reg;
    codec->base.get_reg = es8311_get_reg;
    codec->base.close = es8311_close;
    do {
        int ret = codec->base.open(&codec->base, codec_cfg, sizeof(es8311_codec_cfg_t));
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
