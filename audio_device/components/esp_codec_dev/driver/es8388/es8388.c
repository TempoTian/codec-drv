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
#include "es8388_reg.h"
#include "es8388.h"
#include "esxxx_common.h"
#include "codec_dev_defaults.h"
#include "codec_dev_err.h"
#include "codec_dev_gpio.h"
#include "codec_dev_utils.h"

#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
#include "headphone_detect.h"
#endif

#define TAG "ES8388"

typedef struct {
    audio_codec_if_t             base;
    const audio_codec_ctrl_if_t *ctrl_if;
    bool                         is_open;
    codec_work_mode_t            codec_mode;
    int16_t                      pa_pin;
} audio_codec_es8388_t;

static const codec_dev_vol_range_t vol_range = {
    .min_vol = {
        .vol = 0xC0,
        .db_value = -96.5,
    },
    .max_vol = {
        .vol = 0,
        .db_value = 0.0,
    },
};

static int es8388_write_reg(audio_codec_es8388_t *codec, int reg, int value)
{
    return codec->ctrl_if->write_addr(codec->ctrl_if, reg, 1, &value, 1);
}

static int es8388_read_reg(audio_codec_es8388_t *codec, int reg, int *value)
{
    *value = 0;
    return codec->ctrl_if->read_addr(codec->ctrl_if, reg, 1, value, 1);
}

void es8388_read_all(audio_codec_es8388_t *codec)
{
    for (uint8_t i = 0; i < 50; i++) {
        int reg = 0;
        es8388_read_reg(codec, i, &reg);
        ets_printf("%x: %x\n", i, reg);
    }
}

static int es8388_set_adc_dac_volume(audio_codec_es8388_t *codec, codec_work_mode_t mode, int volume, int dot)
{
    int res = 0;
    if (volume < -96 || volume > 0) {
        ESP_LOGW(TAG, "Warning: volume < -96! or > 0!\n");
        if (volume < -96)
            volume = -96;
        else
            volume = 0;
    }
    dot = (dot >= 5 ? 1 : 0);
    volume = (-volume << 1) + dot;
    if (mode & CODEC_WORK_MODE_ADC) {
        res |= es8388_write_reg(codec, ES8388_ADCCONTROL8, volume);
        res |= es8388_write_reg(codec, ES8388_ADCCONTROL9, volume); // ADC Right Volume=0db
    }
    if (mode & CODEC_WORK_MODE_DAC) {
        res |= es8388_write_reg(codec, ES8388_DACCONTROL5, volume);
        res |= es8388_write_reg(codec, ES8388_DACCONTROL4, volume);
    }
    return res;
}

