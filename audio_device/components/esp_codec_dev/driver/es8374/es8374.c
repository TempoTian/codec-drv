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
#include "es8374.h"
#include "esp_log.h"
#include "codec_dev_defaults.h"
#include "codec_dev_err.h"
#include "codec_dev_gpio.h"
#include "codec_dev_utils.h"
#include "esxxx_common.h"

#define TAG "ES8374"

typedef struct {
    audio_codec_if_t   base;
    es8374_codec_cfg_t cfg;
    bool               is_open;
} audio_codec_es8374_t;

static const codec_dev_vol_range_t vol_range = {
    .min_vol = {
        .vol = 0xC0,
        .db_value = -96.0,
    },
    .max_vol = {
        .vol = 0x0,
        .db_value = 0.0,
    },
};

static int es8374_write_reg(audio_codec_es8374_t *codec, int reg, int value)
{
    return codec->cfg.ctrl_if->write_addr(codec->cfg.ctrl_if, reg, 1, &value, 1);
}

static int es8374_read_reg(audio_codec_es8374_t *codec, int reg, int *value)
{
    *value = 0;
    return codec->cfg.ctrl_if->read_addr(codec->cfg.ctrl_if, reg, 1, value, 1);
}

void es8374_read_all(audio_codec_es8374_t *codec)
{
    for (int i = 0; i < 50; i++) {
        int reg = 0;
        es8374_read_reg(codec, i, &reg);
        ESP_LOGI(TAG, "%x: %x", i, reg);
    }
}

static int es8374_set_voice_mute(audio_codec_es8374_t *codec, bool enable)
{
    int ret = 0;
    int reg = 0;
    ret |= es8374_read_reg(codec, 0x36, &reg);
    if (ret == 0) {
        reg = reg & 0xdf;
        ret |= es8374_write_reg(codec, 0x36, reg | (((int) enable) << 5));
    }
    return ret;
}

bool es8374_get_voice_mute(audio_codec_es8374_t *codec)
{
    int reg = 0;
    int ret = es8374_read_reg(codec, 0x36, &reg);
    if (ret == 0) {
        return (reg & 0x40) > 0;
    }
    return false;
}

static es_bits_length_t get_bits(uint8_t bits)
{
    switch (bits) {
        default:
        case 16:
            return BIT_LENGTH_16BITS;
        case 18:
            return BIT_LENGTH_18BITS;
        case 20:
            return BIT_LENGTH_20BITS;
        case 24:
            return BIT_LENGTH_24BITS;
        case 32:
            return BIT_LENGTH_32BITS;
    }
}

static int es8374_set_bits_per_sample(audio_codec_es8374_t *codec, uint8_t bits)
{
    int ret = CODEC_DEV_OK;
    int reg = 0;
    es_bits_length_t bit_per_sample = get_bits(bits);
    bits = (int) bit_per_sample & 0x0f;

    if (codec->cfg.codec_mode & CODEC_WORK_MODE_ADC) {
        ret |= es8374_read_reg(codec, 0x10, &reg);
        if (ret == 0) {
            reg = reg & 0xe3;
            ret |= es8374_write_reg(codec, 0x10, reg | (bits << 2));
        }
    }
    if (codec->cfg.codec_mode & CODEC_WORK_MODE_DAC) {
        ret |= es8374_read_reg(codec, 0x11, &reg);
        if (ret == 0) {
            reg = reg & 0xe3;
            ret |= es8374_write_reg(codec, 0x11, reg | (bits << 2));
        }
    }
    return ret;
}

static int _set_mic_gain(audio_codec_es8374_t *codec, float gain)
{
    int ret = 0;
    if (gain >= 0 && gain < 24) {
        int gain_n = 0;
        gain_n = (int) gain / 3;
        ret = es8374_write_reg(codec, 0x22, gain_n | (gain_n << 4)); // MIC PGA
    } else {
        ret = -1;
    }
    return ret;
}

int es8374_codec_get_voice_volume(audio_codec_es8374_t *codec, float *volume)
{
    int reg = 0;
    int ret = es8374_read_reg(codec, 0x38, &reg);
    if (ret != CODEC_DEV_OK) {
        *volume = 0;
    } else {
        *volume = audio_codec_calc_vol_reg(&vol_range, reg);
    }
    return ret;
}

