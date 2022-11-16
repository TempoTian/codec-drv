/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "unity.h"
#include "test_utils.h"
#include "esp_codec_dev.h"

/*
 * Customized codec realization
 */
typedef enum {
    MY_CODEC_REG_VOL,
    MY_CODEC_REG_MUTE,
    MY_CODEC_REG_MIC_GAIN,
    MY_CODEC_REG_MIC_MUTE,
    MY_CODEC_REG_MAX,
} my_codec_reg_type_t;

typedef struct {
    audio_codec_ctrl_if_t base;
    uint8_t               reg[MY_CODEC_REG_MAX];
    bool                  is_open;
} my_codec_ctrl_t;

typedef struct {
    audio_codec_data_if_t base;
    codec_sample_info_t   fmt;
    int                   read_idx;
    int                   write_idx;
    bool                  is_open;
} my_codec_data_t;

typedef struct {
    const audio_codec_ctrl_if_t *ctrl_if;
} my_codec_cfg_t;

typedef struct {
    audio_codec_if_t             base;
    const audio_codec_ctrl_if_t *ctrl_if;
    codec_sample_info_t          fs;
    bool                         is_open;
    bool                         enable;
} my_codec_t;

/*
 * Fake implementation of codec data interface
 */
static int my_codec_data_open(const audio_codec_data_if_t *h, void *data_cfg, int cfg_size)
{
    my_codec_data_t *data_if = (my_codec_data_t *) h;
    data_if->is_open = true;
    return 0;
}
static bool my_codec_data_is_open(const audio_codec_data_if_t *h)
{
    my_codec_data_t *data_if = (my_codec_data_t *) h;
    return data_if->is_open;
}

static int my_codec_data_set_fmt(const audio_codec_data_if_t *h, codec_sample_info_t *fs)
{
    my_codec_data_t *data_if = (my_codec_data_t *) h;
    memcpy(&data_if->fmt, fs, sizeof(codec_sample_info_t));
    return 0;
}

static int my_codec_data_read(const audio_codec_data_if_t *h, uint8_t *data, int size)
{
    my_codec_data_t *data_if = (my_codec_data_t *) h;
    for (int i = 0; i < size; i++) {
        data[i] = (int) (data_if->read_idx + i);
    }
    data_if->read_idx += size;
    return 0;
}

static int my_codec_data_write(const audio_codec_data_if_t *h, uint8_t *data, int size)
{
    my_codec_data_t *data_if = (my_codec_data_t *) h;
    data_if->write_idx += size;
    return 0;
}

static int my_codec_data_close(const audio_codec_data_if_t *h)
{
    my_codec_data_t *data_if = (my_codec_data_t *) h;
    data_if->is_open = false;
    return 0;
}

static const audio_codec_data_if_t *my_codec_data_new()
{
    my_codec_data_t *data_if = (my_codec_data_t *) calloc(1, sizeof(my_codec_data_t));
    if (data_if == NULL) {
        return NULL;
    }
    data_if->base.open = my_codec_data_open;
    data_if->base.is_open = my_codec_data_is_open;
    data_if->base.set_fmt = my_codec_data_set_fmt;
    data_if->base.read = my_codec_data_read;
    data_if->base.write = my_codec_data_write;
    data_if->base.close = my_codec_data_close;
    data_if->base.open(&data_if->base, NULL, 0);
    return &data_if->base;
}

/*
 * Fake implementation of codec control interface
 */
static int my_codec_ctrl_open(const audio_codec_ctrl_if_t *ctrl, void *cfg, int cfg_size)
{
    my_codec_ctrl_t *ctrl_if = (my_codec_ctrl_t *) ctrl;
    ctrl_if->is_open = true;
    return 0;
}

static bool my_codec_ctrl_is_open(const audio_codec_ctrl_if_t *ctrl)
{
    my_codec_ctrl_t *ctrl_if = (my_codec_ctrl_t *) ctrl;
    return ctrl_if->is_open;
}

static int my_codec_ctrl_read_addr(const audio_codec_ctrl_if_t *ctrl, int addr, int addr_len, void *data, int data_len)
{
    my_codec_ctrl_t *ctrl_if = (my_codec_ctrl_t *) ctrl;
    if (data_len == 1 && addr < MY_CODEC_REG_MAX) {
        *(uint8_t *) data = ctrl_if->reg[addr];
        return 0;
    }
    return -1;
}

