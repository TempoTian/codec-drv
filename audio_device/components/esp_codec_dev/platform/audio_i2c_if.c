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

#include "audio_codec_ctrl_if.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "codec_dev_err.h"

#define TAG "I2C_If"

typedef struct {
    audio_codec_ctrl_if_t base;
    bool                  is_open;
    uint8_t               port;
    uint8_t               addr;
} i2c_ctrl_t;

int _i2c_ctrl_open(const audio_codec_ctrl_if_t *ctrl, void *cfg, int cfg_size)
{
    if (ctrl == NULL || cfg == NULL || cfg_size != sizeof(codec_i2c_dev_cfg_t)) {
        return CODEC_DEV_INVALID_ARG;
    }
    i2c_ctrl_t *i2c_ctrl = (i2c_ctrl_t *) ctrl;
    codec_i2c_dev_cfg_t *i2c_cfg = (codec_i2c_dev_cfg_t *) cfg;
    i2c_ctrl->port = i2c_cfg->port;
    i2c_ctrl->addr = i2c_cfg->addr;
    return 0;
}

bool _i2c_ctrl_is_open(const audio_codec_ctrl_if_t *ctrl)
{
    if (ctrl) {
        i2c_ctrl_t *i2c_ctrl = (i2c_ctrl_t *) ctrl;
        return i2c_ctrl->is_open;
    }
    return false;
}

static int _i2c_ctrl_read_addr(const audio_codec_ctrl_if_t *ctrl, int addr, int addr_len, void *data, int data_len)
{
    if (ctrl == NULL || data == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    i2c_ctrl_t *i2c_ctrl = (i2c_ctrl_t *) ctrl;
    if (i2c_ctrl->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd;
    cmd = i2c_cmd_link_create();
    ret |= i2c_master_start(cmd);
    ret |= i2c_master_write_byte(cmd, i2c_ctrl->addr, 1);
    ret |= i2c_master_write(cmd, (uint8_t *) &addr, addr_len, 1);
    ret |= i2c_master_stop(cmd);
    ret |= i2c_master_cmd_begin(i2c_ctrl->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    ret |= i2c_master_start(cmd);
    ret |= i2c_master_write_byte(cmd, i2c_ctrl->addr | 0x01, 1);

    for (int i = 0; i < data_len - 1; i++) {
        ret |= i2c_master_read_byte(cmd, (uint8_t *) data + i, 0);
    }
    ret |= i2c_master_read_byte(cmd, (uint8_t *) data + (data_len - 1), 1);

    ret = i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_ctrl->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != 0) {
        ESP_LOGE(TAG, "Fail to read from dev %x", i2c_ctrl->addr);
    }
    return ret ? CODEC_DEV_READ_FAIL : CODEC_DEV_OK;
}

static int _i2c_ctrl_write_addr(const audio_codec_ctrl_if_t *ctrl, int addr, int addr_len, void *data, int data_len)
{
    i2c_ctrl_t *i2c_ctrl = (i2c_ctrl_t *) ctrl;
    if (ctrl == NULL || data == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    if (i2c_ctrl->is_open == false) {
        return CODEC_DEV_WRONG_STATE;
    }
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret |= i2c_master_start(cmd);
    ret |= i2c_master_write_byte(cmd, i2c_ctrl->addr, 1);
    ret |= i2c_master_write(cmd, (uint8_t *) &addr, addr_len, 1);
    if (data_len) {
        ret |= i2c_master_write(cmd, data, data_len, 1);
    }
    ret |= i2c_master_stop(cmd);
    ret |= i2c_master_cmd_begin(i2c_ctrl->port, cmd, 1000 / portTICK_RATE_MS);
    if (ret != 0) {
        ESP_LOGE(TAG, "Fail to write to dev %x", i2c_ctrl->addr);
    }
    i2c_cmd_link_delete(cmd);
    return ret ? CODEC_DEV_WRITE_FAIL : CODEC_DEV_OK;
}

int _i2c_ctrl_close(const audio_codec_ctrl_if_t *ctrl)
{
    if (ctrl == NULL) {
        return CODEC_DEV_INVALID_ARG;
    }
    i2c_ctrl_t *i2c_ctrl = (i2c_ctrl_t *) ctrl;
    i2c_ctrl->is_open = false;
    return 0;
}

const audio_codec_ctrl_if_t *audio_codec_new_i2c_ctrl_if(codec_i2c_dev_cfg_t *i2c_cfg)
{
    i2c_ctrl_t *ctrl = calloc(1, sizeof(i2c_ctrl_t));
    if (ctrl == NULL) {
        return NULL;
    }
    ctrl->base.open = _i2c_ctrl_open;
    ctrl->base.is_open = _i2c_ctrl_is_open;
    ctrl->base.read_addr = _i2c_ctrl_read_addr;
    ctrl->base.write_addr = _i2c_ctrl_write_addr;
    ctrl->base.close = _i2c_ctrl_close;
    int ret = _i2c_ctrl_open(&ctrl->base, i2c_cfg, sizeof(codec_i2c_dev_cfg_t));
    if (ret != 0) {
        free(ctrl);
        return NULL;
    }
    ctrl->is_open = true;
    return &ctrl->base;
}