static int es8374_i2s_config_clock(audio_codec_es8374_t *codec, es_i2s_clock_t cfg)
{
    int ret = 0;
    int reg = 0;
    ret |= es8374_read_reg(codec, 0x0f, &reg); // power up adc and input
    reg &= 0xe0;
    int divratio = 0;
    switch (cfg.sclk_div) {
        case MCLK_DIV_1:
            divratio = 1;
            break;
        case MCLK_DIV_2: // = 2,
            divratio = 2;
            break;
        case MCLK_DIV_3: // = 3,
            divratio = 3;
            break;
        case MCLK_DIV_4: // = 4,
            divratio = 4;
            break;
        case MCLK_DIV_5: // = 20,
            divratio = 5;
            break;
        case MCLK_DIV_6: // = 5,
            divratio = 6;
            break;
        case MCLK_DIV_7: //  = 29,
            divratio = 7;
            break;
        case MCLK_DIV_8: // = 6,
            divratio = 8;
            break;
        case MCLK_DIV_9: // = 7,
            divratio = 9;
            break;
        case MCLK_DIV_10: // = 21,
            divratio = 10;
            break;
        case MCLK_DIV_11: // = 8,
            divratio = 11;
            break;
        case MCLK_DIV_12: // = 9,
            divratio = 12;
            break;
        case MCLK_DIV_13: // = 30,
            divratio = 13;
            break;
        case MCLK_DIV_14: // = 31
            divratio = 14;
            break;
        case MCLK_DIV_15: // = 22,
            divratio = 15;
            break;
        case MCLK_DIV_16: // = 10,
            divratio = 16;
            break;
        case MCLK_DIV_17: // = 23,
            divratio = 17;
            break;
        case MCLK_DIV_18: // = 11,
            divratio = 18;
            break;
        case MCLK_DIV_20: // = 24,
            divratio = 19;
            break;
        case MCLK_DIV_22: // = 12,
            divratio = 20;
            break;
        case MCLK_DIV_24: // = 13,
            divratio = 21;
            break;
        case MCLK_DIV_25: // = 25,
            divratio = 22;
            break;
        case MCLK_DIV_30: // = 26,
            divratio = 23;
            break;
        case MCLK_DIV_32: // = 27,
            divratio = 24;
            break;
        case MCLK_DIV_33: // = 14,
            divratio = 25;
            break;
        case MCLK_DIV_34: // = 28,
            divratio = 26;
            break;
        case MCLK_DIV_36: // = 15,
            divratio = 27;
            break;
        case MCLK_DIV_44: // = 16,
            divratio = 28;
            break;
        case MCLK_DIV_48: // = 17,
            divratio = 29;
            break;
        case MCLK_DIV_66: // = 18,
            divratio = 30;
            break;
        case MCLK_DIV_72: // = 19,
            divratio = 31;
            break;
        default:
            break;
    }
    reg |= divratio;
    ret |= es8374_write_reg(codec, 0x0f, reg);

    int dacratio_l = 0;
    int dacratio_h = 0;

    switch (cfg.lclk_div) {
        case LCLK_DIV_128:
            dacratio_l = 128 % 256;
            dacratio_h = 128 / 256;
            break;
        case LCLK_DIV_192:
            dacratio_l = 192 % 256;
            dacratio_h = 192 / 256;
            break;
        case LCLK_DIV_256:
            dacratio_l = 256 % 256;
            dacratio_h = 256 / 256;
            break;
        case LCLK_DIV_384:
            dacratio_l = 384 % 256;
            dacratio_h = 384 / 256;
            break;
        case LCLK_DIV_512:
            dacratio_l = 512 % 256;
            dacratio_h = 512 / 256;
            break;
        case LCLK_DIV_576:
            dacratio_l = 576 % 256;
            dacratio_h = 576 / 256;
            break;
        case LCLK_DIV_768:
            dacratio_l = 768 % 256;
            dacratio_h = 768 / 256;
            break;
        case LCLK_DIV_1024:
            dacratio_l = 1024 % 256;
            dacratio_h = 1024 / 256;
            break;
        case LCLK_DIV_1152:
            dacratio_l = 1152 % 256;
            dacratio_h = 1152 / 256;
            break;
        case LCLK_DIV_1408:
            dacratio_l = 1408 % 256;
            dacratio_h = 1408 / 256;
            break;
        case LCLK_DIV_1536:
            dacratio_l = 1536 % 256;
            dacratio_h = 1536 / 256;
            break;
        case LCLK_DIV_2112:
            dacratio_l = 2112 % 256;
            dacratio_h = 2112 / 256;
            break;
        case LCLK_DIV_2304:
            dacratio_l = 2304 % 256;
            dacratio_h = 2304 / 256;
            break;
        case LCLK_DIV_125:
            dacratio_l = 125 % 256;
            dacratio_h = 125 / 256;
            break;
        case LCLK_DIV_136:
            dacratio_l = 136 % 256;
            dacratio_h = 136 / 256;
            break;
        case LCLK_DIV_250:
            dacratio_l = 250 % 256;
            dacratio_h = 250 / 256;
            break;
        case LCLK_DIV_272:
            dacratio_l = 272 % 256;
            dacratio_h = 272 / 256;
            break;
        case LCLK_DIV_375:
            dacratio_l = 375 % 256;
            dacratio_h = 375 / 256;
            break;
        case LCLK_DIV_500:
            dacratio_l = 500 % 256;
            dacratio_h = 500 / 256;
            break;
        case LCLK_DIV_544:
            dacratio_l = 544 % 256;
            dacratio_h = 544 / 256;
            break;
        case LCLK_DIV_750:
            dacratio_l = 750 % 256;
            dacratio_h = 750 / 256;
            break;
        case LCLK_DIV_1000:
            dacratio_l = 1000 % 256;
            dacratio_h = 1000 / 256;
            break;
        case LCLK_DIV_1088:
            dacratio_l = 1088 % 256;
            dacratio_h = 1088 / 256;
            break;
        case LCLK_DIV_1496:
            dacratio_l = 1496 % 256;
            dacratio_h = 1496 / 256;
            break;
        case LCLK_DIV_1500:
            dacratio_l = 1500 % 256;
            dacratio_h = 1500 / 256;
            break;
        default:
            break;
    }
    ret |= es8374_write_reg(codec, 0x06, dacratio_h); // ADCFsMode,singel SPEED,RATIO=256
    ret |= es8374_write_reg(codec, 0x07, dacratio_l); // ADCFsMode,singel SPEED,RATIO=256

    return ret;
}