static int my_codec_ctrl_write_addr(const audio_codec_ctrl_if_t *ctrl, int addr, int addr_len, void *data, int data_len)
{
    my_codec_ctrl_t *ctrl_if = (my_codec_ctrl_t *) ctrl;
    if (data_len == 1 && addr < MY_CODEC_REG_MAX) {
        ctrl_if->reg[addr] = *(uint8_t *) data;
        return 0;
    }
    return -1;
}

static int my_codec_ctrl_close(const audio_codec_ctrl_if_t *ctrl)
{
    my_codec_ctrl_t *ctrl_if = (my_codec_ctrl_t *) ctrl;
    ctrl_if->is_open = false;
    return 0;
}

const audio_codec_ctrl_if_t *my_codec_ctrl_new()
{
    my_codec_ctrl_t *ctrl_if = (my_codec_ctrl_t *) calloc(1, sizeof(my_codec_ctrl_t));
    if (ctrl_if == NULL) {
        return NULL;
    }
    ctrl_if->base.open = my_codec_ctrl_open;
    ctrl_if->base.is_open = my_codec_ctrl_is_open;
    ctrl_if->base.read_addr = my_codec_ctrl_read_addr;
    ctrl_if->base.write_addr = my_codec_ctrl_write_addr;
    ctrl_if->base.close = my_codec_ctrl_close;
    ctrl_if->base.open(&ctrl_if->base, NULL, 0);
    return &ctrl_if->base;
}

/*
 * Fake implementation of codec interface
 */
static int my_codec_open(const audio_codec_if_t *h, void *cfg, int cfg_size)
{
    my_codec_cfg_t *codec_cfg = (my_codec_cfg_t *) cfg;
    if (cfg_size != sizeof(my_codec_cfg_t)) {
        return -1;
    }
    my_codec_t *codec = (my_codec_t *) h;
    codec->ctrl_if = codec_cfg->ctrl_if;
    codec->is_open = true;
    return 0;
}

static bool my_codec_is_open(const audio_codec_if_t *h)
{
    my_codec_t *codec = (my_codec_t *) h;
    return codec->is_open;
}

static int my_codec_enable(const audio_codec_if_t *h, bool enable)
{
    my_codec_t *codec = (my_codec_t *) h;
    codec->enable = enable;
    return 0;
}

static int my_codec_set_fs(const audio_codec_if_t *h, codec_sample_info_t *fs)
{
    my_codec_t *codec = (my_codec_t *) h;
    memcpy(&codec->fs, fs, sizeof(codec_sample_info_t));
    return 0;
}

static int my_codec_mute(const audio_codec_if_t *h, bool mute)
{
    my_codec_t *codec = (my_codec_t *) h;
    uint8_t data = (uint8_t) mute;
    return codec->ctrl_if->write_addr(codec->ctrl_if, MY_CODEC_REG_MUTE, 1, &data, 1);
}

static int my_codec_set_vol(const audio_codec_if_t *h, float db)
{
    my_codec_t *codec = (my_codec_t *) h;
    uint8_t data = (uint8_t) (int) db;
    return codec->ctrl_if->write_addr(codec->ctrl_if, MY_CODEC_REG_VOL, 1, &data, 1);
}

static int my_codec_set_mic_gain(const audio_codec_if_t *h, float db)
{
    my_codec_t *codec = (my_codec_t *) h;
    uint8_t data = (uint8_t) (int) db;
    return codec->ctrl_if->write_addr(codec->ctrl_if, MY_CODEC_REG_MIC_GAIN, 1, &data, 1);
}

static int my_codec_mute_mic(const audio_codec_if_t *h, bool mute)
{
    my_codec_t *codec = (my_codec_t *) h;
    uint8_t data = (uint8_t) (int) mute;
    return codec->ctrl_if->write_addr(codec->ctrl_if, MY_CODEC_REG_MIC_MUTE, 1, &data, 1);
}

static int my_codec_set_reg(const audio_codec_if_t *h, int reg, int value)
{
    my_codec_t *codec = (my_codec_t *) h;
    if (reg < MY_CODEC_REG_MAX) {
        return codec->ctrl_if->write_addr(codec->ctrl_if, reg, 1, &value, 1);
    }
    return -1;
}