/**
 * @brief Configure ES8388 DAC mute or not. Basically you can use this function to mute the output or unmute
 *
 * @param enable: enable or disable
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
static int es8388_set_voice_mute(audio_codec_es8388_t *codec, bool enable)
{
    int res = 0;
    int reg = 0;
    res = es8388_read_reg(codec, ES8388_DACCONTROL3, &reg);
    reg = reg & 0xFB;
    res |= es8388_write_reg(codec, ES8388_DACCONTROL3, reg | (((int) enable) << 2));
    return res;
}

/**
 * @brief Power Management
 *
 * @param mod:      if ES_POWER_CHIP, the whole chip including ADC and DAC is enabled
 * @param enable:   false to disable true to enable
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
static int es8388_start(audio_codec_es8388_t *codec, codec_work_mode_t mode)
{
    int res = 0;
    int prev_data = 0, data = 0;
    es8388_read_reg(codec, ES8388_DACCONTROL21, &prev_data);
    if (mode == CODEC_WORK_MODE_LINE) {
        res |= es8388_write_reg(codec, ES8388_DACCONTROL16,
                                0x09); // 0x00 audio on LIN1&RIN1,  0x09 LIN2&RIN2 by pass enable
        res |= es8388_write_reg(
            codec, ES8388_DACCONTROL17,
            0x50); // left DAC to left mixer enable  and  LIN signal to left mixer enable 0db  : bupass enable
        res |= es8388_write_reg(
            codec, ES8388_DACCONTROL20,
            0x50); // right DAC to right mixer enable  and  LIN signal to right mixer enable 0db : bupass enable
        res |= es8388_write_reg(codec, ES8388_DACCONTROL21, 0xC0); // enable adc
    } else {
        res |= es8388_write_reg(codec, ES8388_DACCONTROL21, 0x80); // enable dac
    }
    es8388_read_reg(codec, ES8388_DACCONTROL21, &data);
    if (prev_data != data) {
        res |= es8388_write_reg(codec, ES8388_CHIPPOWER, 0xF0); // start state machine
        // res |= es8388_write_reg(codec, ES8388_CONTROL1, 0x16);
        // res |= es8388_write_reg(codec, ES8388_CONTROL2, 0x50);
        res |= es8388_write_reg(codec, ES8388_CHIPPOWER, 0x00); // start state machine
    }
    if ((mode & CODEC_WORK_MODE_ADC) || mode == CODEC_WORK_MODE_LINE) {
        res |= es8388_write_reg(codec, ES8388_ADCPOWER, 0x00); // power up adc and line in
    }
    if ((mode & CODEC_WORK_MODE_DAC) || mode == CODEC_WORK_MODE_LINE) {
        res |= es8388_write_reg(codec, ES8388_DACPOWER, 0x3c); // power up dac and line out
        res |= es8388_set_voice_mute(codec, false);
        ESP_LOGI(TAG, "es8388_start default is mode:%d", mode);
    }
    return res;
}

/**
 * @brief Power Management
 *
 * @param mod:      if ES_POWER_CHIP, the whole chip including ADC and DAC is enabled
 * @param enable:   false to disable true to enable
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
static int es8388_stop(audio_codec_es8388_t *codec, codec_work_mode_t mode)
{
    int res = 0;
    if (mode == CODEC_WORK_MODE_LINE) {
        res |= es8388_write_reg(codec, ES8388_DACCONTROL21, 0x80); // enable dac
        res |= es8388_write_reg(codec, ES8388_DACCONTROL16, 0x00); // 0x00 audio on LIN1&RIN1,  0x09 LIN2&RIN2
        res |= es8388_write_reg(codec, ES8388_DACCONTROL17, 0x90); // only left DAC to left mixer enable 0db
        res |= es8388_write_reg(codec, ES8388_DACCONTROL20, 0x90); // only right DAC to right mixer enable 0db
        return res;
    }
    if (mode & CODEC_WORK_MODE_DAC) {
        res |= es8388_write_reg(codec, ES8388_DACPOWER, 0x00);
        res |= es8388_set_voice_mute(codec, true); // res |= Es8388SetAdcDacVolume(ES_MODULE_DAC, -96, 5);      // 0db
        // res |= es8388_write_reg(codec, ES8388_DACPOWER, 0xC0);  //power down dac and line out
    }
    if (mode & CODEC_WORK_MODE_ADC) {
        // res |= Es8388SetAdcDacVolume(ES_MODULE_ADC, -96, 5);      // 0db
        res |= es8388_write_reg(codec, ES8388_ADCPOWER, 0xFF); // power down adc and line in
    }
    if (mode == CODEC_WORK_MODE_BOTH) {
        res |= es8388_write_reg(codec, ES8388_DACCONTROL21, 0x9C); // disable mclk
        //        res |= es8388_write_reg(codec, ES8388_CONTROL1, 0x00);
        //        res |= es8388_write_reg(codec, ES8388_CONTROL2, 0x58);
        //        res |= es8388_write_reg(codec, ES8388_CHIPPOWER, 0xF3);  //stop state machine
    }
    return res;
}

/**
 * @brief Config I2s clock in MSATER mode
 *
 * @param cfg.sclkDiv:      generate SCLK by dividing MCLK in MSATER mode
 * @param cfg.lclkDiv:      generate LCLK by dividing MCLK in MSATER mode
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int es8388_i2s_config_clock(audio_codec_es8388_t *codec, es_i2s_clock_t cfg)
{
    int res = 0;
    res |= es8388_write_reg(codec, ES8388_MASTERMODE, cfg.sclk_div);
    res |= es8388_write_reg(codec, ES8388_ADCCONTROL5, cfg.lclk_div); // ADCFsMode,singel SPEED,RATIO=256
    res |= es8388_write_reg(codec, ES8388_DACCONTROL2, cfg.lclk_div); // ADCFsMode,singel SPEED,RATIO=256
    return res;
}

/**
 * @brief Configure ES8388 I2S format
 *
 * @param mode:           set ADC or DAC or all
 * @param bitPerSample:   see Es8388I2sFmt
 *
 * @return
 *     - (-1) Error
 *     - (0)  Success
 */