static int es8374_set_d2se_pga(audio_codec_es8374_t *codec, es_d2se_pga_t gain)
{
    int ret = 0;
    int reg = 0;
    if (gain > D2SE_PGA_GAIN_MIN && gain < D2SE_PGA_GAIN_MAX) {
        ret = es8374_read_reg(codec, 0x21, &reg);
        reg &= 0xfb;
        reg |= gain << 2;
        ret = es8374_write_reg(codec, 0x21, reg); // MIC PGA
    }
    return ret;
}

static int es8374_config_fmt(audio_codec_es8374_t *codec, es_i2s_fmt_t fmt)
{
    int ret = 0;
    int reg = 0;
    int fmt_i2s = fmt & 0x0f;
    if (codec->cfg.codec_mode & CODEC_WORK_MODE_ADC) {
        ret |= es8374_read_reg(codec, 0x10, &reg);
        if (ret == 0) {
            reg = reg & 0xfc;
            ret |= es8374_write_reg(codec, 0x10, reg | fmt_i2s);
        }
    }
    if (codec->cfg.codec_mode & CODEC_WORK_MODE_DAC) {
        ret |= es8374_read_reg(codec, 0x11, &reg);
        if (ret == 0) {
            reg = reg & 0xfc;
            ret |= es8374_write_reg(codec, 0x11, reg | (fmt_i2s));
        }
    }
    return ret;
}