int my_codec_get_reg(const audio_codec_if_t *h, int reg, int *value)
{
    my_codec_t *codec = (my_codec_t *) h;
    if (reg < MY_CODEC_REG_MAX) {
        *value = 0;
        return codec->ctrl_if->read_addr(codec->ctrl_if, reg, 1, value, 1);
    }
    return -1;
}

int my_codec_close(const audio_codec_if_t *h)
{
    my_codec_t *codec = (my_codec_t *) h;
    codec->is_open = false;
    return 0;
}

const audio_codec_if_t *my_codec_new(my_codec_cfg_t *codec_cfg)
{
    my_codec_t *codec = (my_codec_t *) calloc(1, sizeof(my_codec_t));
    if (codec == NULL) {
        return NULL;
    }
    codec->base.open = my_codec_open;
    codec->base.is_open = my_codec_is_open;
    codec->base.enable = my_codec_enable;
    codec->base.set_fs = my_codec_set_fs;
    codec->base.mute = my_codec_mute;
    codec->base.set_vol = my_codec_set_vol;
    codec->base.set_mic_gain = my_codec_set_mic_gain;
    codec->base.mute_mic = my_codec_mute_mic;
    codec->base.set_reg = my_codec_set_reg;
    codec->base.get_reg = my_codec_get_reg;
    codec->base.close = my_codec_close;
    codec->base.open(&codec->base, codec_cfg, sizeof(my_codec_cfg_t));
    return &codec->base;
}

/*
 * Test case for esp_codec_dev API using fake interface
 */