static int es8388_config_fmt(audio_codec_es8388_t *codec, codec_work_mode_t mode, es_i2s_fmt_t fmt)
{
    int res = 0;
    int reg = 0;
    if (mode & CODEC_WORK_MODE_ADC) {
        res = es8388_read_reg(codec, ES8388_ADCCONTROL4, &reg);
        reg = reg & 0xfc;
        res |= es8388_write_reg(codec, ES8388_ADCCONTROL4, reg | fmt);
    }
    if (mode & CODEC_WORK_MODE_DAC) {
        res = es8388_read_reg(codec, ES8388_DACCONTROL1, &reg);
        reg = reg & 0xf9;
        res |= es8388_write_reg(codec, ES8388_DACCONTROL1, reg | (fmt << 1));
    }
    return res;
}

/**
 *
 * @return
 *           volume
 */
int es8388_get_voice_volume(audio_codec_es8388_t *codec, int *volume)
{
    int res = 0;
    int reg = 0;
    res = es8388_read_reg(codec, ES8388_DACCONTROL24, &reg);
    if (res != 0) {
        *volume = 0;
    } else {
        *volume = reg;
        *volume *= 3;
        if (*volume == 99)
            *volume = 100;
    }
    return res;
}

int es8388_get_voice_mute(audio_codec_es8388_t *codec)
{
    int res = 0;
    int reg = 0;
    res = es8388_read_reg(codec, ES8388_DACCONTROL3, &reg);
    if (res == 0) {
        reg = (reg & 0x04) >> 2;
    }
    return res == 0 ? reg : res;
}