static int es8374_config_dac_output(audio_codec_es8374_t *codec, es_dac_output_t output)
{
    int ret = 0;
    int reg = 0;
    ret = es8374_write_reg(codec, 0x1d, 0x02);
    ret |= es8374_read_reg(codec, 0x1c, &reg); // set spk mixer
    reg |= 0x80;
    ret |= es8374_write_reg(codec, 0x1c, reg);
    ret |= es8374_write_reg(codec, 0x1D, 0x02); // spk set
    ret |= es8374_write_reg(codec, 0x1F, 0x00); // spk set
    ret |= es8374_write_reg(codec, 0x1E, 0xA0); // spk on
    return ret;
}

static int es8374_config_adc_input(audio_codec_es8374_t *codec, es_adc_input_t input)
{
    int ret = 0;
    int reg = 0;
    ret |= es8374_read_reg(codec, 0x21, &reg);
    if (ret == 0) {
        reg = (reg & 0xcf) | 0x14;
        ret |= es8374_write_reg(codec, 0x21, reg);
    }
    return ret;
}

static int es8374_set_adc_dac_volume(audio_codec_es8374_t *codec, codec_work_mode_t mode, float db_value)
{
    int reg = audio_codec_calc_vol_db(&vol_range, db_value);
    int ret = CODEC_DEV_OK;
    if (mode & CODEC_WORK_MODE_ADC) {
        ret = es8374_write_reg(codec, 0x25, (uint8_t) reg);
    }
    if (mode & CODEC_WORK_MODE_DAC) {
        ret = es8374_write_reg(codec, 0x38, (uint8_t) reg);
    }
    return ret;
}

static int es8374_init_reg(audio_codec_es8374_t *codec, es_i2s_fmt_t fmt, es_i2s_clock_t cfg,
                           es_dac_output_t out_channel, es_adc_input_t in_channel)
{
    int ret = 0;
    int reg = 0;

    ret |= es8374_write_reg(codec, 0x00, 0x3F); // IC Rst start
    ret |= es8374_write_reg(codec, 0x00, 0x03); // IC Rst stop
    ret |= es8374_write_reg(codec, 0x01, 0x7F); // IC clk on

    ret |= es8374_read_reg(codec, 0x0F, &reg);
    reg &= 0x7f;

    reg |= (codec->cfg.master_mode << 7);
    ret |= es8374_write_reg(codec, 0x0f, reg); // CODEC IN I2S SLAVE MODE

    ret |= es8374_write_reg(codec, 0x6F, 0xA0); // pll set:mode enable
    ret |= es8374_write_reg(codec, 0x72, 0x41); // pll set:mode set
    ret |= es8374_write_reg(codec, 0x09, 0x01); // pll set:reset on ,set start
    ret |= es8374_write_reg(codec, 0x0C, 0x22); // pll set:k
    ret |= es8374_write_reg(codec, 0x0D, 0x2E); // pll set:k
    ret |= es8374_write_reg(codec, 0x0E, 0xC6); // pll set:k
    ret |= es8374_write_reg(codec, 0x0A, 0x3A); // pll set:
    ret |= es8374_write_reg(codec, 0x0B, 0x07); // pll set:n
    ret |= es8374_write_reg(codec, 0x09, 0x41); // pll set:reset off ,set stop

    ret |= es8374_i2s_config_clock(codec, cfg);

    ret |= es8374_write_reg(codec, 0x24, 0x08); // adc set
    ret |= es8374_write_reg(codec, 0x36, 0x00); // dac set
    ret |= es8374_write_reg(codec, 0x12, 0x30); // timming set
    ret |= es8374_write_reg(codec, 0x13, 0x20); // timming set

    ret |= es8374_config_fmt(codec, fmt);

    ret |= es8374_write_reg(codec, 0x21, 0x50); // adc set: SEL LIN1 CH+PGAGAIN=0DB
    ret |= es8374_write_reg(codec, 0x22, 0xFF); // adc set: PGA GAIN=0DB
    ret |= es8374_write_reg(codec, 0x21, 0x14); // adc set: SEL LIN1 CH+PGAGAIN=18DB
    ret |= es8374_write_reg(codec, 0x22, 0x55); // pga = +15db
    ret |= es8374_write_reg(codec, 0x08,
                            0x21); // set class d divider = 33, to avoid the high frequency tone on laudspeaker
    ret |= es8374_write_reg(codec, 0x00, 0x80); // IC START

    ret |= es8374_set_adc_dac_volume(codec, CODEC_WORK_MODE_ADC, 0.0); // 0db

    ret |= es8374_write_reg(codec, 0x14, 0x8A); // IC START
    ret |= es8374_write_reg(codec, 0x15, 0x40); // IC START
    ret |= es8374_write_reg(codec, 0x1A, 0xA0); // monoout set
    ret |= es8374_write_reg(codec, 0x1B, 0x19); // monoout set
    ret |= es8374_write_reg(codec, 0x1C, 0x90); // spk set
    ret |= es8374_write_reg(codec, 0x1D, 0x01); // spk set
    ret |= es8374_write_reg(codec, 0x1F, 0x00); // spk set
    ret |= es8374_write_reg(codec, 0x1E, 0x20); // spk on
    ret |= es8374_write_reg(codec, 0x28, 0x00); // alc set
    ret |= es8374_write_reg(codec, 0x25, 0x00); // ADCVOLUME on
    ret |= es8374_write_reg(codec, 0x38, 0x00); // DACVOLUME on
    ret |= es8374_write_reg(codec, 0x37, 0x30); // dac set
    ret |= es8374_write_reg(codec, 0x6D, 0x60); // SEL:GPIO1=DMIC CLK OUT+SEL:GPIO2=PLL CLK OUT
    ret |= es8374_write_reg(codec, 0x71, 0x05); // for automute setting
    ret |= es8374_write_reg(codec, 0x73, 0x70);

    ret |= es8374_config_dac_output(codec, out_channel); // 0x3c Enable DAC and Enable Lout/Rout/1/2
    ret |= es8374_config_adc_input(
        codec,
        in_channel); // 0x00 LINSEL & RINSEL, LIN1/RIN1 as ADC Input; DSSEL,use one DS Reg11; DSR, LINPUT1-RINPUT1
    ret |= es8374_set_adc_dac_volume(codec, CODEC_WORK_MODE_DAC, -96.0);
    ret |= es8374_write_reg(codec, 0x37, 0x00); // dac set
    return ret;
}