TEST_CASE("esp codec dev API test", "[esp_codec_dev]")
{
    const audio_codec_ctrl_if_t *ctrl_if = my_codec_ctrl_new();
    TEST_ASSERT_NOT_NULL(ctrl_if);
    my_codec_ctrl_t *codec_ctrl = (my_codec_ctrl_t *) ctrl_if;
    const audio_codec_data_if_t *data_if = my_codec_data_new();
    TEST_ASSERT_NOT_NULL(data_if);
    my_codec_data_t *codec_data = (my_codec_data_t *) data_if;
    my_codec_cfg_t codec_cfg = {
        .ctrl_if = ctrl_if,
    };
    const audio_codec_if_t *codec_if = my_codec_new(&codec_cfg);
    TEST_ASSERT_NOT_NULL(codec_if);
    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = CODEC_DEV_TYPE_IN_OUT,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    esp_codec_dev_handle_t dev = esp_codec_dev_new(&dev_cfg);
    TEST_ASSERT_NOT_NULL(codec_if);

    codec_sample_info_t fs = {
        .bits_per_sample = 16,
        .sample_rate = 48000,
        .channel = 2,
    };
    int ret = esp_codec_dev_open(dev, &fs);
    TEST_ESP_OK(ret);
    codec_dev_vol_map_t vol_maps[2] = {
        {.vol = 0,   .db_value = 0  },
        {.vol = 100, .db_value = 100},
    };
    esp_codec_dev_vol_curve_t vol_curve = {
        .count = 2,
        .vol_map = vol_maps,
    };
    // Test for volume curve settings
    ret = esp_codec_dev_set_vol_curve(dev, &vol_curve);
    TEST_ESP_OK(ret);

    // Test for volume setting
    ret = esp_codec_dev_set_out_vol(dev, 30.0);
    TEST_ESP_OK(ret);
    TEST_ASSERT_EQUAL(30, codec_ctrl->reg[MY_CODEC_REG_VOL]);
    ret = esp_codec_dev_set_out_vol(dev, 80.0);
    TEST_ESP_OK(ret);
    TEST_ASSERT_EQUAL(80, codec_ctrl->reg[MY_CODEC_REG_VOL]);

    // test for mute setting
    ret = esp_codec_dev_set_out_mute(dev, true);
    TEST_ESP_OK(ret);
    TEST_ASSERT_EQUAL(1, codec_ctrl->reg[MY_CODEC_REG_MUTE]);
    TEST_ESP_OK(ret);
    ret = esp_codec_dev_set_out_mute(dev, false);
    TEST_ASSERT_EQUAL(0, codec_ctrl->reg[MY_CODEC_REG_MUTE]);

    // test for microphone gain
    ret = esp_codec_dev_set_in_gain(dev, 20.0);
    TEST_ESP_OK(ret);
    TEST_ASSERT_EQUAL(20, codec_ctrl->reg[MY_CODEC_REG_MIC_GAIN]);
    ret = esp_codec_dev_set_in_gain(dev, 40.0);
    TEST_ESP_OK(ret);
    TEST_ASSERT_EQUAL(40, codec_ctrl->reg[MY_CODEC_REG_MIC_GAIN]);

    // test for microphone mute setting
    ret = esp_codec_dev_set_in_mute(dev, true);
    TEST_ESP_OK(ret);
    TEST_ASSERT_EQUAL(1, codec_ctrl->reg[MY_CODEC_REG_MIC_MUTE]);
    ret = esp_codec_dev_set_in_mute(dev, false);
    TEST_ESP_OK(ret);
    TEST_ASSERT_EQUAL(0, codec_ctrl->reg[MY_CODEC_REG_MIC_MUTE]);

    // test for read data
    uint8_t *data = (uint8_t *) calloc(1, 512);
    TEST_ASSERT_NOT_NULL(data);
    ret = esp_codec_dev_read(dev, data, 256);
    TEST_ESP_OK(ret);
    ret = esp_codec_dev_read(dev, data + 256, 256);
    TEST_ESP_OK(ret);
    for (int i = 0; i < 512; i++) {
        uint8_t v = (uint8_t) i;
        TEST_ASSERT_EQUAL(v, data[i]);
    }
    // test for write data
    ret = esp_codec_dev_write(dev, data, 512);
    TEST_ESP_OK(ret);
    TEST_ASSERT_EQUAL(512, codec_data->write_idx);

    esp_codec_dev_close(dev);

    // Test for volume curve settings
    ret = esp_codec_dev_set_vol_curve(dev, &vol_curve);
    TEST_ASSERT(ret == 0);

    // Test for volume setting
    ret = esp_codec_dev_set_out_vol(dev, 30.0);
    TEST_ESP_OK(ret);
    // test for mute setting
    ret = esp_codec_dev_set_out_mute(dev, true);
    TEST_ESP_OK(ret);
    // test for microphone gain
    ret = esp_codec_dev_set_in_gain(dev, 20.0);
    TEST_ESP_OK(ret);

    // test for microphone mute setting
    ret = esp_codec_dev_set_in_mute(dev, true);
    TEST_ESP_OK(ret);

    // APP need fail after close
    // test for read data
    ret = esp_codec_dev_read(dev, data, 256);
    TEST_ASSERT(ret != CODEC_DEV_OK);
    // test for write data
    ret = esp_codec_dev_write(dev, data, 512);
    TEST_ASSERT(ret != CODEC_DEV_OK);

    free(data);
    // Delete codec dev handle
    esp_codec_dev_delete(dev);
    // Delete codec interface
    audio_codec_delete_codec_if(codec_if);
    // Delete codec control interface
    audio_codec_delete_ctrl_if(ctrl_if);
    // Delete codec data interface
    audio_codec_delete_data_if(data_if);
}