/**
 * @param gain: Config DAC Output
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int es8388_config_dac_output(audio_codec_es8388_t *codec, int output)
{
    int res;
    int reg = 0;
    res = es8388_read_reg(codec, ES8388_DACPOWER, &reg);
    reg = reg & 0xc3;
    res |= es8388_write_reg(codec, ES8388_DACPOWER, reg | output);
    return res;
}

/**
 * @param gain: Config ADC input
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int es8388_config_adc_input(audio_codec_es8388_t *codec, es_adc_input_t input)
{
    int res;
    int reg = 0;
    res = es8388_read_reg(codec, ES8388_ADCCONTROL2, &reg);
    reg = reg & 0x0f;
    res |= es8388_write_reg(codec, ES8388_ADCCONTROL2, reg | input);
    return res;
}

/**
 * @param gain: see es_mic_gain_t
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
static int es8388_set_mic_gain(audio_codec_es8388_t *codec, float db)
{
    es_mic_gain_t gain = db > 0 ? (int) (db / 3) + MIC_GAIN_0DB : MIC_GAIN_0DB;
    int res, gain_n;
    gain_n = (int) gain / 3;
    gain_n = (gain_n << 4) + gain_n;
    res = es8388_write_reg(codec, ES8388_ADCCONTROL1, gain_n); // MIC PGA
    return res;
}

/**
 * @brief Configure ES8388 data sample bits
 *
 * @param mode:             set ADC or DAC or all
 * @param bitPerSample:   see BitsLength
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
static es_bits_length_t get_bits_enum(uint8_t bits)
{
    switch (bits) {
        case 16:
        default:
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

static int es8388_set_bits_per_sample(audio_codec_es8388_t *codec, codec_work_mode_t mode, uint8_t bits_length)
{
    int res = 0;
    int reg = 0;
    int bits = (int) get_bits_enum(bits_length);

    if (mode & CODEC_WORK_MODE_ADC) {
        res = es8388_read_reg(codec, ES8388_ADCCONTROL4, &reg);
        reg = reg & 0xe3;
        res |= es8388_write_reg(codec, ES8388_ADCCONTROL4, reg | (bits << 2));
    }
    if (mode & CODEC_WORK_MODE_DAC) {
        res = es8388_read_reg(codec, ES8388_DACCONTROL1, &reg);
        reg = reg & 0xc7;
        res |= es8388_write_reg(codec, ES8388_DACCONTROL1, reg | (bits << 3));
    }
    return res;
}

static void es8388_pa_power(audio_codec_es8388_t *codec, bool enable)
{
    int16_t pa_pin = codec->pa_pin;
    const audio_codec_gpio_if_t *gpio_if = audio_codec_get_gpio_if();
    if (pa_pin == -1 || gpio_if == NULL) {
        return;
    }
    gpio_if->setup(pa_pin, AUDIO_GPIO_DIR_OUT, AUDIO_GPIO_MODE_FLOAT);
    gpio_if->set(pa_pin, enable);
    ESP_LOGI(TAG, "PA gpio %d enable %d", pa_pin, enable);
}

static int es8388_open(const audio_codec_if_t *h, void *cfg, int cfg_size)
{
    audio_codec_es8388_t *codec = (audio_codec_es8388_t *) h;
    es8388_codec_cfg_t *codec_cfg = (es8388_codec_cfg_t *) cfg;
    if (codec == NULL || codec_cfg->ctrl_if == NULL || cfg_size != sizeof(es8388_codec_cfg_t)) {
        return CODEC_DEV_INVALID_ARG;
    }
    int res = CODEC_DEV_OK;
    codec->ctrl_if = codec_cfg->ctrl_if;
    codec->pa_pin = codec_cfg->pa_pin;
    codec->codec_mode = codec_cfg->codec_mode;

    // 0x04 mute/0x00 unmute&ramp;DAC unmute and  disabled digital volume control soft ramp
    res |= es8388_write_reg(codec, ES8388_DACCONTROL3, 0x04);
    /* Chip Control and Power Management */
    res |= es8388_write_reg(codec, ES8388_CONTROL2, 0x50);
    res |= es8388_write_reg(codec, ES8388_CHIPPOWER, 0x00); // normal all and power up all

    // Disable the internal DLL to improve 8K sample rate
    res |= es8388_write_reg(codec, 0x35, 0xA0);
    res |= es8388_write_reg(codec, 0x37, 0xD0);
    res |= es8388_write_reg(codec, 0x39, 0xD0);

    res |= es8388_write_reg(codec, ES8388_MASTERMODE, codec_cfg->master_mode); // CODEC IN I2S SLAVE MODE

    /* dac */
    res |= es8388_write_reg(codec, ES8388_DACPOWER, 0xC0); // disable DAC and disable Lout/Rout/1/2
    res |= es8388_write_reg(codec, ES8388_CONTROL1, 0x12); // Enfr=0,Play&Record Mode,(0x17-both of mic&paly)
    //    res |= es8388_write_reg(codec, ES8388_CONTROL2, 0);  //LPVrefBuf=0,Pdn_ana=0
    res |= es8388_write_reg(codec, ES8388_DACCONTROL1, 0x18);  // 1a 0x18:16bit iis , 0x00:24
    res |= es8388_write_reg(codec, ES8388_DACCONTROL2, 0x02);  // DACFsMode,SINGLE SPEED; DACFsRatio,256
    res |= es8388_write_reg(codec, ES8388_DACCONTROL16, 0x00); // 0x00 audio on LIN1&RIN1,  0x09 LIN2&RIN2
    res |= es8388_write_reg(codec, ES8388_DACCONTROL17, 0x90); // only left DAC to left mixer enable 0db
    res |= es8388_write_reg(codec, ES8388_DACCONTROL20, 0x90); // only right DAC to right mixer enable 0db
    // set internal ADC and DAC use the same LRCK clock, ADC LRCK as internal LRCK
    res |= es8388_write_reg(codec, ES8388_DACCONTROL21, 0x80);
    res |= es8388_write_reg(codec, ES8388_DACCONTROL23, 0x00);    // vroi=0
    res |= es8388_set_adc_dac_volume(codec, ES_MODULE_DAC, 0, 0); // 0db
    // TODO default use DAC_ALL