static int es8374_stop(audio_codec_es8374_t *codec)
{
    int ret = 0;
    int reg = 0;
    if (codec->cfg.codec_mode == CODEC_WORK_MODE_LINE) {
        ret |= es8374_read_reg(codec, 0x1a, &reg); // disable lout
        reg |= 0x08;
        ret |= es8374_write_reg(codec, 0x1a, reg);
        reg &= 0x9f;
        ret |= es8374_write_reg(codec, 0x1a, reg);
        ret |= es8374_write_reg(codec, 0x1D, 0x12); // mute speaker
        ret |= es8374_write_reg(codec, 0x1E, 0x20); // disable class d
        ret |= es8374_read_reg(codec, 0x1c, &reg);  // disable spkmixer
        reg &= 0xbf;
        ret |= es8374_write_reg(codec, 0x1c, reg);
        ret |= es8374_write_reg(codec, 0x1F, 0x00); // spk set
    }
    if (codec->cfg.codec_mode & CODEC_WORK_MODE_DAC) {
        ret |= es8374_set_voice_mute(codec, true);

        ret |= es8374_read_reg(codec, 0x1a, &reg); // disable lout
        reg |= 0x08;
        ret |= es8374_write_reg(codec, 0x1a, reg);
        reg &= 0xdf;
        ret |= es8374_write_reg(codec, 0x1a, reg);
        ret |= es8374_write_reg(codec, 0x1D, 0x12); // mute speaker
        ret |= es8374_write_reg(codec, 0x1E, 0x20); // disable class d
        ret |= es8374_read_reg(codec, 0x15, &reg);  // power up dac
        reg |= 0x20;
        ret |= es8374_write_reg(codec, 0x15, reg);
    }
    if (codec->cfg.codec_mode & CODEC_WORK_MODE_ADC) {
        ret |= es8374_read_reg(codec, 0x10, &reg); // power up adc and input
        reg |= 0xc0;
        ret |= es8374_write_reg(codec, 0x10, reg);
        ret |= es8374_read_reg(codec, 0x21, &reg); // power up adc and input
        reg |= 0xc0;
        ret |= es8374_write_reg(codec, 0x21, reg);
    }
    ESP_LOGI(TAG, "Enter into es8374_stop");
    return ret;
}