TEST_CASE("esp codec dev wrong argument test", "[esp_codec_dev]")
{
    const audio_codec_ctrl_if_t *ctrl_if = my_codec_ctrl_new();
    TEST_ASSERT_NOT_NULL(ctrl_if);
    const audio_codec_data_if_t *data_if = my_codec_data_new();
    TEST_ASSERT_NOT_NULL(data_if);
    my_codec_cfg_t codec_cfg = {
        .ctrl_if = ctrl_if,
    };
    const audio_codec_if_t *codec_if = my_codec_new(&codec_cfg);
    TEST_ASSERT_NOT_NULL(codec_if);
    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = CODEC_DEV_TYPE_IN_OUT,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    esp_codec_dev_handle_t dev = esp_codec_dev_new(&dev_cfg);
    TEST_ASSERT_NOT_NULL(codec_if);

    esp_codec_dev_handle_t dev_bad = esp_codec_dev_new(NULL);
    TEST_ASSERT(dev_bad == NULL);
    dev_cfg.data_if = NULL;
    dev_bad = esp_codec_dev_new(&dev_cfg);
    TEST_ASSERT(dev_bad == NULL);

    int ret = esp_codec_dev_open(dev, NULL);
    TEST_ASSERT(ret != CODEC_DEV_OK);

    esp_codec_dev_vol_curve_t vol_curve = {
        .count = 2,
    };
    // Test for volume curve settings
    ret = esp_codec_dev_set_vol_curve(dev, &vol_curve);
    TEST_ASSERT(ret != CODEC_DEV_OK);

    // Test for volume setting
    ret = esp_codec_dev_set_out_vol(NULL, 30.0);
    TEST_ASSERT(ret != CODEC_DEV_OK);

    // test for mute setting
    ret = esp_codec_dev_set_out_mute(NULL, true);
    TEST_ASSERT(ret != CODEC_DEV_OK);

    // test for microphone gain
    ret = esp_codec_dev_set_in_gain(NULL, 20.0);
    TEST_ASSERT(ret != CODEC_DEV_OK);

    // test for microphone mute setting
    ret = esp_codec_dev_set_in_mute(NULL, true);
    TEST_ASSERT(ret != CODEC_DEV_OK);

    // test for read data
    uint8_t data[16];
    ret = esp_codec_dev_read(dev, NULL, sizeof(data));
    TEST_ASSERT(ret != CODEC_DEV_OK);
    ret = esp_codec_dev_read(dev, data, sizeof(data));
    TEST_ASSERT(ret != CODEC_DEV_OK);
    ret = esp_codec_dev_read(NULL, NULL, sizeof(data));
    TEST_ASSERT(ret != CODEC_DEV_OK);

    // test for write data
    ret = esp_codec_dev_write(dev, data, sizeof(data));
    TEST_ASSERT(ret != CODEC_DEV_OK);
    ret = esp_codec_dev_write(NULL, data, sizeof(data));
    TEST_ASSERT(ret != CODEC_DEV_OK);
    ret = esp_codec_dev_write(dev, NULL, sizeof(data));
    TEST_ASSERT(ret != CODEC_DEV_OK);

    esp_codec_dev_close(dev);
    // Delete codec dev handle
    esp_codec_dev_delete(dev);
    // Delete codec interface
    audio_codec_delete_codec_if(codec_if);
    // Delete codec control interface
    audio_codec_delete_ctrl_if(ctrl_if);
    // Delete codec data interface
    audio_codec_delete_data_if(data_if);
}

TEST_CASE("esp codec dev feature should not support", "[esp_codec_dev]")
{
    const audio_codec_ctrl_if_t *ctrl_if = my_codec_ctrl_new();
    TEST_ASSERT_NOT_NULL(ctrl_if);
    const audio_codec_data_if_t *data_if = my_codec_data_new();
    TEST_ASSERT_NOT_NULL(data_if);
    my_codec_cfg_t codec_cfg = {
        .ctrl_if = ctrl_if,
    };
    const audio_codec_if_t *codec_if = my_codec_new(&codec_cfg);
    TEST_ASSERT_NOT_NULL(codec_if);
    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = CODEC_DEV_TYPE_IN,
        .codec_if = codec_if,
        .data_if = data_if,
    };
    int ret = 0;
    uint8_t data[16];
    // input device should not support output function
    {
        esp_codec_dev_handle_t dev = esp_codec_dev_new(&dev_cfg);
        TEST_ASSERT_NOT_NULL(codec_if);
        codec_dev_vol_map_t vol_maps[2] = {
            {.vol = 0,   .db_value = 0  },
            {.vol = 100, .db_value = 100},
        };
        esp_codec_dev_vol_curve_t vol_curve = {
            .count = 2,
            .vol_map = vol_maps,
        };
        // Test for volume curve settings
        ret = esp_codec_dev_set_vol_curve(dev, &vol_curve);
        TEST_ASSERT(ret != CODEC_DEV_OK);

        // Test for volume setting
        ret = esp_codec_dev_set_out_vol(dev, 30.0);
        TEST_ASSERT(ret != CODEC_DEV_OK);

        // test for mute setting
        ret = esp_codec_dev_set_out_mute(NULL, true);
        TEST_ASSERT(ret != CODEC_DEV_OK);

        // test for write data
        ret = esp_codec_dev_write(dev, data, sizeof(data));
        TEST_ASSERT(ret != CODEC_DEV_OK);
        esp_codec_dev_close(dev);
        // Delete codec dev handle
        esp_codec_dev_delete(dev);
    }
    // output device should not support input function
    {
        dev_cfg.dev_type = CODEC_DEV_TYPE_OUT;
        esp_codec_dev_handle_t dev = esp_codec_dev_new(&dev_cfg);
        TEST_ASSERT_NOT_NULL(dev);

        // Test for volume setting
        ret = esp_codec_dev_set_in_gain(dev, 20.0);
        TEST_ASSERT(ret != CODEC_DEV_OK);

        // test for mute setting
        ret = esp_codec_dev_set_in_mute(dev, true);
        ;
        TEST_ASSERT(ret != CODEC_DEV_OK);

        // test for write data
        ret = esp_codec_dev_read(dev, data, sizeof(data));
        TEST_ASSERT(ret != CODEC_DEV_OK);
        esp_codec_dev_close(dev);
        // Delete codec dev handle
        esp_codec_dev_delete(dev);
    }
    // Delete codec interface
    audio_codec_delete_codec_if(codec_if);
    // Delete codec control interface
    audio_codec_delete_ctrl_if(ctrl_if);
    // Delete codec data interface
    audio_codec_delete_data_if(data_if);
}