#if 0
    if (AUDIO_HAL_DAC_OUTPUT_LINE2 == dac_output) {
        tmp = DAC_OUTPUT_LOUT1 | DAC_OUTPUT_ROUT1;
    } else if (AUDIO_HAL_DAC_OUTPUT_LINE1 == dac_output) {
        tmp = DAC_OUTPUT_LOUT2 | DAC_OUTPUT_ROUT2;
    } else {
        tmp = DAC_OUTPUT_LOUT1 | DAC_OUTPUT_LOUT2 | DAC_OUTPUT_ROUT1 | DAC_OUTPUT_ROUT2;
    }
#endif
    int tmp = DAC_OUTPUT_LOUT1 | DAC_OUTPUT_LOUT2 | DAC_OUTPUT_ROUT1 | DAC_OUTPUT_ROUT2;
    res |= es8388_write_reg(codec, ES8388_DACPOWER, tmp); // 0x3c Enable DAC and Enable Lout/Rout/1/2
    /* adc */
    res |= es8388_write_reg(codec, ES8388_ADCPOWER, 0xFF);
    res |= es8388_write_reg(codec, ES8388_ADCCONTROL1, 0xbb); // MIC Left and Right channel PGA gain
    tmp = 0;
    // TODO default use ADC LINE1
#if 0
    es_adc_input_t adc_input = AUDIO_HAL_ADC_INPUT_LINE1;
    if (AUDIO_HAL_ADC_INPUT_LINE1 == adc_input) {
        tmp = ADC_INPUT_LINPUT1_RINPUT1;
    } else if (AUDIO_HAL_ADC_INPUT_LINE2 == adc_input) {
        tmp = ADC_INPUT_LINPUT2_RINPUT2;
    } else {
        tmp = ADC_INPUT_DIFFERENCE;
    }
#endif
    // 0x00 LINSEL & RINSEL, LIN1/RIN1 as ADC Input; DSSEL,use one DS Reg11; DSR, LINPUT1-RINPUT1
    res |= es8388_write_reg(codec, ES8388_ADCCONTROL2, ADC_INPUT_LINPUT1_RINPUT1);
    res |= es8388_write_reg(codec, ES8388_ADCCONTROL3, 0x02);
    res |= es8388_write_reg(codec, ES8388_ADCCONTROL4, 0x0c); // 16 Bits length and I2S serial audio data format
    res |= es8388_write_reg(codec, ES8388_ADCCONTROL5, 0x02); // ADCFsMode,singel SPEED,RATIO=256
    // ALC for Microphone
    res |= es8388_set_adc_dac_volume(codec, CODEC_WORK_MODE_ADC, 0, 0); // 0db
    res |= es8388_write_reg(codec, ES8388_ADCPOWER,
                            0x09); // Power on ADC, Enable LIN&RIN, Power off MICBIAS, set int1lp to low power mode
    if (res != 0) {
        ESP_LOGI(TAG, "Fail to write register");
        return CODEC_DEV_WRITE_FAIL;
    }
    /* enable es8388 PA */
    es8388_pa_power(codec, true);
    codec->is_open = true;
    return CODEC_DEV_OK;
}