static int es8374_start(audio_codec_es8374_t *codec)
{
    int ret = CODEC_DEV_OK;
    int reg = 0;
    bool mode_line = (codec->cfg.codec_mode == CODEC_WORK_MODE_LINE);
    if (mode_line) {
        ret |= es8374_read_reg(codec, 0x1a, &reg); // set monomixer
        reg |= 0x60;
        reg |= 0x20;
        reg &= 0xf7;
        ret |= es8374_write_reg(codec, 0x1a, reg);
        ret |= es8374_read_reg(codec, 0x1c, &reg); // set spk mixer
        reg |= 0x40;
        ret |= es8374_write_reg(codec, 0x1c, reg);
        ret |= es8374_write_reg(codec, 0x1D, 0x02); // spk set
        ret |= es8374_write_reg(codec, 0x1F, 0x00); // spk set
        ret |= es8374_write_reg(codec, 0x1E, 0xA0); // spk on
    }
    if (mode_line || (codec->cfg.codec_mode & CODEC_WORK_MODE_ADC)) {
        ret |= es8374_read_reg(codec, 0x21, &reg); // power up adc and input
        reg &= 0x3f;
        ret |= es8374_write_reg(codec, 0x21, reg);
        ret |= es8374_read_reg(codec, 0x10, &reg); // power up adc and input
        reg &= 0x3f;
        ret |= es8374_write_reg(codec, 0x10, reg);
    }

    if (mode_line || (codec->cfg.codec_mode & CODEC_WORK_MODE_DAC)) {
        ret |= es8374_read_reg(codec, 0x1a, &reg); // disable lout
        reg |= 0x08;
        ret |= es8374_write_reg(codec, 0x1a, reg);
        reg &= 0xdf;
        ret |= es8374_write_reg(codec, 0x1a, reg);
        ret |= es8374_write_reg(codec, 0x1D, 0x12); // mute speaker
        ret |= es8374_write_reg(codec, 0x1E, 0x20); // disable class d
        ret |= es8374_read_reg(codec, 0x15, &reg);  // power up dac
        reg &= 0xdf;
        ret |= es8374_write_reg(codec, 0x15, reg);
        ret |= es8374_read_reg(codec, 0x1a, &reg); // disable lout
        reg |= 0x20;
        ret |= es8374_write_reg(codec, 0x1a, reg);
        reg &= 0xf7;
        ret |= es8374_write_reg(codec, 0x1a, reg);
        ret |= es8374_write_reg(codec, 0x1D, 0x02); // mute speaker
        ret |= es8374_write_reg(codec, 0x1E, 0xa0); // disable class d
        ret |= es8374_set_voice_mute(codec, false);
    }
    return ret;
}

static int es8374_set_mute(const audio_codec_if_t *h, bool mute)
{
    audio_codec_es8374_t *codec = (audio_codec_es8374_t *) h;
    if (codec == NULL || codec->is_open == false) {
        return CODEC_DEV_INVALID_ARG;
    }
    return es8374_set_voice_mute(codec, mute);
}