#define CONFIG_USE_S3_KORVO2_V3

#ifdef CONFIG_USE_S3_KORVO2_V3
#include "codec_dev_types.h"
#include "codec_dev_gpio.h"
#include "codec_dev_defaults.h"
#include "driver/i2c.h"
#include "driver/i2s.h"
#include "es8311.h"
#include "es7210.h"

#define TEST_CODEC_I2C_SDA_PIN      (17)
#define TEST_CODEC_I2C_SCL_PIN      (18)

#define TEST_CODEC_I2S_BCK_PIN      (9)
#define TEST_CODEC_I2S_MCK_PIN      (16)
#define TEST_CODEC_I2S_DATA_IN_PIN  (10)
#define TEST_CODEC_I2S_DATA_OUT_PIN (8)
#define TEST_CODEC_I2S_DATA_WS_PIN  (45)

#define TEST_CODEC_PA_PIN           (48)

static int ut_i2c_init(uint8_t port)
{
    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_cfg.sda_io_num = TEST_CODEC_I2C_SDA_PIN;
    i2c_cfg.scl_io_num = TEST_CODEC_I2C_SCL_PIN;
    esp_err_t ret = i2c_param_config(port, &i2c_cfg);
    if (ret != ESP_OK) {
        return -1;
    }
    return i2c_driver_install(port, i2c_cfg.mode, 0, 0, 0);
}

static int ut_i2c_deinit(uint8_t port)
{
    return i2c_driver_delete(port);
}

static int ut_i2s_init(uint8_t port)
{
    i2s_config_t i2s_config = {.mode = (i2s_mode_t) (I2S_MODE_TX | I2S_MODE_RX | I2S_MODE_MASTER),
                               .sample_rate = 44100,
                               .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                               .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
                               .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                               .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,
                               .dma_buf_count = 2,
                               .dma_buf_len = 128,
                               .use_apll = true,
                               .tx_desc_auto_clear = true,
                               .fixed_mclk = 0};
    int ret = i2s_driver_install(port, &i2s_config, 0, NULL);
    i2s_pin_config_t i2s_pin_cfg = {
        .mck_io_num = TEST_CODEC_I2S_MCK_PIN,
        .bck_io_num = TEST_CODEC_I2S_BCK_PIN,
        .ws_io_num = TEST_CODEC_I2S_DATA_WS_PIN,
        .data_out_num = TEST_CODEC_I2S_DATA_OUT_PIN,
        .data_in_num = TEST_CODEC_I2S_DATA_IN_PIN,
    };
    i2s_set_pin(port, &i2s_pin_cfg);
    return ret;
}

static int ut_i2s_deinit(uint8_t port)
{
    i2s_driver_uninstall(port);
    return 0;
}

static int codec_max_sample(uint8_t *data, int size)
{
    int16_t *s = (int16_t *) data;
    size >>= 1;
    int i = 0, max = 0;
    while (i < size) {
        if (s[i] > max) {
            max = s[i];
        }
        i++;
    }
    return max;
}