static int es8388_enable(const audio_codec_if_t *h, bool enable)
{
    audio_codec_es8388_t *codec = (audio_codec_es8388_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    int res;
    if (enable == false) {
        res = es8388_stop(codec, codec->codec_mode);
        es8388_pa_power(codec, false);
    } else {
        res = es8388_start(codec, codec->codec_mode);
        es8388_pa_power(codec, true);
    }
    return res;
}

static int es8388_mute(const audio_codec_if_t *h, bool mute)
{
    audio_codec_es8388_t *codec = (audio_codec_es8388_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return es8388_set_voice_mute(codec, mute);
    ;
}

static int es8388_set_vol(const audio_codec_if_t *h, float db_value)
{
    audio_codec_es8388_t *codec = (audio_codec_es8388_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    int volume = audio_codec_calc_vol_reg(&vol_range, db_value);
    ESP_LOGI(TAG, "SET: volume:%x db:%f", volume, db_value);
    int res = es8388_write_reg(codec, ES8388_DACCONTROL24, volume);
    res |= es8388_write_reg(codec, ES8388_DACCONTROL25, volume);
    res |= es8388_write_reg(codec, ES8388_DACCONTROL26, 0);
    res |= es8388_write_reg(codec, ES8388_DACCONTROL27, 0);
    return res ? CODEC_DEV_WRITE_FAIL : CODEC_DEV_OK;
}

static int es8388_set_fs(const audio_codec_if_t *h, codec_sample_info_t *fs)
{
    audio_codec_es8388_t *codec = (audio_codec_es8388_t *) h;
    if (codec == NULL || fs == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    int res = 0;
    // TODO force to use normal i2s mode
    es_i2s_fmt_t fmt = ES_I2S_NORMAL;
    if (fs->channel == 1) {
        fmt = ES_I2S_LEFT;
    }
    res |= es8388_config_fmt(codec, CODEC_WORK_MODE_BOTH, fmt);
    res |= es8388_set_bits_per_sample(codec, CODEC_WORK_MODE_BOTH, fs->bits_per_sample);
    return res;
}

static int es8388_set_gain(const audio_codec_if_t *h, float db)
{
    audio_codec_es8388_t *codec = (audio_codec_es8388_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    return es8388_set_mic_gain(codec, db);
}

static int es8388_close(const audio_codec_if_t *h)
{
    audio_codec_es8388_t *codec = (audio_codec_es8388_t *) h;
    if (codec == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (codec->is_open) {
        es8388_pa_power(codec, false);
        codec->is_open = false;
    }
    return CODEC_DEV_OK;
}

const audio_codec_if_t *es8388_codec_new(es8388_codec_cfg_t *codec_cfg)
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
    audio_codec_es8388_t *codec = (audio_codec_es8388_t *) calloc(1, sizeof(audio_codec_es8388_t));
    if (codec == NULL) {
        return NULL;
    }
    codec->ctrl_if = codec_cfg->ctrl_if;
    // overwrite functions
    codec->base.open = es8388_open;
    codec->base.enable = es8388_enable;
    codec->base.set_fs = es8388_set_fs;
    codec->base.set_vol = es8388_set_vol;
    codec->base.mute = es8388_mute;
    codec->base.set_mic_gain = es8388_set_gain;
    codec->base.close = es8388_close;
    do {
        int ret = codec->base.open(&codec->base, codec_cfg, sizeof(es8388_codec_cfg_t));
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