static int es8374_set_vol(const audio_codec_if_t *h, float db_value)
{
    audio_codec_es8374_t *codec = (audio_codec_es8374_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return es8374_set_adc_dac_volume(codec, CODEC_WORK_MODE_DAC, db_value);
}

static int es8374_set_mic_gain(const audio_codec_if_t *h, float db)
{
    audio_codec_es8374_t *codec = (audio_codec_es8374_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    int ret = _set_mic_gain(codec, db);
    return ret == 0 ? CODEC_DEV_OK : CODEC_DEV_WRITE_FAIL;
}

static void es8374_pa_power(audio_codec_es8374_t *codec, bool enable)
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

static int es8374_open(const audio_codec_if_t *h, void *cfg, int cfg_size)
{
    audio_codec_es8374_t *codec = (audio_codec_es8374_t *) h;
    es8374_codec_cfg_t *codec_cfg = (es8374_codec_cfg_t *) cfg;
    if (codec == NULL || codec_cfg == NULL || codec_cfg->ctrl_if == NULL || cfg_size != sizeof(es8374_codec_cfg_t)) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = CODEC_DEV_OK;
    memcpy(&codec->cfg, codec_cfg, sizeof(es8374_codec_cfg_t));
    es_i2s_clock_t clkdiv;
    clkdiv.lclk_div = LCLK_DIV_256;
    clkdiv.sclk_div = MCLK_DIV_4;
    ret |= es8374_stop(codec);
    ret |= es8374_init_reg(codec, (BIT_LENGTH_16BITS << 4) | ES_I2S_NORMAL, clkdiv, DAC_OUTPUT_ALL,
                           ADC_INPUT_LINPUT1_RINPUT1);
    ret |= _set_mic_gain(codec, 15);
    ret |= es8374_set_d2se_pga(codec, D2SE_PGA_GAIN_EN);
    ret |= es8374_config_fmt(codec, ES_I2S_NORMAL);
    if (ret != 0) {
        return CODEC_DEV_WRITE_FAIL;
    }
    es8374_pa_power(codec, true);
    codec->is_open = true;
    return CODEC_DEV_OK;
}

static int es8374_close(const audio_codec_if_t *h)
{
    audio_codec_es8374_t *codec = (audio_codec_es8374_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open) {
        es8374_stop(codec);
        es8374_write_reg(codec, 0x00, 0x7F); // IC Reset and STOP
        es8374_pa_power(codec, false);
        codec->is_open = false;
    }
    return CODEC_DEV_OK;
}

int es8374_set_fs(const audio_codec_if_t *h, codec_sample_info_t *fs)
{
    audio_codec_es8374_t *codec = (audio_codec_es8374_t *) h;
    if (codec == NULL || codec->is_open == false) {
        return CODEC_DEV_INVALID_ARG;
    }
    int ret = 0;
    es_i2s_fmt_t fmt = ES_I2S_NORMAL;
    if (fs->channel == 1) {
        fmt = ES_I2S_LEFT;
    }
    ret |= es8374_config_fmt(codec, fmt);
    ret |= es8374_set_bits_per_sample(codec, fs->bits_per_sample);
    return CODEC_DEV_OK;
}

int es8374_enable(const audio_codec_if_t *h, bool enable)
{
    int ret = CODEC_DEV_OK;
    audio_codec_es8374_t *codec = (audio_codec_es8374_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    if (enable) {
        ret = es8374_start(codec);
        ESP_LOGI(TAG, "Start ret %d", ret);
    } else {
        ESP_LOGW(TAG, "The codec is about to stop");
        ret = es8374_stop(codec);
        es8374_stop(codec);
        es8374_write_reg(codec, 0x00, 0x7F); // IC Reset and STOP
    }
    return ret;
}

static int es8374_set_reg(const audio_codec_if_t *h, int reg, int value)
{
    audio_codec_es8374_t *codec = (audio_codec_es8374_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return es8374_write_reg(codec, reg, value);
}

static int es8374_get_reg(const audio_codec_if_t *h, int reg, int *value)
{
    audio_codec_es8374_t *codec = (audio_codec_es8374_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return es8374_read_reg(codec, reg, value);
}

const audio_codec_if_t *es8374_codec_new(es8374_codec_cfg_t *codec_cfg)
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
    audio_codec_es8374_t *codec = (audio_codec_es8374_t *) calloc(1, sizeof(audio_codec_es8374_t));
    if (codec == NULL) {
        return NULL;
    }
    codec->base.open = es8374_open;
    codec->base.enable = es8374_enable;
    codec->base.set_fs = es8374_set_fs;
    codec->base.mute = es8374_set_mute;
    codec->base.set_vol = es8374_set_vol;
    codec->base.set_mic_gain = es8374_set_mic_gain;
    codec->base.set_reg = es8374_set_reg;
    codec->base.get_reg = es8374_get_reg;
    codec->base.close = es8374_close;
    do {
        int ret = codec->base.open(&codec->base, codec_cfg, sizeof(es8374_codec_cfg_t));
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