TEST_CASE("esp codec dev test using S3 board", "[esp_codec_dev]")
{
    int ret = ut_i2c_init(0);
    TEST_ESP_OK(ret);
    ret = ut_i2s_init(0);
    TEST_ESP_OK(ret);

    codec_i2s_dev_cfg_t i2s_cfg = {};
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data_if(&i2s_cfg);
    TEST_ASSERT_NOT_NULL(data_if);

    codec_i2c_dev_cfg_t i2c_cfg = {.addr = ES8311_CODEC_DEFAULT_ADDR};
    const audio_codec_ctrl_if_t *out_ctrl_if = audio_codec_new_i2c_ctrl_if(&i2c_cfg);
    TEST_ASSERT_NOT_NULL(out_ctrl_if);

    i2c_cfg.addr = ES7210_CODEC_DEFAULT_ADDR;
    const audio_codec_ctrl_if_t *in_ctrl_if = audio_codec_new_i2c_ctrl_if(&i2c_cfg);
    TEST_ASSERT_NOT_NULL(in_ctrl_if);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio_if();
    TEST_ASSERT_NOT_NULL(gpio_if);
    audio_codec_set_gpio_if(gpio_if);

    es8311_codec_cfg_t es8311_cfg = {
        .codec_mode = CODEC_WORK_MODE_DAC,
        .ctrl_if = out_ctrl_if,
        .pa_pin = TEST_CODEC_PA_PIN,
    };
    const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);
    TEST_ASSERT_NOT_NULL(out_codec_if);

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = in_ctrl_if,
        .mic_selected = ES7120_SEL_MIC1 | ES7120_SEL_MIC2 | ES7120_SEL_MIC3,
    };
    const audio_codec_if_t *in_codec_if = es7210_codec_new(&es7210_cfg);
    TEST_ASSERT_NOT_NULL(in_codec_if);

    esp_codec_dev_cfg_t dev_cfg = {
        .codec_if = out_codec_if,
        .data_if = data_if,
        .dev_type = CODEC_DEV_TYPE_OUT,
    };
    esp_codec_dev_handle_t play_dev = esp_codec_dev_new(&dev_cfg);
    TEST_ASSERT_NOT_NULL(play_dev);
    dev_cfg.codec_if = in_codec_if;
    dev_cfg.dev_type = CODEC_DEV_TYPE_IN;
    esp_codec_dev_handle_t record_dev = esp_codec_dev_new(&dev_cfg);
    TEST_ASSERT_NOT_NULL(record_dev);

    ret = esp_codec_dev_set_out_vol(play_dev, 45.0);
    TEST_ESP_OK(ret);
    ret = esp_codec_dev_set_in_gain(record_dev, 30.0);
    TEST_ESP_OK(ret);

    codec_sample_info_t fs = {
        .sample_rate = 48000,
        .channel = 2,
        .bits_per_sample = 16,
    };
    ret = esp_codec_dev_open(play_dev, &fs);
    TEST_ESP_OK(ret);

    ret = esp_codec_dev_open(record_dev, &fs);
    TEST_ESP_OK(ret);
    uint8_t *data = (uint8_t *) malloc(512);
    int limit_size = 10 * fs.sample_rate * fs.channel * (fs.bits_per_sample >> 3);
    int got_size = 0;
    int max_sample = 0;
    while (got_size < limit_size) {
        ret = esp_codec_dev_read(record_dev, data, 512);
        TEST_ESP_OK(ret);
        ret = esp_codec_dev_write(play_dev, data, 512);
        TEST_ESP_OK(ret);
        int max_value = codec_max_sample(data, 512);
        if (max_value > max_sample) {
            max_sample = max_value;
        }
        got_size += 512;
    }
    TEST_ASSERT(max_sample > 0);
    free(data);

    ret = esp_codec_dev_close(play_dev);
    TEST_ESP_OK(ret);
    ret = esp_codec_dev_close(record_dev);
    TEST_ESP_OK(ret);
    esp_codec_dev_delete(play_dev);
    esp_codec_dev_delete(record_dev);

    // Delete codec interface
    audio_codec_delete_codec_if(in_codec_if);
    audio_codec_delete_codec_if(out_codec_if);
    // Delete codec control interface
    audio_codec_delete_ctrl_if(in_ctrl_if);
    audio_codec_delete_ctrl_if(out_ctrl_if);
    audio_codec_delete_gpio_if(gpio_if);
    // Delete codec data interface
    audio_codec_delete_data_if(data_if);

    ut_i2c_deinit(0);
    ut_i2s_deinit(0);
}

#endif
